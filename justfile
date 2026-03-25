set shell := ['bash', '-ceuo', 'pipefail']

build_dir := "build"

@default: all

# Configure, build, install, and test
all: test

# Configure the CMake build
configure:
    cmake -D CMAKE_INSTALL_PREFIX="$HOME/.local" -B {{ build_dir }} -S .

# Build the project
build: configure
    cmake --build {{ build_dir }} --parallel

# Install the project
install: build
    cmake --install {{ build_dir }}

# Set up the Python test virtualenv
setup-venv:
    ./test/setup_venv.sh

# Run the end-to-end tests
test: install setup-venv
    cd test && .venv/bin/pytest test_e2e.py --echo-log-on-failure

# Remove the build directory
clean:
    rm -rf {{ build_dir }}
