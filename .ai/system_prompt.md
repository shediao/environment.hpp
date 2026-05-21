This content provides guidance to Ai CommandLine users when working with code in this repository.

## Build & Test Commands

```bash
# Configure (from repo root)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
# Or with a specific compiler:
cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test
ctest --test-dir build -R <test_name> --output-on-failure
# or directly:
./build/tests/env_test
```

The project requires **C++20**. Tests use GoogleTest and `subprocess.hpp` (both fetched automatically via CMake `FetchContent`).

## Cross-Compilation

> **Note:** Cross-compilation environment is currently only available on macOS and Linux platforms.

When directories matching `build/{platform}-{arch}` exist (e.g., `build/darwin-arm64`, `build/linux-x86_64`, `build/mingw64-x86_64`, `build/windows-arm64`), the cross-compilation environment is already configured. Build directly with:

```bash
cmake --build build/{platform}-{arch}
```

Supported build directories:

| Directory              | Target                 |
| ---------------------- | ---------------------- |
| `build/darwin-arm64`   | macOS ARM64            |
| `build/darwin-x86_64`  | macOS x86_64           |
| `build/linux-arm64`    | Linux ARM64            |
| `build/linux-x86_64`   | Linux x86_64           |
| `build/mingw64-x86_64` | Windows x86_64 (MinGW) |
| `build/windows-arm64`  | Windows ARM64 (MSVC)   |
| `build/windows-x86_64` | Windows x86_64 (MSVC)  |

When source files are added/removed or CMake options change (making it necessary to re-run CMake configuration), simply `touch CMakeLists.txt` and the next `cmake --build` will automatically re-generate the build system:

```bash
touch CMakeLists.txt
cmake --build build/{platform}-{arch}
```

## CI

If the remote repository URL is `http://github.com/*` or `git@github.com:*`, then GitHub Actions (configuration files located at `.github/workflows/*.yml`) is used as CI.

Browse and manage GitHub Actions via the `gh` command:

```bash
# View recent workflow runs
gh run list

# View details of a specific run
gh run view <run-id>

# View logs of a specific run
gh run view <run-id> --log

# Manually trigger a workflow
gh workflow run <workflow-name>

# List all workflows
gh workflow list

# View workflow file contents
gh workflow view <workflow-name>
```

**When the user wants to resolve GitHub Actions failures, they should first use `gh` commands to get logs and analyze the problem**, rather than blindly guessing the cause. How to get logs:

```bash
# Get logs of the latest run (usually the failed run)
gh run list --limit 1 --json databaseId -q '.[].databaseId' | xargs gh run view --log

# Get logs of a specific run-id (including failed steps)
gh run view <run-id> --log

# Get logs of the run corresponding to a specific commit
gh run list --commit <commit-sha> --limit 1 --json databaseId -q '.[].databaseId' | xargs gh run view --log

# Only view failed runs
gh run list --status failure --limit 5

# View logs of a failed job in a run (if the run has multiple jobs)
gh run view <run-id> --log --job <job-id>
```

**When fetching logs with `gh` commands, always prefer getting only the key information rather than the full logs unless absolutely necessary.** For example:

- Use `gh run view <run-id> --log --failed` to get only failed step logs (if supported).
- Pipe through `grep` to filter for error messages, failed test names, or compilation errors: `gh run view <run-id> --log 2>&1 | grep -E '(error:|FAILED|failure)'`.
- For test failures, look for the specific test output rather than the entire build log.
- Only fetch full logs when the filtered output does not provide enough context to diagnose the issue.

After getting the logs, identify the specific failure cause based on the error messages in the logs (compilation errors, test failures, environment issues, etc.), then make targeted fixes.

**Fix Verification**: If the failure occurs on a platform different from the current machine (e.g., currently on macOS ARM64, but the failure is on Linux x86_64), and a corresponding cross-compilation environment `build/{platform}-{arch}/` exists locally, after fixing, you **must** verify the build through that cross-compilation environment to ensure the fix also passes on the target platform:

```bash
# For example: fixed a compilation error on Linux x86_64, with build/linux-x86_64/ available locally
cmake --build build/linux-x86_64
```

If the corresponding cross-compilation environment does not exist locally, just push the fix directly and let CI verify.

## Architecture

This is a **header-only** C++20 library. The single public header is `include/environment/environment.hpp`. There are no `.cpp` files — all implementation lives in the header.

### Namespace layering

- **`env`** — Public API: `get`, `set`, `unset`, `all`, `allw`, `path`, `pathw`, `expand`, `search_path`, `with`.
- **`env::detail`** — Internal helpers: `string_like_type` concept, `get_char_type` trait, `to_string_view_t`/`to_string_t` aliases, `split`, `scoped_env` RAII class, and platform-specific `get`/`set`/`unset`/`all`/`expand`.

### Platform split (`#if defined(_WIN32)`)

- **Windows**: All operations go through the Win32 **Wide** API (`GetEnvironmentVariableW`, `SetEnvironmentVariableW`, `GetEnvironmentStringsW`, `ExpandEnvironmentStringsW`, `SearchPathW`). The header includes `windows.h`. UTF-8 ↔ UTF-16 conversion helpers (`to_wstring`/`to_string`) bridge the narrow-char public API with the wide OS API. Wide-char overloads (`allw`, `pathw`, `expand` with `wstring`) are exposed only on Windows.
- **Unix (macOS/Linux)**: Uses POSIX `getenv`/`setenv`/`unsetenv` and iterates the external `environ` pointer for `all()`.

### Template/concepts design

The public templates (`get`, `set`, `unset`, `expand`) accept any type matching the `detail::string_like_type` concept: `std::string`, `std::string_view`, `const char*`, `char*`, and on Windows also the `wchar_t` equivalents. The `to_string_view_t<T>` and `to_string_t<T>` aliases normalize any accepted type to the appropriate `basic_string_view`/`basic_string` for internal use.

### Key types

- `detail::scoped_env<CharT>` — RAII guard that saves original values on construction and restores them on destruction (used by `env::with`). Supports both single-variable and multi-variable (map-based) scoped changes, including `std::nullopt` to unset a variable for the scope.
- `env::search_path` — Resolves executables via `PATH` (plus `PATHEXT` probing on Windows). On Unix, it uses `stat` + `access(X_OK)` and rejects directories. On Windows, it delegates to `SearchPathW`.

### Code style

Clang-format config uses `BasedOnStyle: Google` with `InsertBraces: true` and `Cpp11` standard. Format on save is expected (see `.vim/coc-settings.json`).

## Testing

Tests are in `tests/`. The CMake function `add_environment_test` creates a test executable linked against GoogleTest and `subprocess.hpp`, compiled with `-Wall -Wextra -Werror` (GCC/Clang) or `/W4` (MSVC). On Windows, a second `_ansi` executable variant is produced.

Tests use `subprocess.hpp` to spawn `bash -c` or `cmd.exe` to verify that environment variable changes are visible at the OS level. Test environment variable keys are generated at compile time with the `MK_ENV()` macro which incorporates `__LINE__` to avoid collisions.
