ESP_OPEN_SDK = esp_open_sdk

DEPS = make unrar autoconf automake libtool gcc g++ gperf flex bison texinfo gawk ncurses-dev libexpat-dev python python-serial sed git libtool-bin screen picocom

all: build

deps:
	sudo apt-get install $(DEPS)

$(ESP_OPEN_SDK):
	@echo "Fetched without --recursive, updating submodules."
	git submodule update --init --recursive $@

build: $(ESP_OPEN_SDK)/bin
	$(MAKE) -C $(ESP_OPEN_SDK) STANDALONE=y

