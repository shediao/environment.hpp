# environment.hpp

[![cmake-multi-platform](https://github.com/shediao/environment.hpp/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/shediao/environment.hpp/actions/workflows/cmake-multi-platform.yml)

`environment.hpp` 是一个轻量级的、仅包含头文件的 C++ 库，用于方便、跨平台地操作环境变量。

## 特性

- **仅头文件**: 只需将 `include/environment/environment.hpp` 包含到您的项目中即可使用。
- **跨平台**: 在 Windows、macOS 和 Linux 上均可使用。
- **类型安全**: 使用 `std::optional` 来处理可能不存在的环境变量，避免了空指针的风险。
- **易于使用**: 提供简洁的函数来获取、设置、删除和遍历环境变量。
- **宽字符支持**: 在 Windows 上，同时支持 `char` (`std::string`) 和 `wchar_t` (`std::wstring`)。

## 如何使用

### 1. 包含头文件

将 `include` 目录下的 `environment` 文件夹复制到您的项目中，然后包含头文件：

```cpp
#include "environment/environment.hpp"
```

### 2. 代码示例

#### 获取环境变量

使用 `environment::getenv` 来获取一个环境变量。如果变量存在，它会返回一个包含值的 `std::optional`，否则返回 `std::nullopt`。

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

#### 设置环境变量

使用 `environment::setenv` 来设置一个环境变量。

```cpp
// 设置一个新变量
environment::setenv("MY_VAR", "my_value");

// 默认情况下，setenv 会覆盖已存在的值。
// 若不希望覆盖，可以将第三个参数设置为 false。
environment::setenv("MY_VAR", "another_value", false); // 不会修改 "MY_VAR"
```

#### 删除环境变量

使用 `environment::unsetenv` 来删除一个环境变量。

```cpp
environment::unsetenv("MY_VAR");
```

#### 获取所有环境变量

使用 `environment::allenv` 来获取一个包含所有环境变量的 `std::map`。

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

#### Windows 上的宽字符支持

在 Windows 上，您可以操作宽字符环境变量。

```cpp
#if defined(_WIN32)
#include "environment/environment.hpp"
#include <iostream>

int main() {
    // 设置宽字符环境变量
    environment::setenv(L"MY_WVAR", L"my_wide_value");

    // 获取宽字符环境变量
    if (auto wvar = environment::getenv(L"MY_WVAR")) {
        std::wcout << L"MY_WVAR is: " << *wvar << std::endl;
    }

    // 获取所有宽字符环境变量
    auto all_wvars = environment::allenv<std::wstring>();
    for (const auto& [key, value] : all_wvars) {
        std::wcout << key << L"=" << value << std::endl;
    }

    return 0;
}
#endif
```

## API 参考

### `std::optional<std::string> getenv(const std::string& name)`
- 获取指定名称的环境变量。
- **参数**:
  - `name`: 环境变量的名称。
- **返回值**:
  - 如果找到，返回包含值的 `std::optional<std::string>`。
  - 如果未找到，返回 `std::nullopt`。
- **Windows `wchar_t` 重载**: `std::optional<std::wstring> getenv(const std::wstring& name)`

### `bool setenv(const std::string& name, const std::string& value, bool overwrite = true)`
- 设置环境变量。
- **参数**:
  - `name`: 环境变量的名称。
  - `value`: 要设置的值。
  - `overwrite`: 如果为 `true`（默认），则覆盖已存在的值。如果为 `false`，且变量已存在，则不进行任何操作。
- **返回值**:
  - 如果操作成功，返回 `true`。
  - 如果操作失败，返回 `false`。
- **Windows `wchar_t` 重载**: `bool setenv(const std::wstring& name, const std::wstring& value, bool overwrite = true)`

### `bool unsetenv(const std::string& name)`
- 删除一个环境变量。
- **参数**:
  - `name`: 要删除的环境变量的名称。
- **返回值**:
  - 如果操作成功，返回 `true`。
  - 如果操作失败，返回 `false`。
- **Windows `wchar_t` 重载**: `bool unsetenv(const std::wstring& name)`

### `std::map<std::string, std::string> allenv()`
- 获取当前环境中所有环境变量的副本。
- **返回值**:
  - 一个 `std::map`，其中键是环境变量名称，值是对应的环境变量值。
- **Windows `wchar_t` 重载**: `std::map<std::wstring, std::wstring> allenv<std::wstring>()`

**注意**: 在 Windows 平台上，环境变量的键是大小写不敏感的。然而，`allenv` 函数返回的是一个 `std::map`，它是一个大小写敏感的容器。这意味着，如果您的环境中存在仅大小写不同的变量名（例如 `Path` 和 `PATH`），在返回的 map 中将只会保留其中一个。因此，在 Windows 上使用 `allenv` 的结果时，建议用户在查找键时自行处理大小写问题（例如，统一转换为大写或小写再进行比较）。
