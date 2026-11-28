VERSION ?= 0.0.1
PRESET  ?= gcc-15-debug

CLANG_FORMAT ?= clang-format-18
LINT_PRESET  ?= clang18-libcxx-debug
CLANG_TIDY   ?= clang-tidy-18
CLANG_TIDY_WARNINGS_AS_ERRORS ?= --warnings-as-errors='*'

COVERAGE_PRESET ?= gcc-15-coverage
COVERAGE_XML    ?= coverage.xml
COVERAGE_HTML   ?= htmlcov
GCOVR_FILTERS   ?= src include tests
GCOVR_EXCLUDES  ?= build _deps
GCOVR_OPTS      ?= --gcov-ignore-errors=no_working_dir_found --exclude-unreachable-branches --exclude-throw-branches --print-summary

ASAN_PRESET ?= gcc-15-asan
TSAN_PRESET ?= gcc-15-tsan
TSAN_SETARCH ?= setarch $$(uname -m) -R

CPP_EXTS      := cpp cxx c hpp hxx h
CPP_FIND_ARGS := $(patsubst %,-o -name '*.%',$(CPP_EXTS))
CPP_EXT_REGEX := \.($(subst $(eval) ,|,$(CPP_EXTS)))$$
SRC_EXTS      := cpp cxx c
CPP_FILES        = $(shell find src include tests -type f \( -false $(CPP_FIND_ARGS) \) 2>/dev/null)
STAGED_CPP_FILES = $(shell git diff --cached --name-only --diff-filter=ACMR 2>/dev/null | grep -E '$(CPP_EXT_REGEX)' || true)
CPP_SOURCE_FILES = $(shell find src tests -type f \( -false $(patsubst %,-o -name '*.%',$(SRC_EXTS)) \) 2>/dev/null)
CMAKE_INSTALL_CONFIG := $(if $(filter debug-% %-debug %-coverage %-asan %-tsan,$(PRESET)),Debug,Release)

.PHONY: help format format-staged lint lint-staged configure build test asan tsan coverage clean \
        pr-fast weekly-deep release debug release-gcc debug-clang debug-clang-libcxx \
        debug-msvc release-clang release-clang-libcxx release-msvc all ci-fast ci-deep ci-coverage build-release

help:
	@echo "gitsync CI contract (default PRESET=$(PRESET))"
	@echo "  make lint                    Format and static-analysis checks"
	@echo "  make build PRESET=<name>    Configure, build, and install"
	@echo "  make test PRESET=<name>     Run the CTest suite"
	@echo "  make asan | make tsan       Sanitizer build and tests"
	@echo "  make coverage               Generate coverage.xml and htmlcov/"
	@echo "  make pr-fast                Run lint, build, and tests"
	@echo "  make weekly-deep            Run sanitizers and coverage"

configure:
	cmake --preset $(PRESET)

build: configure
	cmake --build --preset $(PRESET) --parallel
	cmake --install build/$(PRESET) --config $(CMAKE_INSTALL_CONFIG) --prefix dist/$(PRESET)

test:
	ctest --preset $(PRESET) --output-on-failure

format:
	@if [ -n "$(CPP_FILES)" ]; then $(CLANG_FORMAT) -i $(CPP_FILES); fi

format-staged:
	@if [ -n "$(STAGED_CPP_FILES)" ]; then $(CLANG_FORMAT) -i $(STAGED_CPP_FILES); git add $(STAGED_CPP_FILES); else echo "No staged C/C++ files to format"; fi

lint:
	@if ! command -v $(CLANG_FORMAT) >/dev/null 2>&1 || ! command -v $(CLANG_TIDY) >/dev/null 2>&1; then \
		echo "SKIP: lint tools not found"; \
		exit 0; \
	fi; \
	if [ ! -f "build/$(LINT_PRESET)/compile_commands.json" ]; then \
		cmake --preset $(LINT_PRESET); \
	fi; \
	if [ -n "$(CPP_FILES)" ]; then \
		$(CLANG_FORMAT) --dry-run --Werror $(CPP_FILES); \
	fi; \
	if [ -n "$(CPP_SOURCE_FILES)" ]; then \
		$(CLANG_TIDY) -p build/$(LINT_PRESET) $(CLANG_TIDY_WARNINGS_AS_ERRORS) $(CPP_SOURCE_FILES); \
	fi

asan:
	$(MAKE) build PRESET=$(ASAN_PRESET)
	$(MAKE) test PRESET=$(ASAN_PRESET)

tsan:
	$(MAKE) build PRESET=$(TSAN_PRESET)
	@if command -v setarch >/dev/null 2>&1; then $(TSAN_SETARCH) $(MAKE) test PRESET=$(TSAN_PRESET); else $(MAKE) test PRESET=$(TSAN_PRESET); fi

coverage:
	@if ! command -v gcovr >/dev/null 2>&1; then echo "ERROR: gcovr not found"; exit 1; fi
	$(MAKE) build PRESET=$(COVERAGE_PRESET)
	$(MAKE) test PRESET=$(COVERAGE_PRESET)
	@GCOV_CMD=$$(grep '^COVERAGE_GCOV_EXECUTABLE' build/$(COVERAGE_PRESET)/CMakeCache.txt 2>/dev/null | cut -d= -f2); \
	if [ -z "$$GCOV_CMD" ]; then GCOV_CMD=gcov; fi; \
	mkdir -p $(COVERAGE_HTML); \
	PWD_ABS=$$(pwd); \
	FILTER_ARGS="$$(for d in $(GCOVR_FILTERS); do echo -n "--filter $$PWD_ABS/$$d/ "; done)"; \
	EXCLUDE_ARGS="$$(for d in $(GCOVR_EXCLUDES); do echo -n "--exclude '.*/$$d/' "; done)"; \
	gcovr -r . build/$(COVERAGE_PRESET) --gcov-executable $$GCOV_CMD $$FILTER_ARGS $$EXCLUDE_ARGS \
		$(GCOVR_OPTS) --xml-pretty --output $(COVERAGE_XML) --html-details $(COVERAGE_HTML)/index.html

pr-fast:
	$(MAKE) lint
	$(MAKE) build PRESET=gcc-15-debug
	$(MAKE) test PRESET=gcc-15-debug

weekly-deep:
	$(MAKE) asan
	$(MAKE) tsan
	$(MAKE) coverage

release:
	gh workflow run release.yml --ref main

# Existing local aliases.
debug: PRESET=debug-gcc
debug: build
release-gcc: PRESET=release-gcc
release-gcc: build
debug-clang: PRESET=debug-clang
debug-clang: build
release-clang: PRESET=release-clang
release-clang: build
debug-clang-libcxx: PRESET=debug-clang-libcxx
debug-clang-libcxx: build
release-clang-libcxx: PRESET=release-clang-libcxx
release-clang-libcxx: build
debug-msvc: PRESET=debug-msvc
debug-msvc: build
release-msvc: PRESET=release-msvc
release-msvc: build

all: gcc-15-debug gcc-15-release clang18-libcxx-debug clang18-libcxx-release

gcc-15-debug:
	$(MAKE) build PRESET=$@
gcc-15-release:
	$(MAKE) build PRESET=$@
clang18-libcxx-debug:
	$(MAKE) build PRESET=$@
clang18-libcxx-release:
	$(MAKE) build PRESET=$@

ci-fast: pr-fast
ci-deep: weekly-deep
ci-coverage: coverage
build-release:
	$(MAKE) build PRESET=release-gcc

clean:
	rm -rf build dist $(COVERAGE_HTML) $(COVERAGE_XML)
