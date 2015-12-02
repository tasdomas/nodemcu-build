PORT ?= /dev/ttyUSB0
FLASH_MODE ?= dio

ESP_OPEN_SDK = $(CURDIR)/esp-open-sdk
XTENSA = xtensa-lx106-elf/bin
FW = fw/nodemcu-firmware
ESPTOOL = $(FW)/tools/esptool.py

DEPS = git make python-serial srecord

all: build

deps:
	sudo apt-get install $(DEPS)

$(ESP_OPEN_SDK)/Makefile:
	@echo "Fetched without --recursive, updating submodules."
	git submodule update --init --recursive $(basename $(ESP_OPEN_SDK))

build: $(FW)/bin/0x00000.bin

$(FW)/bin/0x00000.bin: $(FW)/app/include/user_config.h $(FW)/app/include/user_modules.h $(ESP_OPEN_SDK)/$(XTENSA)
	PATH=$(ESP_OPEN_SDK)/$(XTENSA):$(PATH) $(MAKE) -C $(FW)

$(ESP_OPEN_SDK)/$(XTENSA): $(FW)/tools/esp-open-sdk.tar.gz
	tar -xzvf $<

flash: $(FW)/bin/0x00000.bin
	$(ESPTOOL) --port $(PORT) write_flash -fm $(FLASH_MODE) 0x00000 $(FW)/bin/0x00000.bin 0x10000 $(FW)/bin/0x10000.bin

clean:
	rm -rf esp-open-sdk
	git clean -xdf
	$(MAKE) -C $(FW) clean
