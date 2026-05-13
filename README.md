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
- **RAII helpers**: `scoped_env` and `with_env` for scoped environment variable changes.

## How to Use

### 1. Include the Header

Copy the `environment` folder from the `include` directory into your project, then include the header file:

```cpp
#include "environment/environment.hpp"
```

### 2. Code Examples

#### Get an Environment Variable

Use `env::get` to retrieve an environment variable. It returns an `std::optional` containing the value if the variable exists, otherwise it returns `std::nullopt`.

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
```

#### Unset an Environment Variable

Use `env::unset` to remove an environment variable.

```cpp
env::unset("MY_VAR");
```

#### Get All Environment Variables

Use `env::all` to get a `std::map` containing all environment variables.

```cpp
#include "environment/environment.hpp"
#include <iostream>
#include <map>
#include <string>

int main() {
    std::map<std::string, std::string> all_vars = env::all();
    for (const auto& [key, value] : all_vars) {
        std::cout << key << "=" << value << std::endl;
    }
    return 0;
}
```

#### Get the PATH as a Vector

Use `env::path` to get the `PATH` environment variable as a `std::vector<std::string>` split by the platform-specific delimiter (`:` on Unix, `;` on Windows).

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

Use `env::scoped_env` or `env::with_env` to temporarily change an environment variable and restore it automatically when the scope ends.

```cpp
#include "environment/environment.hpp"
#include <iostream>

int main() {
    env::set("MY_VAR", "original");

    {
        env::scoped_env guard("MY_VAR", "temporary");
        // MY_VAR is now "temporary" inside this scope
        std::cout << "Inside: " << env::get("MY_VAR").value_or("") << std::endl;
    }
    // MY_VAR is restored to "original"
    std::cout << "Outside: " << env::get("MY_VAR").value_or("") << std::endl;

    // Alternatively, use with_env with a callable:
    env::with_env("MY_VAR", "another_temp", []() {
        // MY_VAR is "another_temp" here
        std::cout << "Inside with_env: " << env::get("MY_VAR").value_or("") << std::endl;
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
    auto all_wvars = env::all<std::wstring>();
    for (const auto& [key, value] : all_wvars) {
        std::wcout << key << L"=" << value << std::endl;
    }

    // Get all UTF-8 and UTF-16 environment variables
    auto utf8_vars = env::allutf8();
    auto utf16_vars = env::allutf16();

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

### `std::optional<std::string> get(const std::string& name)`

- Gets the environment variable with the specified name.
- **Parameters**:
  - `name`: The name of the environment variable.
- **Return Value**:
  - An `std::optional<std::string>` containing the value if found.
  - `std::nullopt` if not found.
- **Windows `wchar_t` Overload**: `std::optional<std::wstring> get(const std::wstring& name)`

### `bool set(const std::string& name, const std::string& value, bool overwrite = true)`

- Sets an environment variable.
- **Parameters**:
  - `name`: The name of the environment variable.
  - `value`: The value to set.
  - `overwrite`: If `true` (default), it overwrites an existing value. If `false` and the variable already exists, no action is taken.
- **Return Value**:
  - `true` if the operation was successful.
  - `false` if the operation failed.
- **Windows `wchar_t` Overload**: `bool set(const std::wstring& name, const std::wstring& value, bool overwrite = true)`

### `bool unset(const std::string& name)`

- Removes an environment variable.
- **Parameters**:
  - `name`: The name of the environment variable to remove.
- **Return Value**:
  - `true` if the operation was successful.
  - `false` if the operation failed.
- **Windows `wchar_t` Overload**: `bool unset(const std::wstring& name)`

### `std::map<std::string, std::string> all()`

- Gets a copy of all environment variables in the current environment.
- **Return Value**:
  - A `std::map` where the keys are environment variable names and the values are their corresponding values.
- **Windows `wchar_t` Overload**: `std::map<std::wstring, std::wstring> all<std::wstring>()`

**Note**: On the Windows platform, environment variable keys are case-insensitive. However, the `all` function returns a `std::map`, which is a case-sensitive container. This means if your environment contains variable names that differ only in case (e.g., `Path` and `PATH`), only one of them will be present in the returned map. Therefore, when using the result of `all` on Windows, it is recommended that users handle case-insensitivity themselves when looking up keys (e.g., by converting keys to a consistent case before comparison).

### `std::map<std::string, std::string> allutf8()` (Windows only)

- Gets all environment variables as a UTF-8 `std::map<std::string, std::string>`.
- Equivalent to `all<std::string>()`.

### `std::map<std::wstring, std::wstring> allutf16()` (Windows only)

- Gets all environment variables as a UTF-16 `std::map<std::wstring, std::wstring>`.
- Equivalent to `all<std::wstring>()`.

### `std::vector<std::string> path()`

- Gets the `PATH` environment variable split into a vector of strings.
- Uses `:` as the delimiter on Unix-like systems and `;` on Windows.

### `std::vector<std::wstring> pathw()` (Windows only)

- Gets the `PATH` environment variable split into a vector of wide strings.
- Uses `;` as the delimiter.

### `std::string expand(const std::string& str)` (Windows only)

- Expands environment variable references (e.g., `%VARNAME%`) within the given string.
- **Parameters**:
  - `str`: The string containing environment variable references to expand.
- **Return Value**:
  - The expanded string. Returns the original string on failure.
- **Windows `wchar_t` Overload**: `std::wstring expand(const std::wstring& str)`

### `class scoped_env<CharT>` (RAII helper)

- Temporarily sets (or unsets) an environment variable and automatically restores it when the object goes out of scope.
- **Constructor**: `scoped_env(const string_type& var, const std::optional<string_type>& value)`
  - `var`: The name of the environment variable.
  - `value`: The value to set. If `std::nullopt`, the variable is unset instead.
- When destroyed, restores the original value (or removes the variable if it didn't exist before).

### `void with_env(const std::string& var, std::optional<std::string> value, F&& f)`

- Temporarily sets an environment variable for the duration of a callable.
- **Parameters**:
  - `var`: The name of the environment variable.
  - `value`: The value to set. If `std::nullopt`, the variable is unset.
  - `f`: A callable to execute with the temporary environment variable in effect.
- **Windows `wchar_t` Overload**: `void with_env(const std::wstring& var, std::optional<std::wstring> value, F&& f)`
