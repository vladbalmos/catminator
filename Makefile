.PHONY: build install

build:
	mkdir -p build
	cd build && cmake .. && make

install: build
	cd build && pico-install.sh catminator

