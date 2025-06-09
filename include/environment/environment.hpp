#ifndef __ENVIRONMENT_ENVIRONMENT_HPP__
#define __ENVIRONMENT_ENVIRONMENT_HPP__

#include <cstdlib>
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

namespace environment {

inline std::optional<std::string> getenv(std::string const& name) {
#if defined(_WIN32)
  auto const size = GetEnvironmentVariableA(name.c_str(), nullptr, 0);
  if (size == 0) {
    return GetLastError() == ERROR_ENVVAR_NOT_FOUND
               ? std::nullopt
               : std::optional<std::string>{""};
  }
  std::vector<char> value(size);
  GetEnvironmentVariableA(name.c_str(), value.data(), size);
  return std::string{value.data()};
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
  if (!overwrite) {
    auto const size = GetEnvironmentVariableA(name.c_str(), nullptr, 0);
    if (size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return SetEnvironmentVariableA(name.c_str(), value.c_str());
    }
    return true;
  }
  return SetEnvironmentVariableA(name.c_str(), value.c_str());
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
  return SetEnvironmentVariableA(name.c_str(), nullptr);
#else
  return ::unsetenv(name.c_str()) == 0;
#endif
}

#if defined(_WIN32)
#if defined(UNICODE) || defined(_UNICODE)
template <typename StringType = std::wstring>
#else
template <typename StringType = std::string>
#endif
inline std::map<StringType, StringType> environments();
#endif

#if defined(_WIN32)
template <>
#endif
inline std::map<std::string, std::string> environments
#if defined(_WIN32)
    <std::string>
#endif
    () {
  std::map<std::string, std::string> envs;
#if defined(_WIN32)
  auto* envBlock = GetEnvironmentStrings();
  if (envBlock == nullptr) {
    return envs;
  }

  const char* currentEnv = envBlock;
  while (*currentEnv != '\0') {
    std::string_view envString(currentEnv);
    auto pos = envString.find('=');
    // Weird variables in Windows that start with '='.
    // The key is the name of a drive, like "=C:", and the value is the
    // current working directory on that drive.
    if (pos == 0) {
      pos = envString.find('=', 1);
    }
    if (pos != std::string_view::npos) {
      std::string_view key = envString.substr(0, pos);
      std::string_view value = envString.substr(pos + 1);
      envs[std::string(key)] = std::string(value);
    }
    currentEnv +=
        envString.length() + 1;  // Move to the next environment variable
  }

#if defined(UNICODE)
  FreeEnvironmentStringsA(envBlock);
#else
  FreeEnvironmentStrings(envBlock);
#endif

#else

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
#endif
  return envs;
}

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
inline std::map<std::wstring, std::wstring> environments<std::wstring>() {
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
      std::wstring_view key = envString.substr(0, pos);
      std::wstring_view value = envString.substr(pos + 1);
      envs[std::wstring(key)] = std::wstring(value);
    }
    currentEnv +=
        envString.length() + 1;  // Move to the next environment variable
  }

  FreeEnvironmentStrings(envBlock);
  return envs;
}
#endif  // _WIN32 && UNICODE

}  // namespace environment

#if defined(ENVIRONMENT_HPP_GET_ENVIRONMENT_STRINGS_UNDEFINED)
#if defined(_MSC_VER) || defined(__GNUC__)
#pragma pop_macro("GetEnvironmentStrings")
#elif defined(UNICODE)
#define GetEnvironmentStrings GetEnvironmentStringsW
#endif
#undef ENVIRONMENT_HPP_GET_ENVIRONMENT_STRINGS_UNDEFINED
#endif  // defined(ENVIRONMENT_HPP_GET_ENVIRONMENT_STRINGS_UNDEFINED)

#endif  // __ENVIRONMENT_ENVIRONMENT_HPP__
