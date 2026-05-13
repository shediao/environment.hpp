//
//  environment.hpp — A lightweight, header-only C++ library for
//                    cross-platform manipulation of environment variables.
//
//  Author  : shediao.xsd <xushediao1987@163.com>
//  Repo    : https://github.com/shediao/environment.hpp.git
//  License : MIT
//
//  Copyright (c) 2024-2026 shediao.xsd
//
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
//  sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//  IN THE SOFTWARE.
//
//  SPDX-License-Identifier: MIT

#ifndef __ENVIRONMENT_ENVIRONMENT_HPP__
#define __ENVIRONMENT_ENVIRONMENT_HPP__

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#if defined(_WIN32)
#include <windows.h>

#include <algorithm>
#include <locale>
#else
extern "C" {
extern char** environ;
}
#endif

namespace env {

namespace detail {

template <typename T>
concept string_like_type =
#if defined(_WIN32)
    std::same_as<wchar_t*, std::decay_t<T>> ||
    std::same_as<const wchar_t*, std::decay_t<T>> ||
    std::same_as<std::wstring, std::decay_t<T>> ||
    std::same_as<std::wstring_view, std::decay_t<T>> ||
#endif
    std::same_as<char*, std::decay_t<T>> ||
    std::same_as<const char*, std::decay_t<T>> ||
    std::same_as<std::string, std::decay_t<T>> ||
    std::same_as<std::string_view, std::decay_t<T>>;

template <typename T>
concept cstr_like_type =
#if defined(_WIN32)
    std::same_as<wchar_t*, std::decay_t<T>> ||
    std::same_as<const wchar_t*, std::decay_t<T>> ||
#endif
    std::same_as<char*, std::decay_t<T>> ||
    std::same_as<const char*, std::decay_t<T>>;

template <typename T>
struct get_char_type;

template <typename CharT>
struct get_char_type<std::basic_string<CharT>> {
  using type = CharT;
};
template <typename CharT>
struct get_char_type<std::basic_string_view<CharT>> {
  using type = CharT;
};
template <typename CharT>
struct get_char_type<CharT*> {
  using type = std::remove_cv_t<CharT>;
};

template <typename T>
using get_char_type_t = typename get_char_type<T>::type;

template <typename CharT>
std::vector<std::basic_string<CharT>> split(std::basic_string<CharT> const& str,
                                            CharT del) {
  std::vector<std::basic_string<CharT>> ret;
  if (str.empty()) {
    return ret;
  }

  std::size_t start = 0;
  std::size_t pos = 0;

  while ((pos = str.find(del, start)) != std::basic_string<CharT>::npos) {
    if (pos != start) {
      ret.emplace_back(str.substr(start, pos - start));
    }
    start = pos + 1;
  }
  // Don't forget the last segment after the final delimiter
  if (start != str.size()) {
    ret.emplace_back(str.substr(start));
  }

  return ret;
}
#if defined(_WIN32)
// Helper function to convert a UTF-8 std::string to a UTF-16 std::wstring
inline std::wstring to_wstring(const std::string_view utf8str,
                               const UINT from_codepage = CP_UTF8) {
  if (utf8str.empty()) {
    return {};
  }
  int size_needed = MultiByteToWideChar(from_codepage, 0, utf8str.data(),
                                        (int)utf8str.size(), NULL, 0);
  if (size_needed <= 0) {
    // Consider throwing an exception for conversion errors
    return {};
  }
  std::wstring utf16str(size_needed, 0);
  MultiByteToWideChar(from_codepage, 0, utf8str.data(), (int)utf8str.size(),
                      &utf16str[0], size_needed);
  return utf16str;
}

// Helper function to convert a UTF-16 std::wstring to a UTF-8 std::string
inline std::string to_string(const std::wstring_view utf16str,
                             const UINT to_codepage = CP_UTF8) {
  if (utf16str.empty()) {
    return {};
  }
  int size_needed =
      WideCharToMultiByte(to_codepage, 0, utf16str.data(), (int)utf16str.size(),
                          NULL, 0, NULL, NULL);
  if (size_needed <= 0) {
    // Consider throwing an exception for conversion errors
    return {};
  }
  std::string utf8str(size_needed, 0);
  WideCharToMultiByte(to_codepage, 0, utf16str.data(), (int)utf16str.size(),
                      &utf8str[0], size_needed, NULL, NULL);
  return utf8str;
}

template <typename CharT>
std::optional<std::basic_string<CharT>> get(
    std::basic_string_view<CharT> name) {
  if constexpr (std::is_same_v<char, CharT>) {
    std::wstring const wname = detail::to_wstring(name);
    DWORD const size =
        GetEnvironmentVariableW(wname.c_str(), nullptr, 0);
    if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return std::nullopt;
    }
    std::wstring ret(size, L'\0');
    DWORD const copied =
        GetEnvironmentVariableW(wname.c_str(), ret.data(), size);
    ret.resize(copied);
    return detail::to_string(ret);
  } else {
    DWORD const size =
        GetEnvironmentVariableW(name.data(), nullptr, 0);
    if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return std::nullopt;
    }
    std::wstring ret(size, L'\0');
    DWORD const copied =
        GetEnvironmentVariableW(name.data(), ret.data(), size);
    ret.resize(copied);
    return ret;
  }
}

