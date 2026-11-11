# PAW3222 with DYA Studio settings (prototype)

Based on [sekigon-gonnoc/zmk-driver-paw3222](https://github.com/sekigon-gonnoc/zmk-driver-paw3222).
Existing driver license and attribution are retained. This extension publishes
sensor settings via cormoran's shared custom-settings RPC, under the separate
`cormoran__paw3222` identity. It does not emulate the PMW3610 protocol.

## Enable

Use the cormoran ZMK fork with custom Studio RPC and add
`cormoran/zmk-feature-custom-settings` plus this module to west.yml.
Do not also load the original PAW3222 module (same driver and DT compatible).

```conf
CONFIG_PAW3222=y
CONFIG_ZMK_STUDIO=y
CONFIG_ZMK_STUDIO_LOCKING=y
CONFIG_ZMK_PAW3222_CUSTOM_SETTINGS=y
```

Keep your SPI wiring and `compatible = "pixart,paw3222"`. Add a unique,
stable `settings-id = "trackball"` to each sensor node, and set
`res-cpi = <608>` for the default. Without an id, the node name is used.
Bind `&studio_unlock` in your keymap to authorize Studio writes.

## Operate

In the accompanying DYA Studio branch, open Trackball → Sensor Drivers →
`cormoran__paw3222`. The stock Studio also lists these keys in Settings →
Advanced Settings. Edit `cpi@trackball` or `force_awake@trackball`, wait for the
memory write, then select **Save** to retain changes across reboots.
**Discard** restores the saved values; **Reset** restores firmware defaults.

CPI range is 608–4826. The chip uses 38-CPI steps; values are rounded down
by the upstream API. The accompanying UI submits whole steps. Other clients
can submit intermediate values: the saved requested CPI may then differ from
the hardware's rounded-down CPI. Force awake disables the driver's sensor
sleep bits and can increase battery use. Input rotation, sensitivity scaling,
and scroll behavior remain in the separate runtime input processors.

This is a hardware-validation prototype. Settings readback reflects the
requested value, not a verified sensor register read. SPI apply failures are
logged; the GUI is not proof that hardware accepted a write. It has no raw
register, frame capture, or PMW3610-specific power timing controls. Only local
sensors on the Studio central are supported (no PAW-specific peripheral RPC).

The existing driver documentation follows below.

---

# ZMK PAW3222 Driver

This driver enables the use of the PIXART PAW3222 optical sensor with the ZMK framework.

## Overview

The PAW3222 is a low-power optical mouse sensor suitable for tracking applications such as mice and trackballs. This driver communicates with the PAW3222 sensor via SPI interface.

## Installation

1. Add as a ZMK module in your west.yml:

```
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: sekigon-gonnoc
      url-base: https://github.com/sekigon-gonnoc
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-driver-paw3222
      remote: sekigon-gonnoc
      revision: main
```

## Device Tree Configuration

Configure in your shield or board config file (.overlay or .dtsi):

```dts
&pinctrl {
    spi0_default: spi0_default {
        group1 {
            psels = <NRF_PSEL(SPIM_SCK, 0, 12)>,
                <NRF_PSEL(SPIM_MOSI, 1, 9)>,
                <NRF_PSEL(SPIM_MISO, 1, 9)>;
        };
    };

    spi0_sleep: spi0_sleep {
        group1 {
            psels = <NRF_PSEL(SPIM_SCK, 0, 12)>,
                <NRF_PSEL(SPIM_MOSI, 1, 9)>,
                <NRF_PSEL(SPIM_MISO, 1, 9)>;
            low-power-enable;
        };
    };
};

&spi0 {
    status = "okay";
    compatible = "nordic,nrf-spim";
    pinctrl-0 = <&spi0_default>;
    pinctrl-1 = <&spi0_sleep>;
    pinctrl-names = "default", "sleep";
    cs-gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;

    trackball: trackball@0 {
        status = "okay";
        compatible = "pixart,paw3222";
        reg = <0>;
        spi-max-frequency = <2000000>;
        irq-gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;
    };
};
```

## Enable the module in your keyboard's Kconfig file

Add the following to your keyboard's `Kconfig.defconfig`:

```kconfig
if ZMK_KEYBOARD_YOUR_KEYBOARD

config ZMK_POINTING
    default y

config PAW3222
    default y

endif
```

## Properties

- `irq-gpios`: GPIO connected to the motion pin (required)
- `res-cpi`: CPI resolution for the sensor (optional)
- `force-awake`: Initialize the sensor in "force awake" mode (optional, boolean)

---

# ZMK PAW3222 ドライバ

このドライバは、PIXART PAW3222光学センサーをZMKフレームワークで使用できるようにします。

## 概要

PAW3222は、マウスやトラックボールなどのトラッキングアプリケーションに適した低消費電力の光学マウスセンサーです。このドライバはSPIインターフェースを介してPAW3222センサーと通信します。

## インストール

1. ZMKモジュールとして追加：

```
# west.yml に追加
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: sekigon-gonnoc
      url-base: https://github.com/sekigon-gonnoc
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-driver-paw3222
      remote: sekigon-gonnoc
      revision: main
```

## デバイスツリー設定

シールドまたはボード設定ファイル（.overlayまたは.dtsi）で設定：

```dts
&pinctrl {
    spi0_default: spi0_default {
        group1 {
            psels = <NRF_PSEL(SPIM_SCK, 0, 12)>,
                <NRF_PSEL(SPIM_MOSI, 1, 9)>,
                <NRF_PSEL(SPIM_MISO, 1, 9)>;
        };
    };

    spi0_sleep: spi0_sleep {
        group1 {
            psels = <NRF_PSEL(SPIM_SCK, 0, 12)>,
                <NRF_PSEL(SPIM_MOSI, 1, 9)>,
                <NRF_PSEL(SPIM_MISO, 1, 9)>;
            low-power-enable;
        };
    };
};

&spi0 {
    status = "okay";
    compatible = "nordic,nrf-spim";
    pinctrl-0 = <&spi0_default>;
    pinctrl-1 = <&spi0_sleep>;
    pinctrl-names = "default", "sleep";
    cs-gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;

    trackball: trackball@0 {
        status = "okay";
        compatible = "pixart,paw3222";
        reg = <0>;
        spi-max-frequency = <2000000>;
        irq-gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;
    };
};
```

## キーボードのKconfigファイルでモジュールを有効化

キーボードの `Kconfig.defconfig` に以下を追加：

```kconfig
if ZMK_KEYBOARD_YOUR_KEYBOARD

config ZMK_POINTING
    default y

config PAW3222
    default y

endif
```

## プロパティ

- `irq-gpios`: モーションピンに接続されたGPIO（必須）
- `res-cpi`: センサーのCPI解像度（任意）
- `force-awake`: センサーを「強制起動」モードで初期化（任意、ブール値）
