#ifndef __ENVIRONMENT_ENVIRONMENT_HPP__
#define __ENVIRONMENT_ENVIRONMENT_HPP__

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cwctype>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
extern char** environ;
#endif

#if defined(_WIN32)
#if defined(GetEnvironmentStrings)
// Unlike most of the WinAPI, GetEnvironmentStrings is a real function and
// GetEnvironmentStringsA is a macro. In UNICODE builds, GetEnvironmentStrings
// is also defined as a macro that redirects to GetEnvironmentStringsW, and the
// narrow character version become inaccessible. Facepalm.
#if defined(_MSC_VER) || defined(__GNUC__)
#pragma push_macro("GetEnvironmentStrings")
#endif
#undef GetEnvironmentStrings
#define ENVIRONMENT_HPP_GET_ENVIRONMENT_STRINGS_UNDEFINED
#endif  // defined(GetEnvironmentStrings)
#endif  // _WIN32

namespace env {

namespace detail {
#if (defined(_WIN32) || defined(_WIN64)) && \
    (defined(UNICODE) || defined(_UNICODE))
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
#endif
}  // namespace detail

inline std::optional<std::string> getenv(std::string const& name) {
#if defined(_WIN32)
#if defined(UNICODE) || defined(_UNICODE)
  auto namew = detail::to_wstring(name);
  auto const size = GetEnvironmentVariableW(namew.c_str(), nullptr, 0);
  if (size == 0) {
    return GetLastError() == ERROR_ENVVAR_NOT_FOUND
               ? std::nullopt
               : std::optional<std::string>{""};
  }
  std::vector<wchar_t> value(size);
  GetEnvironmentVariableW(namew.c_str(), value.data(), size);
  return detail::to_string(value.data());
#else
  auto const size = GetEnvironmentVariableA(name.c_str(), nullptr, 0);
  if (size == 0) {
    return GetLastError() == ERROR_ENVVAR_NOT_FOUND
               ? std::nullopt
               : std::optional<std::string>{""};
  }
  std::vector<char> value(size);
  GetEnvironmentVariableA(name.c_str(), value.data(), size);
  return std::string{value.data()};
#endif
#else
  auto* env = ::getenv(name.c_str());
  if (env) {
    return std::string(env);
  }
  return std::nullopt;
#endif
}

inline bool setenv(std::string const& name, std::string const& value,
                   bool overwrite = true) {
  if (name.empty()) {
    return false;
  }
#if defined(_WIN32)
#if defined(UNICODE) || defined(_UNICODE)
  auto namew = detail::to_wstring(name);
  auto valuew = detail::to_wstring(value);
  if (!overwrite) {
    auto const size = GetEnvironmentVariableW(namew.c_str(), nullptr, 0);
    if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return SetEnvironmentVariableW(namew.c_str(), valuew.c_str());
    }
    return true;
  }
  return SetEnvironmentVariableW(namew.c_str(), valuew.c_str());
#else
  if (!overwrite) {
    auto const size = GetEnvironmentVariableA(name.c_str(), nullptr, 0);
    if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return SetEnvironmentVariableA(name.c_str(), value.c_str());
    }
    return true;
  }
  return SetEnvironmentVariableA(name.c_str(), value.c_str());
#endif
#else
  if (::setenv(name.c_str(), value.c_str(), overwrite ? 1 : 0) != 0) {
    return false;
  }
  return true;
#endif
}

inline bool unsetenv(std::string const& name) {
  if (name.empty()) {
    return false;
  }
#if defined(_WIN32)
#if defined(UNICODE) || defined(_UNICODE)
  auto namew = detail::to_wstring(name);
  return SetEnvironmentVariableW(namew.c_str(), nullptr);
#else
  return SetEnvironmentVariableA(name.c_str(), nullptr);
#endif
#else
  return ::unsetenv(name.c_str()) == 0;
#endif
}

#if defined(_WIN32)
#if defined(UNICODE) || defined(_UNICODE)
template <typename StringType = std::wstring>
inline std::map<StringType, StringType> environs();
#else
inline std::map<std::string, std::string> environs();
#endif
#endif

