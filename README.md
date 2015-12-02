Build nodemcu from source
=========================

This project basically wraps [nodemcu-firmware](https://github.com/nodemcu/nodemcu-firmware) (included as a subtree) with a Makefile that makes creating a custom build and flashing it to
the nodemcu unit easy.

Use:
----

1. modify fw/nodemcu-firmware/app/include/user_config.h and fw/nodemcu-firmware/app/include/user_modules.h to specify the modules you want to be built in.

2. make build

3. make flash

If the esp8266 is connected to a device other than /dev/ttyUSB0, specify the device using the PORT environment variable:
```
PORT=/dev/ttyUSB1 make flash
```

By default DIO mode is used to flash the device. If you want to use the QIO mode, specify it using the FLASH_MODE environment variable:
```
FLASH_MODE=qio make flash
```
