# environment.hpp

[![cmake-multi-platform](https://github.com/shediao/environment.hpp/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/shediao/environment.hpp/actions/workflows/cmake-multi-platform.yml)
[![cmake-multi-platform](https://github.com/shediao/environment.hpp/actions/workflows/msys2.yml/badge.svg)](https://github.com/shediao/environment.hpp/actions/workflows/msys2.yml)

`environment.hpp` is a lightweight, header-only C++ library for convenient, cross-platform manipulation of environment variables.

## Features

- **Header-only**: Just include `include/environment/environment.hpp` in your project to use it.
- **Cross-platform**: Works on Windows, macOS, and Linux.
- **Type-safe**: Uses `std::optional` to handle potentially non-existent environment variables, avoiding the risk of null pointers.
- **Easy to use**: Provides simple functions to get, set, unset, and iterate over environment variables.
- **Wide-character support**: Supports both `char` (`std::string`) and `wchar_t` (`std::wstring`) on Windows.

## How to Use

### 1. Include the Header

Copy the `environment` folder from the `include` directory into your project, then include the header file:

```cpp
#include "environment/environment.hpp"
```

### 2. Code Examples

#### Get an Environment Variable

Use `environment::getenv` to retrieve an environment variable. It returns an `std::optional` containing the value if the variable exists, otherwise it returns `std::nullopt`.

```cpp
#include "environment/environment.hpp"
#include <iostream>

int main() {
    if (auto path = environment::getenv("PATH")) {
        std::cout << "PATH is: " << *path << std::endl;
    } else {
        std::cout << "PATH environment variable not found." << std::endl;
    }
    return 0;
}
```

#### Set an Environment Variable

Use `environment::setenv` to set an environment variable.

```cpp
// Set a new variable
environment::setenv("MY_VAR", "my_value");

// By default, setenv will overwrite an existing value.
// To prevent overwriting, set the third argument to false.
environment::setenv("MY_VAR", "another_value", false); // This will not modify "MY_VAR"
```

#### Unset an Environment Variable

Use `environment::unsetenv` to remove an environment variable.

```cpp
environment::unsetenv("MY_VAR");
```

#### Get All Environment Variables

Use `environment::allenv` to get a `std::map` containing all environment variables.

```cpp
#include "environment/environment.hpp"
#include <iostream>
#include <map>
#include <string>

int main() {
    std::map<std::string, std::string> all_vars = environment::allenv();
    for (const auto& [key, value] : all_vars) {
        std::cout << key << "=" << value << std::endl;
    }
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
    environment::setenv(L"MY_WVAR", L"my_wide_value");

    // Get a wide-character environment variable
    if (auto wvar = environment::getenv(L"MY_WVAR")) {
        std::wcout << L"MY_WVAR is: " << *wvar << std::endl;
    }

    // Get all wide-character environment variables
    auto all_wvars = environment::allenv<std::wstring>();
    for (const auto& [key, value] : all_wvars) {
        std::wcout << key << L"=" << value << std::endl;
    }

    return 0;
}
#endif
```

## API Reference

### `std::optional<std::string> getenv(const std::string& name)`
- Gets the environment variable with the specified name.
- **Parameters**:
  - `name`: The name of the environment variable.
- **Return Value**:
  - An `std::optional<std::string>` containing the value if found.
  - `std::nullopt` if not found.
- **Windows `wchar_t` Overload**: `std::optional<std::wstring> getenv(const std::wstring& name)`

### `bool setenv(const std::string& name, const std::string& value, bool overwrite = true)`
- Sets an environment variable.
- **Parameters**:
  - `name`: The name of the environment variable.
  - `value`: The value to set.
  - `overwrite`: If `true` (default), it overwrites an existing value. If `false` and the variable already exists, no action is taken.
- **Return Value**:
  - `true` if the operation was successful.
  - `false` if the operation failed.
- **Windows `wchar_t` Overload**: `bool setenv(const std::wstring& name, const std::wstring& value, bool overwrite = true)`

### `bool unsetenv(const std::string& name)`
- Removes an environment variable.
- **Parameters**:
  - `name`: The name of the environment variable to remove.
- **Return Value**:
  - `true` if the operation was successful.
  - `false` if the operation failed.
- **Windows `wchar_t` Overload**: `bool unsetenv(const std::wstring& name)`

### `std::map<std::string, std::string> allenv()`
- Gets a copy of all environment variables in the current environment.
- **Return Value**:
  - A `std::map` where the keys are environment variable names and the values are their corresponding values.
- **Windows `wchar_t` Overload**: `std::map<std::wstring, std::wstring> allenv<std::wstring>()`

**Note**: On the Windows platform, environment variable keys are case-insensitive. However, the `allenv` function returns a `std::map`, which is a case-sensitive container. This means if your environment contains variable names that differ only in case (e.g., `Path` and `PATH`), only one of them will be present in the returned map. Therefore, when using the result of `allenv` on Windows, it is recommended that users handle case-insensitivity themselves when looking up keys (e.g., by converting keys to a consistent case before comparison).
