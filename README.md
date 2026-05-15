# environment.hpp

[![cmake-multi-platform](https://github.com/shediao/environment.hpp/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/shediao/environment.hpp/actions/workflows/cmake-multi-platform.yml)
[![cmake-multi-platform](https://github.com/shediao/environment.hpp/actions/workflows/msys2.yml/badge.svg)](https://github.com/shediao/environment.hpp/actions/workflows/msys2.yml)

`environment.hpp` is a lightweight, header-only C++ library for convenient, cross-platform manipulation of environment variables.

## Features

- **Header-only**: Just include `include/environment/environment.hpp` in your project to use it.
- **Cross-platform**: Works on Windows, macOS, and Linux.
- **Type-safe**: Uses `std::optional` to handle potentially non-existent environment variables, avoiding the risk of null pointers.
- **Easy to use**: Provides simple functions to get, set, unset, expand, and iterate over environment variables.
- **Wide-character support**: Supports both `char` (`std::string`) and `wchar_t` (`std::wstring`) on Windows.
- **RAII helpers**: `env::with` for scoped environment variable changes (single or multiple variables).

## How to Use

### 1. Include the Header

Copy the `environment` folder from the `include` directory into your project, then include the header file:

```cpp
#include "environment/environment.hpp"
```

### 2. Code Examples

#### Get an Environment Variable

Use `env::get` to retrieve an environment variable. It returns an `std::optional` containing the value if the variable exists, otherwise it returns `std::nullopt`. The function accepts any string-like type (`std::string`, `std::string_view`, `const char*`, etc.).

```cpp
#include "environment/environment.hpp"
#include <iostream>

int main() {
    if (auto path = env::get("PATH")) {
        std::cout << "PATH is: " << *path << std::endl;
    } else {
        std::cout << "PATH environment variable not found." << std::endl;
    }
    return 0;
}
```

#### Set an Environment Variable

Use `env::set` to set an environment variable.

```cpp
// Set a new variable
env::set("MY_VAR", "my_value");

// By default, set will overwrite an existing value.
// To prevent overwriting, set the third argument to false.
env::set("MY_VAR", "another_value", false); // This will not modify "MY_VAR"

// Works with various string types
std::string name = "MY_VAR";
std::string_view value = "hello";
env::set(name, value);
```

#### Unset an Environment Variable

Use `env::unset` to remove an environment variable. Passing `nullptr` is safe and returns `false`.

```cpp
env::unset("MY_VAR");
```

#### Get All Environment Variables

Use `env::all` to get a `std::map<std::string, std::string>` containing all environment variables. On Windows, use `env::allw` to get them as `std::map<std::wstring, std::wstring>`.

```cpp
#include "environment/environment.hpp"
#include <iostream>
#include <map>
#include <string>

int main() {
    auto all_vars = env::all();
    for (const auto& [key, value] : all_vars) {
        std::cout << key << "=" << value << std::endl;
    }
    return 0;
}
```

#### Get the PATH as a Vector

Use `env::path` to get the `PATH` environment variable as a `std::vector<std::string>` split by the platform-specific delimiter (`:` on Unix, `;` on Windows). On Windows, use `env::pathw` to get it as `std::vector<std::wstring>`.

```cpp
#include "environment/environment.hpp"
#include <iostream>

int main() {
    auto paths = env::path();
    for (const auto& p : paths) {
        std::cout << p << std::endl;
    }
    return 0;
}
```

#### Expand Environment Variable References (Windows only)

On Windows, use `env::expand` to expand environment variable references (e.g., `%USERPROFILE%`) within a string.

```cpp
#if defined(_WIN32)
#include "environment/environment.hpp"
#include <iostream>

int main() {
    std::string expanded = env::expand("%USERPROFILE%\\Documents");
    std::cout << expanded << std::endl;
    return 0;
}
#endif
```

#### Scoped Environment Variable Changes (RAII)

Use `env::with` to temporarily change an environment variable and restore it automatically when the callable finishes. You can also change multiple variables at once by passing a `std::map`.

```cpp
#include "environment/environment.hpp"
#include <iostream>

int main() {
    env::set("MY_VAR", "original");

    // Temporarily change a single variable
    env::with("MY_VAR", std::optional<std::string>{"temporary"}, []() {
        // MY_VAR is "temporary" here
        std::cout << "Inside: " << env::get("MY_VAR").value_or("") << std::endl;
    });
    // MY_VAR is restored to "original"
    std::cout << "Outside: " << env::get("MY_VAR").value_or("") << std::endl;

    // Unset a variable temporarily by passing std::nullopt
    env::with("MY_VAR", std::nullopt, []() {
        // MY_VAR is unset here
        std::cout << "Inside (unset): " << env::get("MY_VAR").value_or("(not set)") << std::endl;
    });

    // Change multiple variables at once
    env::with(std::map<std::string, std::optional<std::string>>{
        {"VAR_A", "value_a"},
        {"VAR_B", std::nullopt}  // unset VAR_B
    }, []() {
        // VAR_A = "value_a", VAR_B is unset
    });

    return 0;
}
```

#### Wide-Character Support on Windows

On Windows, you can work with wide-character environment variables.

```cpp
#if defined(_WIN32)
#include "environment/environment.hpp"
#include <iostream>

int main() {
    // Set a wide-character environment variable
    env::set(L"MY_WVAR", L"my_wide_value");

    // Get a wide-character environment variable
    if (auto wvar = env::get(L"MY_WVAR")) {
        std::wcout << L"MY_WVAR is: " << *wvar << std::endl;
    }

    // Get all wide-character environment variables
    auto all_wvars = env::allw();
    for (const auto& [key, value] : all_wvars) {
        std::wcout << key << L"=" << value << std::endl;
    }

    // Get PATH as a wide-character vector
    auto wide_paths = env::pathw();
    for (const auto& p : wide_paths) {
        std::wcout << p << std::endl;
    }

    // Expand wide-character strings
    std::wstring expanded = env::expand(L"%USERPROFILE%\\Documents");
    std::wcout << expanded << std::endl;

    return 0;
}
#endif
```

## API Reference

### `template<typename T> std::optional<to_string_t<T>> get(T&& name)`

- Gets the environment variable with the specified name.
- **Parameters**:
  - `name`: The name of the environment variable. Accepts `std::string`, `std::string_view`, `const char*`, and on Windows also `std::wstring`, `std::wstring_view`, `const wchar_t*`.
- **Return Value**:
  - An `std::optional` containing the value if found.
  - `std::nullopt` if not found.

### `template<typename K, typename V> bool set(K&& name, V&& value, bool overwrite = true)`

- Sets an environment variable.
- **Parameters**:
  - `name`: The name of the environment variable.
  - `value`: The value to set.
  - `overwrite`: If `true` (default), it overwrites an existing value. If `false` and the variable already exists, no action is taken.
- **Return Value**:
  - `true` if the operation was successful.
  - `false` if the operation failed (e.g., empty name).

### `template<typename T> bool unset(T&& name)`

- Removes an environment variable.
- **Parameters**:
  - `name`: The name of the environment variable to remove. Passing `nullptr` returns `false` safely.
- **Return Value**:
  - `true` if the operation was successful.
  - `false` if the operation failed.

### `std::map<std::string, std::string> all()`

- Gets a copy of all environment variables in the current environment.
- **Return Value**:
  - A `std::map<std::string, std::string>` where the keys are environment variable names and the values are their corresponding values.

### `std::map<std::wstring, std::wstring> allw()` (Windows only)

- Gets all environment variables as a `std::map<std::wstring, std::wstring>`.

**Note**: On the Windows platform, environment variable keys are case-insensitive. However, the `all`/`allw` functions return a `std::map`, which is a case-sensitive container. This means if your environment contains variable names that differ only in case (e.g., `Path` and `PATH`), only one of them will be present in the returned map. Therefore, when using the result on Windows, it is recommended that users handle case-insensitivity themselves when looking up keys (e.g., by converting keys to a consistent case before comparison).

### `std::vector<std::string> path()`

- Gets the `PATH` environment variable split into a vector of strings.
- Uses `:` as the delimiter on Unix-like systems and `;` on Windows.

### `std::vector<std::wstring> pathw()` (Windows only)

- Gets the `PATH` environment variable split into a vector of wide strings.
- Uses `;` as the delimiter.

### `template<typename T> to_string_t<T> expand(T&& str)` (Windows only)

- Expands environment variable references (e.g., `%VARNAME%`) within the given string.
- **Parameters**:
  - `str`: The string containing environment variable references to expand.
- **Return Value**:
  - The expanded string. Returns the original string on failure.

### `template<typename T, typename F> void with(T&& var, std::optional<to_string_t<T>> const& value, F&& f)`

- Temporarily sets (or unsets) an environment variable for the duration of a callable. Automatically restores the original value when the callable returns (even if an exception is thrown).
- **Parameters**:
  - `var`: The name of the environment variable.
  - `value`: The value to set. If `std::nullopt`, the variable is unset instead.
  - `f`: A callable to execute with the temporary environment variable in effect.

### `template<typename CharT, typename F> void with(std::map<std::basic_string<CharT>, std::optional<std::basic_string<CharT>>> const& envs, F&& f)`

- Temporarily sets/unset multiple environment variables for the duration of a callable. Automatically restores all original values when the callable returns.
- **Parameters**:
  - `envs`: A map of variable names to optional values. `std::nullopt` means the variable will be unset.
  - `f`: A callable to execute with the temporary environment variables in effect.
