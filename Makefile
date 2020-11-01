.PHONY: clean
clean:
	rm -rf build

all: clean debug release

RESMACK_PERF_EXE=resmack_perf

# -----------------------------------------------------------------------------
# DEBUG -----------------------------------------------------------------------
# -----------------------------------------------------------------------------

.PHONY: debug
debug: build/debug build/debug/ws/resmack_perf/$(RESMACK_PERF_EXE)

.PHONY: clean-debug
clean-debug:
	rm -rf build/debug

run-debug: debug
	build/debug/ws/resmack_perf/$(RESMACK_PERF_EXE)

gdb-debug: debug
	gdb -ex run build/debug/ws/resmack_perf/$(RESMACK_PERF_EXE)

gdb-test: test
	gdb -ex run build/test/ws/libresmack/test/test_libresmack

build/debug:
	mkdir -p build/debug ; \
	cd build/debug ; \
	cmake ../../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ; \
	cp -u compile_commands.json ../../

.PHONY: build/debug/ws/resmack_perf/$(RESMACK_PERF_EXE)
build/debug/ws/resmack_perf/$(RESMACK_PERF_EXE):
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
	rm -rf build/release*

run-release: release
	$(RELEASE_PATH)/ws/resmack_perf/$(RESMACK_PERF_EXE)

release-syms:
	$(MAKE) build-release RELEASE_TYPE=RelWithDebInfo RELEASE_SUFFIX="-syms"

gdb-release: release-syms
	gdb -ex run $(RELEASE_PATH)-syms/ws/resmack_perf/$(RESMACK_PERF_EXE)

run-perf: release-syms
	bash -c "trap 'trap - SIGINT ERR SIGTERM; perf report; exit 1' SIGINT SIGTERM ERR; $(MAKE) perf-release-inner"

perf-release-inner:
	perf record --call-graph dwarf -g $(RELEASE_PATH)-syms/ws/resmack_perf/$(RESMACK_PERF_EXE) || true


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

.PHONY: tests clean-tests test-libresmack-fuzz test-libresmack libs-test/googletest

tests: libs-test/googletest build/test/ws/libresmack/test/test_libresmack test-libresmack-fuzz

run-tests: test
	build/test/ws/libresmack/test/test_libresmack --gtest_filter=$(TEST)
	build/test/ws/libresmack_fuzz/test/test_libresmack_fuzz --gtest_filter=$(TEST)

clean-tests:
	rm -rf build/test

test-libresmack-fuzz:
	cd build/test/ws/libresmack_fuzz/test ; \
	make -j $(nproc)

run-test-libresmack-fuzz: test-libresmack-fuzz
	build/test/ws/libresmack_fuzz/test/test_libresmack_fuzz --gtest_filter=$(TEST)

test-libresmack:
	cd build/test/ws/libresmack/test ; \
	make -j $(nproc)

run-test-libresmack: test-libresmack
	build/test/ws/libresmack/test/test_libresmack --gtest_filter=$(TEST)

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
