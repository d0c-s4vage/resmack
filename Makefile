.PHONY: clean
clean:
	rm -rf build

all: clean debug release

RESMACK_PERF_EXE=resmack_perf

# -----------------------------------------------------------------------------
# DEBUG -----------------------------------------------------------------------
# -----------------------------------------------------------------------------

.PHONY: debug
debug: build/debug build/debug/$(RESMACK_PERF_EXE)

.PHONY: clean-debug
clean-debug:
	rm -rf build/debug

run-debug: debug
	build/debug/$(RESMACK_PERF_EXE)

gdb-debug: debug
	gdb -ex run build/debug/$(RESMACK_PERF_EXE)

gdb-test: test
	gdb -ex run build/test/test/test_resmack

build/debug:
	mkdir -p build/debug ; \
	cd build/debug ; \
	cmake ../../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ; \
	cp -u compile_commands.json ../../

.PHONY: build/debug/$(RESMACK_PERF_EXE)
build/debug/$(RESMACK_PERF_EXE):
	cd build/debug ; \
	make -j $(nproc)

# -----------------------------------------------------------------------------
# RELEASE ---------------------------------------------------------------------
# -----------------------------------------------------------------------------

RELEASE_TYPE=Release
RELEASE_SUFFIX=""
RELEASE_PATH=build/release$(RELEASE_SUFFIX)

.PHONY: \
	release \
	release-syms \
	build-release \
	perf-release-inner \
	$(RELEASE_PATH)/$(RESMACK_PERF_EXE)

release:
	$(MAKE) build-release

.PHONY: clean-release
clean-release:
	rm -rf build/release

run-release: release
	$(RELEASE_PATH)/$(RESMACK_PERF_EXE)

release-syms:
	$(MAKE) build-release RELEASE_TYPE=RelWithDebInfo RELEASE_SUFFIX="-syms"

gdb-release: release-syms
	gdb -ex run $(RELEASE_PATH)-syms/$(RESMACK_PERF_EXE)

run-perf: release-syms
	bash -c "trap 'trap - SIGINT ERR SIGTERM; perf report; exit 1' SIGINT SIGTERM ERR; $(MAKE) perf-release-inner"

perf-release-inner:
	perf record --call-graph dwarf -g $(RELEASE_PATH)-syms/$(RESMACK_PERF_EXE) || true


# -----------------------------------------------------------------------------

build-release: $(RELEASE_PATH)/$(RESMACK_PERF_EXE)

$(RELEASE_PATH)/$(RESMACK_PERF_EXE): $(RELEASE_PATH)
	cd $(RELEASE_PATH) ; \
	make -j $(nproc)

$(RELEASE_PATH):
	mkdir -p $(RELEASE_PATH) ; \
	cd $(RELEASE_PATH) ; \
	cmake ../../ -DCMAKE_BUILD_TYPE=$(RELEASE_TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=1

# -----------------------------------------------------------------------------
# TEST ------------------------------------------------------------------------
# -----------------------------------------------------------------------------

TEST="*"

.PHONY: test test_resmack clean-test libs-test/googletest build/test/test/test_resmack

test: libs-test/googletest build/test/test/test_resmack

run-test: test
	build/test/test/test_resmack --gtest_filter=$(TEST)

clean-test:
	rm -rf build/test

libs-test/googletest: libs-test/googletest/build/lib/libgtest_main.a

libs-test/googletest/build/lib/libgtest_main.a:
	mkdir -p libs-test
	wget -qO- "https://github.com/google/googletest/archive/release-1.10.0.tar.gz" | tar xvz -C libs-test --transform 's/^googletest-release-1.10.0/googletest/'
	cd libs-test/googletest ; \
		mkdir build ; \
		cd build ; \
		cmake ../ ; \
		make -j $(nproc)

build/test:
	mkdir -p build/test ; \
	cd build/test ; \
	cmake ../../ -DBUILD_TEST=1 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ; \
	cp -u compile_commands.json ../../

build/test/test/test_resmack: build/test
	cd build/test ; \
	make -j $(nproc)
