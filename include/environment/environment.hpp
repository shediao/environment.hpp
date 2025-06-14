#ifndef __ENVIRONMENT_ENVIRONMENT_HPP__
#define __ENVIRONMENT_ENVIRONMENT_HPP__

#include <cctype>
#include <cstdlib>
#include <cwctype>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#if defined(_WIN32)
#include <windows.h>

#include <algorithm>
#include <vector>
#else
extern char** environ;
#endif

namespace env {

namespace detail {
#if defined(_WIN32)
#if defined(UNICODE)
// Helper function to convert a UTF-8 std::string to a UTF-16 std::wstring
inline std::wstring to_wstring(const std::string& str) {
  if (str.empty()) {
    return {};
  }
  int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  if (size_needed <= 0) {
    // Consider throwing an exception for conversion errors
    return {};
  }
  std::wstring wstr(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0],
                      size_needed);
  return wstr;
}

// Helper function to convert a UTF-16 std::wstring to a UTF-8 std::string
inline std::string to_string(const std::wstring& wstr) {
  if (wstr.empty()) {
    return {};
  }
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                                        NULL, 0, NULL, NULL);
  if (size_needed <= 0) {
    // Consider throwing an exception for conversion errors
    return {};
  }
  std::string str(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0],
                      size_needed, NULL, NULL);
  return str;
}
template <typename CharT>
std::optional<std::basic_string<CharT>> get(
    std::basic_string<CharT> const& name) {
  size_t size = 0;
  if constexpr (std::is_same_v<char, CharT>) {
    size = GetEnvironmentVariable(detail::to_wstring(name).c_str(), nullptr, 0);
  } else {
    size = GetEnvironmentVariable(name.c_str(), nullptr, 0);
  }
  if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
    return std::nullopt;
  }
  std::vector<wchar_t> value(size);

  if constexpr (std::is_same_v<char, CharT>) {
    GetEnvironmentVariable(detail::to_wstring(name).c_str(), value.data(),
                           size);
  } else {
    GetEnvironmentVariable(name.c_str(), value.data(), size);
  }
  std::wstring ret{value.data()};

  if constexpr (std::is_same_v<char, CharT>) {
    return detail::to_string(ret);
  } else {
    return ret;
  }
}

template <typename CharT>
bool set(std::basic_string<CharT> const& name,
         std::basic_string<CharT> const& value, bool overwrite = true) {
  if (name.empty()) {
    return false;
  }
  if (!overwrite) {
    size_t size = 0;
    if constexpr (std::is_same_v<char, CharT>) {
      size =
          GetEnvironmentVariable(detail::to_wstring(name).c_str(), nullptr, 0);
    } else {
      size = GetEnvironmentVariable(name.c_str(), nullptr, 0);
    }
    if (size != 0 || GetLastError() != ERROR_ENVVAR_NOT_FOUND) {
      return true;
    }
  }
  if constexpr (std::is_same_v<char, CharT>) {
    return SetEnvironmentVariable(detail::to_wstring(name).c_str(),
                                  detail::to_wstring(value).c_str());
  } else {
    return SetEnvironmentVariable(name.c_str(), value.c_str());
  }
}

template <typename CharT>
bool unset(std::basic_string<CharT> const& name) {
  if (name.empty()) {
    return false;
  }
  if constexpr (std::is_same_v<char, CharT>) {
    return SetEnvironmentVariable(detail::to_wstring(name).c_str(), nullptr);
  } else {
    return SetEnvironmentVariable(name.c_str(), nullptr);
  }
}
#endif  // !UNICODE
#endif  // _WIN32
}  // namespace detail

#if !defined(_WIN32)
inline std::optional<std::string> get(std::string const& name) {
  auto* env = ::getenv(name.c_str());
  if (env) {
    return std::string(env);
  }
  return std::nullopt;
}

inline bool set(std::string const& name, std::string const& value,
                bool overwrite = true) {
  if (name.empty()) {
    return false;
  }
  if (::setenv(name.c_str(), value.c_str(), overwrite ? 1 : 0) != 0) {
    return false;
  }
  return true;
}

