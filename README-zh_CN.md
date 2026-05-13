# environment.hpp

[![cmake-multi-platform](https://github.com/shediao/environment.hpp/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/shediao/environment.hpp/actions/workflows/cmake-multi-platform.yml)
[![cmake-multi-platform](https://github.com/shediao/environment.hpp/actions/workflows/msys2.yml/badge.svg)](https://github.com/shediao/environment.hpp/actions/workflows/msys2.yml)

`environment.hpp` 是一个轻量级的、仅包含头文件的 C++ 库，用于方便、跨平台地操作环境变量。

## 特性

- **仅头文件**: 只需将 `include/environment/environment.hpp` 包含到您的项目中即可使用。
- **跨平台**: 在 Windows、macOS 和 Linux 上均可使用。
- **类型安全**: 使用 `std::optional` 来处理可能不存在的环境变量，避免了空指针的风险。
- **易于使用**: 提供简洁的函数来获取、设置、删除、展开和遍历环境变量。
- **宽字符支持**: 在 Windows 上，同时支持 `char` (`std::string`) 和 `wchar_t` (`std::wstring`)。
- **RAII 辅助工具**: `scoped_env` 和 `with_env` 用于作用域内的环境变量临时修改。

## 如何使用

### 1. 包含头文件

将 `include` 目录下的 `environment` 文件夹复制到您的项目中，然后包含头文件：

```cpp
#include "environment/environment.hpp"
```

### 2. 代码示例

#### 获取环境变量

使用 `env::get` 来获取一个环境变量。如果变量存在，它会返回一个包含值的 `std::optional`，否则返回 `std::nullopt`。

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

#### 设置环境变量

使用 `env::set` 来设置一个环境变量。

```cpp
// 设置一个新变量
env::set("MY_VAR", "my_value");

// 默认情况下，set 会覆盖已存在的值。
// 若不希望覆盖，可以将第三个参数设置为 false。
env::set("MY_VAR", "another_value", false); // 不会修改 "MY_VAR"
```

#### 删除环境变量

使用 `env::unset` 来删除一个环境变量。

```cpp
env::unset("MY_VAR");
```

#### 获取所有环境变量

使用 `env::all` 来获取一个包含所有环境变量的 `std::map`。

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

#### 获取 PATH 为向量

使用 `env::path` 将 `PATH` 环境变量按平台分隔符（Unix 上为 `:`，Windows 上为 `;`）拆分为 `std::vector<std::string>`。

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

#### 展开环境变量引用（仅限 Windows）

在 Windows 上，使用 `env::expand` 展开字符串中的环境变量引用（例如 `%USERPROFILE%`）。

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

#### 作用域环境变量修改（RAII）

使用 `env::scoped_env` 或 `env::with_env` 临时修改环境变量，并在作用域结束时自动恢复。

```cpp
#include "environment/environment.hpp"
#include <iostream>

int main() {
    env::set("MY_VAR", "original");

    {
        env::scoped_env guard("MY_VAR", "temporary");
        // 在此作用域内 MY_VAR 为 "temporary"
        std::cout << "Inside: " << env::get("MY_VAR").value_or("") << std::endl;
    }
    // MY_VAR 已恢复为 "original"
    std::cout << "Outside: " << env::get("MY_VAR").value_or("") << std::endl;

    // 也可以使用 with_env 配合可调用对象：
    env::with_env("MY_VAR", "another_temp", []() {
        // 此处 MY_VAR 为 "another_temp"
        std::cout << "Inside with_env: " << env::get("MY_VAR").value_or("") << std::endl;
    });

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
    env::set(L"MY_WVAR", L"my_wide_value");

    // 获取宽字符环境变量
    if (auto wvar = env::get(L"MY_WVAR")) {
        std::wcout << L"MY_WVAR is: " << *wvar << std::endl;
    }

    // 获取所有宽字符环境变量
    auto all_wvars = env::all<std::wstring>();
    for (const auto& [key, value] : all_wvars) {
        std::wcout << key << L"=" << value << std::endl;
    }

    // 获取所有 UTF-8 和 UTF-16 环境变量
    auto utf8_vars = env::allutf8();
    auto utf16_vars = env::allutf16();

    // 获取 PATH 宽字符向量
    auto wide_paths = env::pathw();
    for (const auto& p : wide_paths) {
        std::wcout << p << std::endl;
    }

    // 展开宽字符字符串
    std::wstring expanded = env::expand(L"%USERPROFILE%\\Documents");
    std::wcout << expanded << std::endl;

    return 0;
}
#endif
```

## API 参考

### `std::optional<std::string> get(const std::string& name)`

- 获取指定名称的环境变量。
- **参数**:
  - `name`: 环境变量的名称。
- **返回值**:
  - 如果找到，返回包含值的 `std::optional<std::string>`。
  - 如果未找到，返回 `std::nullopt`。
- **Windows `wchar_t` 重载**: `std::optional<std::wstring> get(const std::wstring& name)`

### `bool set(const std::string& name, const std::string& value, bool overwrite = true)`

- 设置环境变量。
- **参数**:
  - `name`: 环境变量的名称。
  - `value`: 要设置的值。
  - `overwrite`: 如果为 `true`（默认），则覆盖已存在的值。如果为 `false`，且变量已存在，则不进行任何操作。
- **返回值**:
  - 如果操作成功，返回 `true`。
  - 如果操作失败，返回 `false`。
- **Windows `wchar_t` 重载**: `bool set(const std::wstring& name, const std::wstring& value, bool overwrite = true)`

### `bool unset(const std::string& name)`

- 删除一个环境变量。
- **参数**:
  - `name`: 要删除的环境变量的名称。
- **返回值**:
  - 如果操作成功，返回 `true`。
  - 如果操作失败，返回 `false`。
- **Windows `wchar_t` 重载**: `bool unset(const std::wstring& name)`

### `std::map<std::string, std::string> all()`

- 获取当前环境中所有环境变量的副本。
- **返回值**:
  - 一个 `std::map`，其中键是环境变量名称，值是对应的环境变量值。
- **Windows `wchar_t` 重载**: `std::map<std::wstring, std::wstring> all<std::wstring>()`

**注意**: 在 Windows 平台上，环境变量的键是大小写不敏感的。然而，`all` 函数返回的是一个 `std::map`，它是一个大小写敏感的容器。这意味着，如果您的环境中存在仅大小写不同的变量名（例如 `Path` 和 `PATH`），在返回的 map 中将只会保留其中一个。因此，在 Windows 上使用 `all` 的结果时，建议用户在查找键时自行处理大小写问题（例如，统一转换为大写或小写再进行比较）。

### `std::map<std::string, std::string> allutf8()`（仅限 Windows）

- 获取所有环境变量，以 UTF-8 编码的 `std::map<std::string, std::string>` 返回。
- 等价于 `all<std::string>()`。

### `std::map<std::wstring, std::wstring> allutf16()`（仅限 Windows）

- 获取所有环境变量，以 UTF-16 编码的 `std::map<std::wstring, std::wstring>` 返回。
- 等价于 `all<std::wstring>()`。

### `std::vector<std::string> path()`

- 获取 `PATH` 环境变量并按分隔符拆分为字符串向量。
- Unix 系统使用 `:` 作为分隔符，Windows 使用 `;`。

### `std::vector<std::wstring> pathw()`（仅限 Windows）

- 获取 `PATH` 环境变量并按分隔符拆分为宽字符串向量。
- 使用 `;` 作为分隔符。

### `std::string expand(const std::string& str)`（仅限 Windows）

- 展开字符串中的环境变量引用（例如 `%VARNAME%`）。
- **参数**:
  - `str`: 包含要展开的环境变量引用的字符串。
- **返回值**:
  - 展开后的字符串。失败时返回原字符串。
- **Windows `wchar_t` 重载**: `std::wstring expand(const std::wstring& str)`

### `class scoped_env<CharT>`（RAII 辅助类）

- 临时设置（或删除）一个环境变量，并在对象离开作用域时自动恢复原值。
- **构造函数**: `scoped_env(const string_type& var, const std::optional<string_type>& value)`
  - `var`: 环境变量的名称。
  - `value`: 要设置的值。如果为 `std::nullopt`，则删除该变量。
- 析构时恢复原始值（如果变量原本不存在则将其删除）。

### `void with_env(const std::string& var, std::optional<std::string> value, F&& f)`

- 在可调用对象的执行期间临时设置环境变量。
- **参数**:
  - `var`: 环境变量的名称。
  - `value`: 要设置的值。如果为 `std::nullopt`，则删除该变量。
  - `f`: 在临时环境变量生效期间执行的可调用对象。
- **Windows `wchar_t` 重载**: `void with_env(const std::wstring& var, std::optional<std::wstring> value, F&& f)`
