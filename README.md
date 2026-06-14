# C++ Quickstart

A minimal, batteries-included C++ project skeleton for engineering
takehome interview exercises. Open the folder in VS Code, accept the
"Reopen in Container" prompt, and you'll land in an Ubuntu 24.04
environment with Clang 18, CMake, Ninja, Conan 2, GoogleTest, and the
ASan/UBSan/TSan sanitizers wired up.

You shouldn't need to install anything on your host other than Docker
and the VS Code [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
extension.

## Project layout

```
.
├── .clang-format          # formatting rules (matches Rain's house style)
├── .clang-tidy            # static-analysis rules
├── .devcontainer/         # VS Code dev container definition
├── CMakeLists.txt
├── build.sh               # convenience driver for Conan + CMake + sanitizers
├── cmake/
│   ├── Sanitizers.cmake   # asan/ubsan/tsan plumbing
│   └── Warnings.cmake     # baseline warning set
├── conanfile.py           # dependencies (fmt, GoogleTest)
├── src/
│   ├── main.cpp           # tiny demo program
│   ├── greeter.{h,cpp}    # one-function example library
│   └── CMakeLists.txt
└── tests/
    ├── greeter_test.cpp
    └── CMakeLists.txt
```

## Quick start

```bash
# Build everything (Debug by default) and run the test suite.
./build.sh --test

# Run the demo binary.
./build.sh --run
```

The first run downloads dependencies through Conan, which takes a couple
of minutes. Subsequent builds reuse the Conan cache and ccache volumes
provisioned by the dev container.

## `build.sh` options

| Flag                                | Effect                                           |
| ----------------------------------- | ------------------------------------------------ |
| `--debug` (default)                 | Build type `Debug`                               |
| `--release`                         | Build type `Release`                             |
| `--relwithdebinfo` / `--minsizerel` | Build types with the obvious meanings            |
| `--asan`                            | Enable AddressSanitizer                          |
| `--ubsan`                           | Enable UndefinedBehaviorSanitizer                |
| `--tsan`                            | Enable ThreadSanitizer (excludes asan)           |
| `--no-tests`                        | Skip building the unit tests                     |
| `--test`                            | Run `ctest` after a successful build             |
| `--run`                             | Run the `quickstart` binary after the build      |
| `--clean`                           | Remove `build/` before configuring               |
| `--no-build`                        | Run `conan install` + configure only             |
| `-j N` / `--jobs N`                 | Override parallelism (default: `nproc`)          |
| `-v` / `--verbose`                  | Verbose ninja output                             |

`--asan` and `--ubsan` can be combined. `--tsan` must be run alone.
Sanitized binaries land in their own build dir (e.g. `build/debug/`),
so you can keep multiple configurations side by side by varying
`--debug`/`--release`.

### Sanitizer examples

```bash
# Catch heap and stack errors, plus undefined behavior, in one pass.
./build.sh --clean --debug --asan --ubsan --test

# Hunt for data races.
./build.sh --clean --debug --tsan --test
```

**TSan + seccomp caveat.** ThreadSanitizer calls
`personality(ADDR_NO_RANDOMIZE)` at startup to pin its shadow memory
mapping. Docker's default seccomp profile blocks that argument and
TSan aborts with `tsan_platform_linux.cpp:282 "personality(...) != -1"`.
The dev container ships with `--security-opt seccomp=unconfined` and
`--cap-add SYS_PTRACE` in `runArgs` to allow both the `personality()`
syscall and gdb/lldb attach. If you change these settings or run the
container by hand outside VS Code, pass the same flags to
`docker run`. After editing `devcontainer.json`'s `runArgs` you must
**rebuild the container** ("Dev Containers: Rebuild Container") — those
flags are applied at `docker run` time, not image build time.

## Adding dependencies

Edit `conanfile.py`:

```python
def requirements(self):
    self.requires("fmt/10.2.1")
    self.requires("nlohmann_json/3.11.3")  # <-- new
```

Then re-run `./build.sh --clean`. Use it from CMake via the usual
`find_package(nlohmann_json CONFIG REQUIRED)` / `target_link_libraries`
reference in `src/CMakeLists.txt`.

## Formatting and linting

`.clang-format` and `.clang-tidy` are in the repo root. The dev
container's clangd integration picks them up automatically: format on
save just works, and clang-tidy diagnostics show up inline.

To run them from the command line:

```bash
# Format every tracked C++ source.
find src tests -name '*.cpp' -o -name '*.h' | xargs clang-format -i

# Lint everything (compile_commands.json is symlinked at the repo root
# after the first build).
find src tests -name '*.cpp' | xargs clang-tidy -p .
```


## Without the dev container

If you'd rather work on the host, you need:

- Clang 16+ (Clang 18 is what the container ships)
- CMake 3.24+
- Ninja
- Conan 2.x with a profile that selects Clang and C++20
  (`conan profile detect --force` and then edit `~/.conan2/profiles/default`)

After that `./build.sh` works identically.