inline bool unset(std::string const& name) {
  if (name.empty()) {
    return false;
  }
  return ::unsetenv(name.c_str()) == 0;
}
inline std::map<std::string, std::string> all() {
  std::map<std::string, std::string> envs;
  if (environ == nullptr) {
    return envs;
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
  return envs;
}
#else  // !_WIN32

#if defined(UNICODE)
inline std::optional<std::string> get(std::string const& name) {
  return detail::get<char>(name);
}
inline std::optional<std::wstring> get(std::wstring const& name) {
  return detail::get<wchar_t>(name);
}
#else
inline std::optional<std::string> get(std::string const& name) {
  auto const size = GetEnvironmentVariableA(name.c_str(), nullptr, 0);
  if (size == 0) {
    return GetLastError() == ERROR_ENVVAR_NOT_FOUND
               ? std::nullopt
               : std::optional<std::string>{""};
  }
  std::vector<char> value(size);
  GetEnvironmentVariableA(name.c_str(), value.data(), size);
  return std::string{value.data()};
}
#endif  // !UNICODE

#if defined(UNICODE)
inline bool set(std::string const& name, std::string const& value,
                bool overwrite = true) {
  return detail::set<char>(name, value, overwrite);
}
inline bool set(std::wstring const& name, std::wstring const& value,
                bool overwrite = true) {
  return detail::set<wchar_t>(name, value, overwrite);
}
#else
inline bool set(std::string const& name, std::string const& value,
                bool overwrite = true) {
  if (name.empty()) {
    return false;
  }
  if (!overwrite) {
    auto const size = GetEnvironmentVariableA(name.c_str(), nullptr, 0);
    if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return SetEnvironmentVariableA(name.c_str(), value.c_str());
    }
    return true;
  }
  return SetEnvironmentVariableA(name.c_str(), value.c_str());
}
#endif  // !UNICODE

#if defined(UNICODE)
inline bool unset(std::string const& name) { return detail::unset<char>(name); }

inline bool unset(std::wstring const& name) {
  return detail::unset<wchar_t>(name);
}
#else
inline bool unset(std::string const& name) {
  if (name.empty()) {
    return false;
  }
  return SetEnvironmentVariableA(name.c_str(), nullptr);
}
#endif  // !UNICODE

#if defined(UNICODE)
template <typename StringType = std::wstring>
std::map<StringType, StringType> all();
template <>
inline std::map<std::wstring, std::wstring> all<std::wstring>() {
  std::map<std::wstring, std::wstring> envs;
  wchar_t* envBlock = GetEnvironmentStrings();
  if (envBlock == nullptr) {
    return envs;
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
                     [](wchar_t c) { return std::towupper(c); });
      envs[std::move(key)] = std::move(value);
    }
    currentEnv +=
        envString.length() + 1;  // Move to the next environment variable
  }

  FreeEnvironmentStrings(envBlock);
  return envs;
}
template <>
inline std::map<std::string, std::string> all<std::string>() {
  std::map<std::string, std::string> envs;
  for (auto const& [key, val] : all<std::wstring>()) {
    envs[detail::to_string(key)] = detail::to_string(val);
  }
  return envs;
}
inline std::map<std::string, std::string> allutf8() {
  return all<std::string>();
}
inline std::map<std::wstring, std::wstring> allutf16() {
  return all<std::wstring>();
}

#else

inline std::map<std::string, std::string> all() {
  std::map<std::string, std::string> envs;

  auto* envBlock = GetEnvironmentStrings();
  if (envBlock == nullptr) {
    return envs;
  }

  const auto* currentEnv = envBlock;
  while (*currentEnv != TEXT('\0')) {
    std::string_view envString(currentEnv);
    auto pos = envString.find(TEXT('='));
    // Weird variables in Windows that start with '='.
    // The key is the name of a drive, like "=C:", and the value is the
    // current working directory on that drive.
    if (pos == 0) {
      pos = envString.find(TEXT('='), 1);
    }
    if (pos != std::string_view::npos) {
      auto key = std::string(envString.substr(0, pos));
      auto value = std::string(envString.substr(pos + 1));
      std::transform(key.begin(), key.end(), key.begin(),
                     [](unsigned char c) { return std::toupper(c); });
      envs[std::move(key)] = std::move(value);
    }
    currentEnv +=
        envString.length() + 1;  // Move to the next environment variable
  }

  FreeEnvironmentStringsA(envBlock);
  return envs;
}

#endif  // !UNICODE

#endif  // _WIN32

}  // namespace env

#endif  // __ENVIRONMENT_ENVIRONMENT_HPP__
