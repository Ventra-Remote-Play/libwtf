SHELL := /bin/sh

BUILD_DIR ?= build
COVERAGE_BUILD_DIR ?= build-coverage
BUILD_TYPE ?= Debug
WTF_BUILD_TESTS ?= ON
WTF_BUILD_SAMPLES ?= ON
CMAKE ?= cmake
CTEST ?= ctest
UV ?= uv
PYTHON ?= python3
CMAKE_FLAGS ?=
BUILD_DIR_ABS := $(abspath $(BUILD_DIR))

.DEFAULT_GOAL := build

.PHONY: help
help:
	@printf '%s\n' 'libwtf targets:'
	@printf '  %-18s %s\n' 'make' 'Configure and build'
	@printf '  %-18s %s\n' 'make configure' 'Configure CMake'
	@printf '  %-18s %s\n' 'make test' 'Build and run all tests'
	@printf '  %-18s %s\n' 'make unit' 'Run unit/regression tests'
	@printf '  %-18s %s\n' 'make integration' 'Run integration tests'
	@printf '  %-18s %s\n' 'make browser-interop' 'Run Chrome WebTransport interop'
	@printf '  %-18s %s\n' 'make report' 'Write test reports'
	@printf '  %-18s %s\n' 'make coverage' 'Build and write coverage reports'
	@printf '  %-18s %s\n' 'make certs' 'Generate local development certs'
	@printf '  %-18s %s\n' 'make clean' 'Clean configured build outputs'
	@printf '  %-18s %s\n' 'make distclean' 'Remove build directories'

.PHONY: configure
configure:
	$(CMAKE) -S . -B "$(BUILD_DIR)" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DWTF_BUILD_TESTS="$(WTF_BUILD_TESTS)" \
		-DWTF_BUILD_SAMPLES="$(WTF_BUILD_SAMPLES)" \
		$(CMAKE_FLAGS)

.PHONY: build
build: configure
	$(CMAKE) --build "$(BUILD_DIR)" --parallel

.PHONY: test
test: build
	$(CTEST) --test-dir "$(BUILD_DIR)" --output-on-failure

.PHONY: unit
unit: build
	$(CMAKE) --build "$(BUILD_DIR)" --target test_unit

.PHONY: integration
integration: build
	$(CMAKE) --build "$(BUILD_DIR)" --target test_integration

.PHONY: browser-interop
browser-interop: build
	$(CTEST) --test-dir "$(BUILD_DIR)" -R wtf_browser_chrome_interop --output-on-failure -V

.PHONY: report
report: build
	$(CMAKE) --build "$(BUILD_DIR)" --target test_report

.PHONY: coverage-configure
coverage-configure:
	$(CMAKE) -S . -B "$(COVERAGE_BUILD_DIR)" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DWTF_BUILD_TESTS=ON \
		-DWTF_BUILD_SAMPLES=ON \
		-DWTF_ENABLE_COVERAGE=ON \
		$(CMAKE_FLAGS)

.PHONY: coverage
coverage: coverage-configure
	$(CMAKE) --build "$(COVERAGE_BUILD_DIR)" --parallel --target coverage

.PHONY: certs
certs:
	./tools/certgen.sh

.PHONY: browser-deps
browser-deps:
	UV_PROJECT_ENVIRONMENT="$(BUILD_DIR_ABS)/tests/browser_chrome_venv" \
		$(UV) sync --project tests/browser_chrome --frozen

.PHONY: clean
clean:
	@if [ -d "$(BUILD_DIR)" ]; then \
		$(CMAKE) --build "$(BUILD_DIR)" --target clean; \
	fi

.PHONY: distclean
distclean:
	rm -rf "$(BUILD_DIR)" "$(COVERAGE_BUILD_DIR)"
