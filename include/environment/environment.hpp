#ifndef __ENVIRONMENT_ENVIRONMENT_HPP__
#define __ENVIRONMENT_ENVIRONMENT_HPP__

#include <cstdlib>
#include <map>
#include <optional>
#include <string>

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
  std::vector<char> buf(128);
  auto const size = GetEnvironmentVariableA(name.c_str(), buf.data(),
                                            static_cast<DWORD>(buf.size()));
  if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
    return std::nullopt;
  }
  if (size > buf.size()) {
    buf.resize(static_cast<size_t>(size));
    buf[0] = '\0';
    GetEnvironmentVariableA(name.c_str(), buf.data(),
                            static_cast<DWORD>(buf.size()));
  }
  return std::string{buf.data()};
#else
  auto* env = ::getenv(name.c_str());
  if (env) {
    return std::string(env);
  }
#endif
  return std::nullopt;
}

inline bool setenv(std::string const& name, std::string const& value,
                   bool overwrite = true) {
  if (name.empty()) {
    return false;
  }
#if defined(_WIN32)
  if (!overwrite) {
    std::vector<char> buf(128);
    GetEnvironmentVariableA(name.c_str(), buf.data(),
                            static_cast<DWORD>(buf.size()));
    if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return SetEnvironmentVariableA(name.c_str(), value.c_str());
    }
    return true;
  } else {
    return SetEnvironmentVariableA(name.c_str(), value.c_str());
  }
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
  return ::unsetenv(name.c_str());
#endif
}

#if defined(_WIN32)
#if defined(UNICODE) || defined(_UNICODE)
template <typename StringType = std::wstring>
#else
template <typename StringType = std::string>
#endif
inline std::map<StringType, StringType> allenv();
#endif

#if defined(_WIN32)
template <>
#endif
inline std::map<std::string, std::string> allenv
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

  char* currentEnv = envBlock;
  while (*currentEnv != '\0') {
    std::string envString(currentEnv);
    size_t pos = envString.find('=');
    if (pos != std::string::npos) {
      std::string key = envString.substr(0, pos);
      std::string value = envString.substr(pos + 1);
      envs[key] = value;
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
    std::string envString(*env);
    size_t pos = envString.find('=');
    if (pos != std::string::npos) {
      std::string key = envString.substr(0, pos);
      std::string value = envString.substr(pos + 1);
      envs[key] = value;
    }
  }
#endif
  return envs;
}

#if defined(_WIN32) && (defined(UNICODE) || defined(_UNICODE))
inline std::optional<std::wstring> getenv(std::wstring const& name) {
  std::vector<wchar_t> buf(128);
  auto const size = GetEnvironmentVariableW(name.c_str(), buf.data(),
                                            static_cast<DWORD>(buf.size()));
  if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
    return std::nullopt;
  }
  if (size > buf.size()) {
    buf.resize(static_cast<size_t>(size));
    buf[0] = L'\0';
    GetEnvironmentVariableW(name.c_str(), buf.data(),
                            static_cast<DWORD>(buf.size()));
  }
  return std::wstring{buf.data()};
}

inline bool setenv(std::wstring const& name, std::wstring const& value,
                   bool overwrite = true) {
  if (!overwrite) {
    std::vector<wchar_t> buf(128);
    GetEnvironmentVariableW(name.c_str(), buf.data(),
                            static_cast<DWORD>(buf.size()));
    if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return SetEnvironmentVariableW(name.c_str(), value.c_str());
    }
    return true;
  } else {
    return SetEnvironmentVariableW(name.c_str(), value.c_str());
  }
}

inline bool unsetenv(std::wstring const& name) {
  if (name.empty()) {
    return false;
  }
  return SetEnvironmentVariableW(name.c_str(), nullptr);
}

template <>
inline std::map<std::wstring, std::wstring> allenv<std::wstring>() {
  std::map<std::wstring, std::wstring> envs;
  wchar_t* envBlock = GetEnvironmentStringsW();
  if (envBlock == nullptr) {
    return envs;
  }

  wchar_t* currentEnv = envBlock;
  while (*currentEnv != L'\0') {
    std::wstring envString(currentEnv);
    size_t pos = envString.find(L'=');
    if (pos != std::string::npos) {
      std::wstring key = envString.substr(0, pos);
      std::wstring value = envString.substr(pos + 1);
      envs[key] = value;
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
