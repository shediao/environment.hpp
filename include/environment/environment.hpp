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
#include <sys/stat.h>
#include <unistd.h>

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
    std::same_as<volatile wchar_t*, std::decay_t<T>> ||
    std::same_as<const wchar_t*, std::decay_t<T>> ||
    std::same_as<std::wstring, std::decay_t<T>> ||
    std::same_as<std::wstring_view, std::decay_t<T>> ||
#endif
    std::same_as<char*, std::decay_t<T>> ||
    std::same_as<volatile char*, std::decay_t<T>> ||
    std::same_as<const char*, std::decay_t<T>> ||
    std::same_as<std::string, std::decay_t<T>> ||
    std::same_as<std::string_view, std::decay_t<T>>;

template <typename T>
concept cstr_like_type =
#if defined(_WIN32)
    std::same_as<wchar_t*, std::decay_t<T>> ||
    std::same_as<volatile wchar_t*, std::decay_t<T>> ||
    std::same_as<const wchar_t*, std::decay_t<T>> ||
#endif
    std::same_as<char*, std::decay_t<T>> ||
    std::same_as<volatile char*, std::decay_t<T>> ||
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

template <typename T>
using to_string_view_t =
    std::basic_string_view<get_char_type_t<std::decay_t<T>>>;

template <typename T>
using to_string_t = std::basic_string<get_char_type_t<std::decay_t<T>>>;

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
    DWORD const size = GetEnvironmentVariableW(wname.c_str(), nullptr, 0);
    if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return std::nullopt;
    }
    std::wstring ret(size, L'\0');
    DWORD const copied =
        GetEnvironmentVariableW(wname.c_str(), ret.data(), size);
    ret.resize(copied);
    return detail::to_string(ret);
  } else {
    DWORD const size = GetEnvironmentVariableW(name.data(), nullptr, 0);
    if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return std::nullopt;
    }
    std::wstring ret(size, L'\0');
    DWORD const copied = GetEnvironmentVariableW(name.data(), ret.data(), size);
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
inline std::optional<std::wstring> system_search_path(
    std::wstring_view command) {
  if (command.empty()) {
    return std::nullopt;
  }

  std::wstring const wcmd = std::wstring(command);
  auto search_with_ext =
      [&](const wchar_t* ext) -> std::optional<std::wstring> {
    DWORD const size =
        SearchPathW(nullptr, wcmd.c_str(), ext, 0, nullptr, nullptr);
    if (size == 0) {
      return std::nullopt;
    }
    std::wstring result(size, L'\0');
    DWORD const copied =
        SearchPathW(nullptr, wcmd.c_str(), ext, size, result.data(), nullptr);
    // copied includes the null terminator
    result.resize(copied);
    return result;
  };

  auto pathext = detail::get(std::wstring_view(L"PATHEXT"))
                     .value_or(L".COM;.EXE;.BAT;.CMD");

  if (auto dot = wcmd.find_last_of(L'.');
      dot != std::wstring_view::npos && dot > 0) {
    return search_with_ext(nullptr);
  }
  for (auto& ext : detail::split(pathext, L';')) {
    if (ext.empty() || ext[0] != L'.') {
      continue;
    }
    if (auto result = search_with_ext(ext.data()); result.has_value()) {
      return result;
    }
  }
  return std::nullopt;
}
inline std::optional<std::wstring> env_search_path(std::wstring_view command) {
  if (command.empty()) {
    return std::nullopt;
  }
  auto path_env = detail::get(std::wstring_view(L"PATH"));
  if (!path_env.has_value()) {
    return std::nullopt;
  }

  std::wstring const wcmd = std::wstring(command);
  auto pathexts = split(detail::get(std::wstring_view(L"PATHEXT"))
                            .value_or(L".COM;.EXE;.BAT;.CMD"),
                        L';');

  bool command_has_ext = false;
  if (auto dot = wcmd.find_last_of(L'.');
      dot != std::wstring_view::npos && dot > 0) {
    command_has_ext = true;
  }
  for (auto& path : detail::split(path_env.value(), L';')) {
    if (command_has_ext) {
      std::wstring file = path + L"\\" + wcmd;
      auto attr = GetFileAttributesW(file.c_str());
      if (attr == INVALID_FILE_ATTRIBUTES || attr & FILE_ATTRIBUTE_DIRECTORY) {
        continue;
      }
      return file;
    }
    for (auto& ext : pathexts) {
      if (ext.empty() || ext[0] != L'.') {
        continue;
      }
      std::wstring file = path + L"\\" + wcmd + ext;
      auto attr = GetFileAttributesW(file.c_str());
      if (attr == INVALID_FILE_ATTRIBUTES ||
          (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        continue;
      }
      return file;
    }
  }
  return std::nullopt;
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
inline std::optional<detail::to_string_t<T>> get(T&& name) {
  return detail::get(detail::to_string_view_t<T>(std::forward<T>(name)));
}

template <detail::string_like_type K, detail::string_like_type V>
inline bool set(K&& name, V&& value, bool overwrite = true) {
  return detail::set(detail::to_string_view_t<K>(std::forward<K>(name)),
                     detail::to_string_view_t<V>(std::forward<V>(value)),
                     overwrite);
}

template <detail::string_like_type T>
inline bool unset(T&& name) {
  if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
    if (name == nullptr) {
      return false;
    }
  }
  return detail::unset(detail::to_string_view_t<T>(std::forward<T>(name)));
}

inline std::vector<std::string> path() {
  auto path = ::env::get("PATH").value_or("");
#if defined(_WIN32)
  char sep = ';';
#else
  char sep = ':';
#endif
  std::vector<std::string> ret;
  for (auto&& p : detail::split(path, sep)) {
    if (p.empty()) {
      continue;
    }
    ret.emplace_back(p);
  }
  return ret;
}

// ---------------------------------------------------------------------------
// search_path(command)
//   Returns the absolute or resolved path of an executable by searching PATH.
//   - On UNIX: splits PATH by ':', checks each candidate with ::access(X_OK).
//   - On Windows: uses SearchPathW which follows the standard Windows
//     search order (application dir → current dir → system dirs → PATH),
//     and automatically appends PATHEXT extensions.
//
//   Returns std::nullopt if the command is empty or cannot be found.
// ---------------------------------------------------------------------------
#if defined(_WIN32)
// Wide-character variant for Windows – mirrors search_path() with wstring.
inline std::optional<std::wstring> search_path(std::wstring_view command) {
  if (command.empty()) {
    return std::nullopt;
  }
  auto ret = detail::env_search_path(command);
  if (ret.has_value()) {
    return ret;
  }
  return detail::system_search_path(command);
}
#endif  // _WIN32

inline std::optional<std::string> search_path(std::string_view command) {
  if (command.empty()) {
    return std::nullopt;
  }

#if defined(_WIN32)
  auto result = search_path(detail::to_wstring(command));
  if (result.has_value()) {
    return detail::to_string(std::move(*result));
  }
  return std::nullopt;
#else
  // If the command already contains a path separator, resolve it directly
  // without consulting PATH (POSIX semantics).
  if (command.find('/') != std::string_view::npos) {
    // Must be a regular file (not a directory) with execute permission.
    struct stat st;
    if (::stat(command.data(), &st) == 0 && S_ISREG(st.st_mode) &&
        ::access(command.data(), X_OK) == 0) {
      return std::string(command);
    }
    return std::nullopt;
  }

  // Search each directory in PATH.
  auto path_env = ::env::get("PATH");
  if (!path_env.has_value() || path_env->empty()) {
    return std::nullopt;
  }

  auto const dirs = detail::split(*path_env, ':');
  for (auto const& dir : dirs) {
    if (dir.empty()) {
      continue;  // skip empty entries from e.g. consecutive/trailing ':'
    }
    std::string candidate;
    candidate.reserve(dir.size() + 1 + command.size());
    candidate.append(dir);
    candidate.push_back('/');
    candidate.append(command);

    struct stat st;
    if (::stat(candidate.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
        ::access(candidate.c_str(), X_OK) == 0) {
      return candidate;
    }
  }

  return std::nullopt;
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
inline detail::to_string_t<T> expand(T&& name) {
  return detail::expand(detail::to_string_view_t<T>(std::forward<T>(name)));
}

inline std::vector<std::wstring> pathw() {
  std::vector<std::wstring> ret;
  auto path = ::env::get(L"PATH").value_or(L"");
  for (auto&& p : detail::split(path, L';')) {
    if (p.empty()) {
      continue;
    }
    ret.emplace_back(std::move(p));
  }
  return ret;
}
#endif  // _WIN32

namespace detail {
// RAII helper to set and restore an environment variable.
template <typename CharT>
class scoped_env {
  using string_type = std::basic_string<CharT>;
  using string_view_type = std::basic_string_view<CharT>;

 public:
  scoped_env(std::map<string_type, std::optional<string_type>> envs)
      : envs_(std::move(envs)) {
    for (const auto& [var, value] : envs_) {
      if (auto v = env::get(var); v.has_value()) {
        original_envs_[var] = v.value();
      }
      if (value) {
        env::set(var, value.value(), true);
      } else {
        env::unset(var);
      }
    }
  }
  scoped_env(string_type var, std::optional<string_type> value)
      : scoped_env(std::map<string_type, std::optional<string_type>>{
            {std::move(var), std::move(value)}}) {}
  ~scoped_env() {
    for (const auto& [var, value] : original_envs_) {
      env::set(var, value, true);
    }
    for (const auto& [var, _] : envs_) {
      if (original_envs_.find(var) == original_envs_.end()) {
        env::unset(var);
      }
    }
  }

 private:
  std::map<string_type, std::optional<string_type>> envs_;
  std::map<string_type, string_type> original_envs_;
};
}  // namespace detail

template <typename T, typename F>
  requires std::is_invocable_v<F> && detail::string_like_type<T>
inline void with(T&& var, std::optional<detail::to_string_t<T>> const& value,
                 F&& f) {
  detail::scoped_env env(detail::to_string_t<T>(std::forward<T>(var)), value);
  std::forward<F>(f)();
}

template <typename CharT, typename F>
  requires std::is_invocable_v<F>
inline void with(std::map<std::basic_string<CharT>,
                          std::optional<std::basic_string<CharT>>> const& envs,
                 F&& f) {
  detail::scoped_env env(envs);
  std::forward<F>(f)();
}

}  // namespace env

#endif  // __ENVIRONMENT_ENVIRONMENT_HPP__
