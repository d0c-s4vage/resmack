.PHONY: clean
clean:
	rm -rf build

all: clean debug release

# -----------------------------------------------------------------------------
# DEBUG -----------------------------------------------------------------------
# -----------------------------------------------------------------------------

.PHONY: debug
debug: build/debug build/debug/resmack

run-debug: debug
	build/debug/resmack

build/debug:
	mkdir -p build/debug ; \
	cd build/debug ; \
	cmake ../../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ; \
	cp compile_commands.json ../../

.PHONY: build/debug/resmack
build/debug/resmack:
	cd build/debug ; \
	make -j $(proc)

# -----------------------------------------------------------------------------
# RELEASE ---------------------------------------------------------------------
# -----------------------------------------------------------------------------

release: build/release build/release/resmack

run-release: release
	build/release/resmack

build/release:
	mkdir -p build/release ; \
	cd build/release ; \
	cmake ../../ -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ; \
	cp compile_commands.json ../../

.PHONY: build/release/resmack
build/release/resmack:
	cd build/release ; \
	make -j $(proc)

# -----------------------------------------------------------------------------
# TEST ------------------------------------------------------------------------
# -----------------------------------------------------------------------------

test: libs-test/google-test

libs-test/google-test: libs-test/googletest/build/lib/libgtest_main.a

libs-test/googletest/build/lib/libgtest_main.a:
	mkdir -p libs-test
	wget -qO- "https://github.com/google/googletest/archive/release-1.10.0.tar.gz" | tar xvz -C libs-test --transform 's/^googletest-release-1.10.0/googletest/'
	cd libs-test/googletest ; \
		mkdir build ; \
		cd build ; \
		cmake ../ ; \
		make -j $(nproc)
