# WIFI support for STEVAL-STWINBX1

![latest tag](https://img.shields.io/github/v/tag/stm32-hotspot/STWINBX1_WIFI.svg?color=brightgreen)


**STWINBX1_WIFI** package enables the support for the MXCHIP WIFI module mounted on the STEVAL-STWINBX1.

The packages includes 3 applications:
- Nx_FtpServer
- Nx_MQTT_Client
- Nx_WebServer

Before using those applications, WIFI module firwmare must be upgraded.
The binary is available in the WiFi_module_upgrade folder.

For each firmware, a detailed readme describing the full setup is provided in each application folder.


## Known Limitations

- None

## Development Toolchains and Compilers

- 	IAR Embedded Workbench for ARM (EWARM) toolchain V9.20
-   STM32CubeIDE v1.12.0

## Supported Devices and Boards

- [STEVAL-STWINBX1](https://www.st.com/stwinbox)

## Backward Compatibility

- None

## Dependencies

- None
