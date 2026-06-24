.PHONY: clean
clean:
	rm -rf build

all: clean debug release

OPTS=
RESMACK_EXE=resmack
RESMACK_PERF_EXE=resmack_perf
NPROCS=$(shell nproc)
TEST="*"

export CXX=clang++


# -----------------------------------------------------------------------------
# CMAKE -----------------------------------------------------------------------
# -----------------------------------------------------------------------------

_RESMACK_CMAKE_FILES=$(shell find . -type f \( -name CMakeLists.txt -or -name "*.cmake" \) -and -not -wholename "*/libs/*")

_BUILD_TYPE=""
_BUILD_PATH="/dev/null"

$(_BUILD_PATH)/Makefile: $(_RESMACK_CMAKE_FILES)
	cmake \
		-DCMAKE_BUILD_TYPE=$(_BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
		-Wno-deprecated \
		-B $(_BUILD_PATH) \
		$(OPTS) ;

_cmake_build: $(_BUILD_PATH)/Makefile
	cmake \
		--build $(_BUILD_PATH) \
		-j $(NPROCS) \
		$(OPTS) ;

# -----------------------------------------------------------------------------
# DEBUG -----------------------------------------------------------------------
# -----------------------------------------------------------------------------

DEBUG_TYPE=Debug
DEBUG_PATH=build/debug

$(DEBUG_PATH):
	$(MAKE) \
		_cmake_build \
		_BUILD_TYPE=$(DEBUG_TYPE) \
		_BUILD_PATH=$(DEBUG_PATH)

$(DEBUG_PATH):
	$(MAKE) \
		_cmake_build \
		_BUILD_TYPE=$(DEBUG_TYPE) \
		_BUILD_PATH=$(DEBUG_PATH)

debug: $(DEBUG_PATH)

.PHONY: clean-debug
clean-debug:
	rm -rf $(DEBUG_PATH)

run-perf-debug: debug
	$(DEBUG_PATH)/$(RESMACK_PERF_EXE)

gdb-perf-debug: debug
	gdb -ex run $(DEBUG_PATH)/$(RESMACK_PERF_EXE)

gdb-test-libresmack: test-libresmack
	gdb -ex run --args build/test/test_libresmack --gtest_filter=$(TEST)

gdb-test-libresmack-fuzz: test-libresmack-fuzz
	gdb -ex run --args build/test/test_libresmack_fuzz --gtest_filter=$(TEST)

# -----------------------------------------------------------------------------
# RELEASE ---------------------------------------------------------------------
# -----------------------------------------------------------------------------

RELEASE_TYPE=Release
RELEASE_SUFFIX=
RELEASE_PATH=build/release$(RELEASE_SUFFIX)

.PHONY: \
	release \
	release-syms \
	build-release \
	perf-release-inner \
	$(RELEASE_PATH)/$(RESMACK_PERF_EXE)

release:
	$(MAKE) build-release

install: release
	cd "$(RELEASE_PATH)" ; \
	make install

install-release: release
	cd "$(RELEASE_PATH)" ; \
	make install

install-debug: debug 
	cd "$(DEBUG_PATH)" ; \
	make install

.PHONY: clean-release
clean-release:
	rm -rf build/release*

run-resmack: run-resmack-release

run-resmack-release: release
	$(RELEASE_PATH)/$(RESMACK_EXE)

run-resmack-debug: debug
	$(RELEASE_PATH)/$(RESMACK_EXE)

gdb-resmack: debug
	gdb -ex run $(RELEASE_PATH)/$(RESMACK_EXE)

release-syms:
	$(MAKE) build-release RELEASE_TYPE=RelWithDebInfo RELEASE_SUFFIX="-syms"

# --- PERF EXE ---

run-perf-release: release
	$(RELEASE_PATH)/$(RESMACK_PERF_EXE)

gdb-perf-release: release
	gdb -ex run $(RELEASE_PATH)/$(RESMACK_PERF_EXE)

gdb-perf: release-syms
	gdb -ex run $(RELEASE_PATH)-syms/$(RESMACK_PERF_EXE)

run-perf: release-syms
	bash -c "trap 'trap - SIGINT ERR SIGTERM; perf report; exit 1' SIGINT SIGTERM ERR; $(MAKE) perf-release-inner"

perf-release-inner:
	perf record --call-graph dwarf -g $(RELEASE_PATH)-syms/$(RESMACK_PERF_EXE) || true


# -----------------------------------------------------------------------------

build-release: $(RELEASE_PATH)/$(RESMACK_PERF_EXE)

$(RELEASE_PATH)/$(RESMACK_PERF_EXE): $(RELEASE_PATH)
	cd $(RELEASE_PATH) ; \
	make -j $(NPROCS)

$(RELEASE_PATH):
	mkdir -p $(RELEASE_PATH) ; \
	cd $(RELEASE_PATH) ; \
	cmake ../../ -DCMAKE_BUILD_TYPE=$(RELEASE_TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=1 $(OPTS)

$(RELEASE_PATH)/Makefile:
	cmake \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
		-B $(RELEASE_PATH) $(OPTS) ;

# -----------------------------------------------------------------------------
# TEST ------------------------------------------------------------------------
# -----------------------------------------------------------------------------

.PHONY: tests clean-tests test-libresmack-fuzz test-libresmack libs-test/googletest

tests: libs-test/googletest test-libresmack test-libresmack-fuzz

run-tests: test-libresmack test-libresmack-fuzz
	LD_LIBRARY_PATH=build/test build/test/test_libresmack_fuzz --gtest_filter=$(TEST)
	LD_LIBRARY_PATH=build/test build/test/test_libresmack --gtest_filter=$(TEST)

clean-tests:
	rm -rf build/test

test-libresmack-fuzz: build/test
	cd build/test/ws/libresmack_fuzz/test ; \
	make -j $(NPROCS)

run-test-libresmack-fuzz: test-libresmack-fuzz
	LD_LIBRARY_PATH=build/test build/test/ws/libresmack_fuzz/test/test_libresmack_fuzz --gtest_filter=$(TEST)

test-libresmack: build/test
	cd build/test/ws/libresmack/test ; \
	make -j $(NPROCS)

run-test-libresmack: test-libresmack
	LD_LIBRARY_PATH=build/test build/test/test_libresmack --gtest_filter=$(TEST)

libs-test/googletest: libs-test/googletest/build/lib/libgtest_main.a

libs-test/googletest/build/lib/libgtest_main.a:
	mkdir -p libs-test
	wget -qO- "https://github.com/google/googletest/archive/release-1.10.0.tar.gz" | tar xvz -C libs-test --transform 's/^googletest-release-1.10.0/googletest/'
	cd libs-test/googletest ; \
		mkdir build ; \
		cd build ; \
		cmake ../ $(OPTS) ; \
		make -j $(NPROCS)

build/test:
	mkdir -p build/test ; \
	cd build/test ; \
	cmake ../../ -DBUILD_TEST=1 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 $(OPTS) ; \
	cp -u compile_commands.json ../../

# -----------------------------------------------------------------------------
# RESMACK MAIN LIB ------------------------------------------------------------
# -----------------------------------------------------------------------------


