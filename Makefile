ESP_OPEN_SDK = $(CURDIR)/esp-open-sdk
XTENSA = xtensa-lx106-elf/bin
FW = fw/node-mcu-firmware

DEPS = make unrar autoconf automake libtool gcc g++ gperf flex bison texinfo gawk ncurses-dev libexpat-dev python python-serial sed git libtool-bin screen picocom

all: build

deps:
	sudo apt-get install $(DEPS)

$(ESP_OPEN_SDK)/Makefile:
	@echo "Fetched without --recursive, updating submodules."
	git submodule update --init --recursive $(basename $(ESP_OPEN_SDK))

build: $(ESP_OPEN_SDK)/$(XTENSA)
	PATH=$(ESP_OPEN_SDK)/$(XTENSA):$(PATH) $(MAKE) -C $(FW)

$(ESP_OPEN_SDK)/$(XTENSA): $(ESP_OPEN_SDK)/Makefile
	cd $(ESP_OPEN_SDK); $(MAKE) STANDALONE=y

