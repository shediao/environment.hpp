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
- **RAII 辅助工具**: `env::with` 用于作用域内的环境变量临时修改（支持单个或多个变量）。

## 如何使用

### 1. 包含头文件

将 `include` 目录下的 `environment` 文件夹复制到您的项目中，然后包含头文件：

```cpp
#include "environment/environment.hpp"
```

### 2. 代码示例

#### 获取环境变量

使用 `env::get` 来获取一个环境变量。如果变量存在，它会返回一个包含值的 `std::optional`，否则返回 `std::nullopt`。该函数接受任意字符串类型（`std::string`、`std::string_view`、`const char*` 等）。

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

// 支持各种字符串类型
std::string name = "MY_VAR";
std::string_view value = "hello";
env::set(name, value);
```

#### 删除环境变量

使用 `env::unset` 来删除一个环境变量。传入 `nullptr` 是安全的，会返回 `false`。

```cpp
env::unset("MY_VAR");
```

#### 获取所有环境变量

使用 `env::all` 获取一个 `std::map<std::string, std::string>`，包含所有环境变量。在 Windows 上，使用 `env::allw` 获取 `std::map<std::wstring, std::wstring>`。

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

#### 获取 PATH 为向量

使用 `env::path` 将 `PATH` 环境变量按平台分隔符（Unix 上为 `:`，Windows 上为 `;`）拆分为 `std::vector<std::string>`。在 Windows 上，使用 `env::pathw` 获取 `std::vector<std::wstring>`。

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

使用 `env::with` 在可调用对象执行期间临时修改环境变量，执行完毕后自动恢复。也可以通过传入 `std::map` 同时修改多个变量。

```cpp
#include "environment/environment.hpp"
#include <iostream>

int main() {
    env::set("MY_VAR", "original");

    // 临时修改单个变量
    env::with("MY_VAR", std::optional<std::string>{"temporary"}, []() {
        // 此处 MY_VAR 为 "temporary"
        std::cout << "Inside: " << env::get("MY_VAR").value_or("") << std::endl;
    });
    // MY_VAR 已恢复为 "original"
    std::cout << "Outside: " << env::get("MY_VAR").value_or("") << std::endl;

    // 通过传入 std::nullopt 临时删除变量
    env::with("MY_VAR", std::nullopt, []() {
        // 此处 MY_VAR 已被删除
        std::cout << "Inside (unset): " << env::get("MY_VAR").value_or("(not set)") << std::endl;
    });

    // 同时修改多个变量
    env::with(std::map<std::string, std::optional<std::string>>{
        {"VAR_A", "value_a"},
        {"VAR_B", std::nullopt}  // 删除 VAR_B
    }, []() {
        // VAR_A = "value_a", VAR_B 已删除
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
    auto all_wvars = env::allw();
    for (const auto& [key, value] : all_wvars) {
        std::wcout << key << L"=" << value << std::endl;
    }

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

### `template<typename T> std::optional<to_string_t<T>> get(T&& name)`

- 获取指定名称的环境变量。
- **参数**:
  - `name`: 环境变量的名称。接受 `std::string`、`std::string_view`、`const char*`，在 Windows 上还接受 `std::wstring`、`std::wstring_view`、`const wchar_t*`。
- **返回值**:
  - 如果找到，返回包含值的 `std::optional`。
  - 如果未找到，返回 `std::nullopt`。

### `template<typename K, typename V> bool set(K&& name, V&& value, bool overwrite = true)`

- 设置环境变量。
- **参数**:
  - `name`: 环境变量的名称。
  - `value`: 要设置的值。
  - `overwrite`: 如果为 `true`（默认），则覆盖已存在的值。如果为 `false`，且变量已存在，则不进行任何操作。
- **返回值**:
  - 如果操作成功，返回 `true`。
  - 如果操作失败（例如名称为空），返回 `false`。

### `template<typename T> bool unset(T&& name)`

- 删除一个环境变量。
- **参数**:
  - `name`: 要删除的环境变量的名称。传入 `nullptr` 会安全返回 `false`。
- **返回值**:
  - 如果操作成功，返回 `true`。
  - 如果操作失败，返回 `false`。

### `std::map<std::string, std::string> all()`

- 获取当前环境中所有环境变量的副本。
- **返回值**:
  - 一个 `std::map<std::string, std::string>`，其中键是环境变量名称，值是对应的环境变量值。

### `std::map<std::wstring, std::wstring> allw()`（仅限 Windows）

- 获取所有环境变量，以 `std::map<std::wstring, std::wstring>` 返回。

**注意**: 在 Windows 平台上，环境变量的键是大小写不敏感的。然而，`all`/`allw` 函数返回的是一个 `std::map`，它是一个大小写敏感的容器。这意味着，如果您的环境中存在仅大小写不同的变量名（例如 `Path` 和 `PATH`），在返回的 map 中将只会保留其中一个。因此，在 Windows 上使用结果时，建议用户在查找键时自行处理大小写问题（例如，统一转换为大写或小写再进行比较）。

### `std::vector<std::string> path()`

- 获取 `PATH` 环境变量并按分隔符拆分为字符串向量。
- Unix 系统使用 `:` 作为分隔符，Windows 使用 `;`。

### `std::vector<std::wstring> pathw()`（仅限 Windows）

- 获取 `PATH` 环境变量并按分隔符拆分为宽字符串向量。
- 使用 `;` 作为分隔符。

### `template<typename T> to_string_t<T> expand(T&& str)`（仅限 Windows）

- 展开字符串中的环境变量引用（例如 `%VARNAME%`）。
- **参数**:
  - `str`: 包含要展开的环境变量引用的字符串。
- **返回值**:
  - 展开后的字符串。失败时返回原字符串。

### `template<typename T, typename F> void with(T&& var, std::optional<to_string_t<T>> const& value, F&& f)`

- 在可调用对象的执行期间临时设置（或删除）环境变量。执行完毕后自动恢复原值（即使抛出异常也会恢复）。
- **参数**:
  - `var`: 环境变量的名称。
  - `value`: 要设置的值。如果为 `std::nullopt`，则删除该变量。
  - `f`: 在临时环境变量生效期间执行的可调用对象。

### `template<typename CharT, typename F> void with(std::map<std::basic_string<CharT>, std::optional<std::basic_string<CharT>>> const& envs, F&& f)`

- 在可调用对象的执行期间临时设置/删除多个环境变量。执行完毕后自动恢复所有原值。
- **参数**:
  - `envs`: 变量名到可选值的映射。`std::nullopt` 表示删除该变量。
  - `f`: 在临时环境变量生效期间执行的可调用对象。