template <typename CharT>
bool set(std::basic_string_view<CharT> const& name,
         std::basic_string_view<CharT> const& value, bool overwrite = true) {
  if (name.empty()) {
    return false;
  }
  if (!overwrite) {
    DWORD size = 0;
    if constexpr (std::is_same_v<char, CharT>) {
      size =
          GetEnvironmentVariableW(detail::to_wstring(name).data(), nullptr, 0);
    } else {
      size = GetEnvironmentVariableW(name.data(), nullptr, 0);
    }
    if (size != 0 || GetLastError() != ERROR_ENVVAR_NOT_FOUND) {
      return true;
    }
  }
  if constexpr (std::is_same_v<char, CharT>) {
    return SetEnvironmentVariableW(detail::to_wstring(name).data(),
                                   detail::to_wstring(value).data());
  } else {
    return SetEnvironmentVariableW(name.data(), value.data());
  }
}

template <typename CharT>
bool unset(std::basic_string_view<CharT> const& name) {
  if (name.empty()) {
    return false;
  }
  if constexpr (std::is_same_v<char, CharT>) {
    return SetEnvironmentVariableW(detail::to_wstring(name).data(), nullptr);
  } else {
    return SetEnvironmentVariableW(name.data(), nullptr);
  }
}

template <typename CharT>
std::basic_string<CharT> expand(std::basic_string_view<CharT> str) {
  if (str.empty()) {
    return {};
  }
  DWORD size = 0;
  if constexpr (std::is_same_v<char, CharT>) {
    size =
        ExpandEnvironmentStringsW(detail::to_wstring(str).data(), nullptr, 0);
    if (size == 0) {
      return std::basic_string<CharT>(str);
    }
    std::wstring ret(static_cast<size_t>(size), L'\0');
    ExpandEnvironmentStringsW(detail::to_wstring(str).data(), ret.data(), size);
    ret.pop_back();
    return detail::to_string(ret);
  } else {
    size = ExpandEnvironmentStringsW(str.data(), nullptr, 0);
    if (size == 0) {
      return std::basic_string<CharT>(str);
    }
    std::wstring ret(static_cast<size_t>(size), L'\0');
    ExpandEnvironmentStringsW(str.data(), ret.data(), size);
    ret.pop_back();
    return ret;
  }
}
template <typename CharT>
inline void all(
    std::map<std::basic_string<CharT>, std::basic_string<CharT>>& envs) {
  envs.clear();
  wchar_t* envBlock = GetEnvironmentStringsW();
  if (envBlock == nullptr) {
    return;
  }

  wchar_t* currentEnv = envBlock;
  while (*currentEnv != L'\0') {
    std::wstring_view envString(currentEnv);
    auto pos = envString.find(L'=');
    if (pos == 0) {
      pos = envString.find(L'=', 1);
    }
    if (pos != std::wstring_view::npos) {
      auto key = std::wstring(envString.substr(0, pos));
      auto value = std::wstring(envString.substr(pos + 1));
      std::transform(key.begin(), key.end(), key.begin(),
                     [](wchar_t c) { return std::toupper(c, std::locale()); });
      if constexpr (std::is_same_v<char, CharT>) {
        envs[to_string(std::move(key))] = to_string(std::move(value));
      } else {
        envs[std::move(key)] = std::move(value);
      }
    }
    currentEnv +=
        envString.length() + 1;  // Move to the next environment variable
  }

  FreeEnvironmentStringsW(envBlock);
}
#else   // !_WIN32
inline std::optional<std::string> get(std::string_view name) {
  auto* env = ::getenv(name.data());
  if (env) {
    return std::string(env);
  }
  return std::nullopt;
}

