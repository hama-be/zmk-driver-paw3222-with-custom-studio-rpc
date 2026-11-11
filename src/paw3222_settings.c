/* SPDX-License-Identifier: Apache-2.0 */
/* PAW3222 settings use the generic custom-settings RPC, not PMW3610 RPC. */
#define DT_DRV_COMPAT pixart_paw3222
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/studio/custom.h>
#include <cormoran/zmk/custom_settings.h>
#include "paw3222.h"

LOG_MODULE_REGISTER(paw3222_settings, CONFIG_ZMK_LOG_LEVEL);
#define SUBSYSTEM "cormoran__paw3222"
#define DEVICE_ID(n) DT_INST_PROP_OR(n, settings_id, DT_NODE_FULL_NAME(DT_DRV_INST(n)))
#define KEY(n, field) field "@" DEVICE_ID(n)

/* Discovery identity. All setting requests go through custom-settings, so
 * direct sensor-specific requests are deliberately unsupported. */
static struct zmk_rpc_custom_subsystem_meta meta = {
    .security = ZMK_STUDIO_RPC_HANDLER_SECURED,
};
static bool unsupported(const zmk_custom_CallRequest *request, pb_callback_t *response) {
    ARG_UNUSED(request);
    ARG_UNUSED(response);
    return false;
}
ZMK_RPC_CUSTOM_SUBSYSTEM(cormoran__paw3222, &meta, unsupported);

#define DEFINE_SETTINGS(n) \
    ZMK_CUSTOM_SETTING_DEFINE(paw_cpi_##n, SUBSYSTEM, KEY(n, "cpi"), \
        ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32, \
        ZMK_CUSTOM_SETTING_VALUE_INT32(DT_INST_PROP_OR(n, res_cpi, 608)), \
        ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC, \
        ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE, ZMK_CUSTOM_SETTING_PERMISSION_SECURE, \
        ZMK_CUSTOM_SETTING_RANGE_INT32(608, 4826)); \
    ZMK_CUSTOM_SETTING_DEFINE(paw_awake_##n, SUBSYSTEM, KEY(n, "force_awake"), \
        ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL, \
        ZMK_CUSTOM_SETTING_VALUE_BOOL(DT_INST_PROP(n, force_awake)), \
        ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC, \
        ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE, ZMK_CUSTOM_SETTING_PERMISSION_SECURE, \
        ZMK_CUSTOM_SETTING_NO_CONSTRAINT);
DT_INST_FOREACH_STATUS_OKAY(DEFINE_SETTINGS)

static void apply_settings(struct k_work *work) {
    ARG_UNUSED(work);
    struct zmk_custom_setting_value value;
#define APPLY(n) \
    do { \
        const struct device *dev = DEVICE_DT_INST_GET(n); \
        if (!device_is_ready(dev)) { break; } \
        int ret; \
        if (zmk_custom_setting_read_by_key(SUBSYSTEM, KEY(n, "cpi"), &value) == 0) { \
            ret = paw32xx_set_resolution(dev, value.int32_value); \
            if (ret < 0) { LOG_ERR("CPI apply failed (%s): %d", DEVICE_ID(n), ret); } \
        } \
        if (zmk_custom_setting_read_by_key(SUBSYSTEM, KEY(n, "force_awake"), &value) == 0) { \
            ret = paw32xx_force_awake(dev, value.bool_value); \
            if (ret < 0) { LOG_ERR("Force-awake apply failed (%s): %d", DEVICE_ID(n), ret); } \
        } \
    } while (0);
    DT_INST_FOREACH_STATUS_OKAY(APPLY)
#undef APPLY
}
/* Run outside the settings RPC/event thread. At boot the initialized event
 * is raised after persisted values load; device init has already completed. */
K_WORK_DEFINE(apply_work, apply_settings);
static int setting_changed(const zmk_event_t *event) {
    const struct zmk_custom_setting_changed *changed = as_zmk_custom_setting_changed(event);
    if (as_zmk_custom_settings_initialized(event) ||
        (changed && changed->setting &&
         strcmp(changed->setting->custom_subsystem_id, SUBSYSTEM) == 0)) {
        k_work_submit(&apply_work);
    }
    return 0;
}
ZMK_LISTENER(paw3222_settings, setting_changed);
ZMK_SUBSCRIPTION(paw3222_settings, zmk_custom_setting_changed);
ZMK_SUBSCRIPTION(paw3222_settings, zmk_custom_settings_initialized);
