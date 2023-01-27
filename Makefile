.PHONY: build clean debug install-debug install

build:
	mkdir -p build
	cd build && cmake .. && make -j4

debug:
	mkdir -p debug
	cd debug && DEBUG_MODE=1 cmake .. && make -j4

clean:
	rm -rf build
	rm -rf debug

install: build
	cd build && pico-install.sh catminator

install-debug: debug
	cd debug && pico-install.sh catminator