#if defined(_WIN32)
#if defined(UNICODE) || defined(_UNICODE)
template <>
inline std::map<std::string, std::string> environs<std::string>() {
#else
inline std::map<std::string, std::string> environs() {
#endif
  std::map<std::string, std::string> envs;

#if defined(UNICODE) || defined(_UNICODE)
  auto* envBlock = GetEnvironmentStringsW();
#else
  auto* envBlock = GetEnvironmentStrings();
#endif
  if (envBlock == nullptr) {
    return envs;
  }

  const auto* currentEnv = envBlock;
  while (*currentEnv != TEXT('\0')) {
#if defined(UNICODE) || defined(_UNICODE)
    std::wstring_view envString(currentEnv);
#else
    std::string_view envString(currentEnv);
#endif
    auto pos = envString.find(TEXT('='));
    // Weird variables in Windows that start with '='.
    // The key is the name of a drive, like "=C:", and the value is the
    // current working directory on that drive.
    if (pos == 0) {
      pos = envString.find(TEXT('='), 1);
    }
#if defined(UNICODE) || defined(_UNICODE)
    if (pos != std::wstring_view::npos) {
      auto key = std::wstring(envString.substr(0, pos));
      auto value = std::wstring(envString.substr(pos + 1));
      std::transform(key.begin(), key.end(), key.begin(),
                     [](wchar_t c) { return std::towupper(c); });
      envs[detail::to_string(key)] = detail::to_string(value);
    }
#else
    if (pos != std::string_view::npos) {
      auto key = std::string(envString.substr(0, pos));
      auto value = std::string(envString.substr(pos + 1));
      std::transform(key.begin(), key.end(), key.begin(),
                     [](unsigned char c) { return std::toupper(c); });
      envs[std::move(key)] = std::move(value);
    }
#endif
    currentEnv +=
        envString.length() + 1;  // Move to the next environment variable
  }

#if defined(UNICODE)
  FreeEnvironmentStrings(envBlock);
#else
  FreeEnvironmentStringsA(envBlock);
#endif
  return envs;
}

#else   // _WIN32
inline std::map<std::string, std::string> environs() {
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
#endif  // !_WIN32

#if defined(_WIN32) && (defined(UNICODE) || defined(_UNICODE))
inline std::optional<std::wstring> getenv(std::wstring const& name) {
  auto const size = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
  if (size == 0) {
    return GetLastError() == ERROR_ENVVAR_NOT_FOUND
               ? std::nullopt
               : std::optional<std::wstring>{L""};
  }
  std::vector<wchar_t> value(size);
  GetEnvironmentVariableW(name.c_str(), value.data(), size);
  return std::wstring{value.data()};
}

inline bool setenv(std::wstring const& name, std::wstring const& value,
                   bool overwrite = true) {
  if (name.empty()) {
    return false;
  }
  if (!overwrite) {
    auto const size = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
    if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return SetEnvironmentVariableW(name.c_str(), value.c_str());
    }
    return true;
  }
  return SetEnvironmentVariableW(name.c_str(), value.c_str());
}

inline bool unsetenv(std::wstring const& name) {
  if (name.empty()) {
    return false;
  }
  return SetEnvironmentVariableW(name.c_str(), nullptr);
}

template <>
inline std::map<std::wstring, std::wstring> environs<std::wstring>() {
  std::map<std::wstring, std::wstring> envs;
  wchar_t* envBlock = GetEnvironmentStringsW();
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
#endif  // _WIN32 && UNICODE

}  // namespace env

#if defined(ENVIRONMENT_HPP_GET_ENVIRONMENT_STRINGS_UNDEFINED)
#if defined(_MSC_VER) || defined(__GNUC__)
#pragma pop_macro("GetEnvironmentStrings")
#elif defined(UNICODE)
#define GetEnvironmentStrings GetEnvironmentStringsW
#endif
#undef ENVIRONMENT_HPP_GET_ENVIRONMENT_STRINGS_UNDEFINED
#endif  // defined(ENVIRONMENT_HPP_GET_ENVIRONMENT_STRINGS_UNDEFINED)

#endif  // __ENVIRONMENT_ENVIRONMENT_HPP__