inline bool set(std::string_view name, std::string_view value,
                bool overwrite = true) {
  if (name.empty()) {
    return false;
  }
  if (::setenv(name.data(), value.data(), overwrite ? 1 : 0) != 0) {
    return false;
  }
  return true;
}

inline bool unset(std::string_view name) {
  if (name.empty()) {
    return false;
  }
  return ::unsetenv(name.data()) == 0;
}
inline void all(std::map<std::string, std::string>& envs) {
  envs.clear();
  if (environ == nullptr) {
    return;
  }

  for (char** env = environ; *env != nullptr; ++env) {
    std::string_view envString(*env);
    auto pos = envString.find('=');
    if (pos != std::string::npos) {
      std::string_view key = envString.substr(0, pos);
      std::string_view value = envString.substr(pos + 1);
      envs[std::string(key)] = std::string(value);
    }
  }
}
#endif  // _WIN32
}  // namespace detail

template <detail::string_like_type T>
inline std::optional<
    std::basic_string<detail::get_char_type_t<std::decay_t<T>>>>
get(T&& name) {
  using CharT = detail::get_char_type_t<std::decay_t<T>>;
  auto name_view = std::basic_string_view<CharT>(std::forward<T>(name));
  return detail::get(name_view);
}

template <detail::string_like_type K, detail::string_like_type V>
inline bool set(K&& name, V&& value, bool overwrite = true) {
  auto name_view =
      std::basic_string_view<detail::get_char_type_t<std::decay_t<K>>>(name);
  auto value_view =
      std::basic_string_view<detail::get_char_type_t<std::decay_t<V>>>(value);
  return detail::set(name_view, value_view, overwrite);
}

template <detail::string_like_type T>
inline bool unset(T&& name) {
  if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
    if (name == nullptr) {
      return false;
    }
  }
  auto name_view =
      std::basic_string_view<detail::get_char_type_t<std::decay_t<T>>>(name);
  return detail::unset(name_view);
}

inline std::vector<std::string> path() {
  auto path = ::env::get("PATH").value_or("");
#if defined(_WIN32)
  return detail::split(path, ';');
#else
  return detail::split(path, ':');
#endif
}

#if !defined(_WIN32)
inline std::map<std::string, std::string> all() {
  std::map<std::string, std::string> envs;
  detail::all(envs);
  return envs;
}
#else   // !_WIN32
inline std::map<std::string, std::string> all() {
  std::map<std::string, std::string> envs;
  detail::all(envs);
  return envs;
}
inline std::map<std::wstring, std::wstring> allw() {
  std::map<std::wstring, std::wstring> envs;
  detail::all(envs);
  return envs;
}
template <detail::string_like_type T>
inline std::basic_string<detail::get_char_type_t<std::decay_t<T>>> expand(
    T&& name) {
  auto name_view =
      std::basic_string_view<detail::get_char_type_t<std::decay_t<T>>>(name);
  return detail::expand(name_view);
}

inline std::vector<std::wstring> pathw() {
  auto path = ::env::get(L"PATH").value_or(L"");
  return detail::split(path, L';');
}
#endif  // _WIN32

namespace detail {
// RAII helper to set and restore an environment variable.
template <typename CharT>
class scoped_env {
  using string_type = std::basic_string<CharT>;

 public:
  scoped_env(string_type const& var, std::optional<string_type> const& value)
      : var_(var) {
    if (auto v = env::get(var); v.has_value()) {
      original_value_.emplace(v.value());
    }
    if (value) {
      env::set(var, value.value(), true);
    } else {
      env::unset(var);
    }
  }
  ~scoped_env() {
    if (original_value_) {
      env::set(var_, original_value_->c_str(), 1);
    } else {
      env::unset(var_);
    }
  }

 private:
  const string_type var_;
  std::optional<string_type> original_value_;
};
}  // namespace detail

template <typename F>
  requires std::is_invocable_v<F>
inline void with_env(std::string const& var,
                     std::optional<std::string> const& value, F&& f) {
  detail::scoped_env env(var, value);
  std::forward<F>(f)();
}
#if defined(_WIN32)
template <typename F>
  requires std::is_invocable_v<F>
inline void with_env(std::wstring const& var,
                     std::optional<std::wstring> const& value, F&& f) {
  detail::scoped_env env(var, value);
  std::forward<F>(f)();
}
#endif

}  // namespace env

#endif  // __ENVIRONMENT_ENVIRONMENT_HPP__
