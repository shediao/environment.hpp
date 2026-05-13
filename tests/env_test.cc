#include <gtest/gtest.h>

#include <environment/environment.hpp>
#include <string_view>
#include <subprocess/subprocess.hpp>

using namespace process::detail::named_args;

#define MK_ENV() MK_ENV_IMPL("TEST_ENV_", __LINE__)
#define MK_ENV_IMPL(x, y) MK_ENV_IMPL_I(x, y)
#define MK_ENV_IMPL_I(x, y) \
  std::pair<std::string, std::string> { x "KEY_" #y, x "VALUE_" #y }

TEST(EnvironmentTest, AllEnv1) {
  auto envs = env::all();
  ASSERT_GT(envs.size(), 0);
  for (auto const& [key, value] : envs) {
    auto env_ = env::get(key);
    ASSERT_TRUE(env_.has_value())
        << "environment: key=" << key << ", value='" << value << "'";
    ASSERT_EQ(value, env_.value());
  }
}

TEST(EnvironmentTest, GenEnv1) {
  ASSERT_TRUE(env::get("PATH").has_value());
#if defined(_WIN32)
  ASSERT_TRUE(env::get(TEXT("USERPROFILE")).has_value());
#else
  ASSERT_TRUE(env::get("HOME").has_value());
  ASSERT_TRUE(env::get("USER").has_value());
#endif
}

TEST(EnvironmentTest, SetEnv1) {
  auto [key, value] = MK_ENV();
  env::set(key, value);
  std::vector<char> stdout_;
#if defined(_WIN32)
  process::run("cmd.exe", "/c", "<nul set /p=%" + key + "%&exit /b 0",
               $stdout > stdout_);
#else
  process::run("bash", "-c", "echo -n $" + key, $stdout > stdout_);
#endif
  ASSERT_EQ(std::string_view(stdout_.data(), stdout_.size()), value);

  ASSERT_TRUE(env::get(key).has_value());
  ASSERT_EQ(env::get(key).value_or(""), value);
}

TEST(EnvironmentTest, UnsetEnv1) {
  auto [key, value] = MK_ENV();
  env::set(key, value);
  std::vector<char> stdout_;
#if defined(_WIN32)
  process::run("cmd.exe", "/c", "<nul set /p=%" + key + "%&exit /b 0",
               $stdout > stdout_);
#else
  process::run("bash", "-c", "echo -n $" + key, $stdout > stdout_);
#endif
  ASSERT_EQ(std::string_view(stdout_.data(), stdout_.size()), value);

  env::unset(key);
  stdout_.clear();
#if defined(_WIN32)
  process::run("cmd.exe", "/c",
               "if defined " + key + " (<nul set /p=%" + key + "%)&exit /b 0",
               $stdout > stdout_);
#else
  process::run("bash", "-c", "echo -n $" + key, $stdout > stdout_);
#endif
  ASSERT_TRUE(stdout_.empty()) << "stdout_='" << stdout_.data() << "'";
}

#if defined(_WIN32)
TEST(EnvironmentTest, ExpandTest) {
  using namespace std::string_literals;
  for (auto const s : {"USERPROFILE", "APPDATA", "TMP", "USERNAME", "PATH",
                       "APPDATA", "ProgramFiles", "SystemRoot"}) {
    auto e = env::expand("%"s + s + "%");
    ASSERT_TRUE(!e.empty() && e[0] != '%');

    auto ew = env::expand(L"%"s + env::detail::to_wstring(s) + L"%"s);
    ASSERT_TRUE(!ew.empty() && ew[0] != L'%');
  }

  env::unset("THIS_ENV_NOT_EXISTS");
  ASSERT_EQ(env::expand("%THIS_ENV_NOT_EXISTS%"), "%THIS_ENV_NOT_EXISTS%");
}
#endif

TEST(EnvironmentTest, WithEnv) {
  constexpr char key[] = "TEST_WITH_ENV_RAII";
  constexpr char value[] = "123";
  ASSERT_FALSE(env::get(key));
  env::with_env(key, value, [&]() {
    ASSERT_TRUE(env::get(key));
    ASSERT_EQ(env::get(key).value(), value);
  });
  ASSERT_FALSE(env::get(key));

  env::set(key, value);
  ASSERT_TRUE(env::get(key));
  env::with_env(key, std::nullopt, [&]() { ASSERT_FALSE(env::get(key)); });
  ASSERT_TRUE(env::get(key));
  env::unset(key);

#if defined(_WIN32)
  constexpr wchar_t wkey[] = L"TEST_WITH_ENV_RAII";
  constexpr wchar_t wvalue[] = L"123";
  env::with_env(wkey, wvalue, [&]() {
    ASSERT_TRUE(env::get(wkey));
    ASSERT_EQ(env::get(wkey).value(), wvalue);
  });
  ASSERT_FALSE(env::get(wkey));

  env::set(wkey, wvalue);
  ASSERT_TRUE(env::get(wkey));
  env::with_env(wkey, std::nullopt, [&]() { ASSERT_FALSE(env::get(wkey)); });
  ASSERT_TRUE(env::get(wkey));
  env::unset(wkey);
#endif
}

// =============================================================================
// env::path() tests
// =============================================================================

TEST(EnvironmentTest, PathNotEmpty) {
  auto paths = env::path();
  ASSERT_GT(paths.size(), 0) << "PATH should contain at least one directory";
}

TEST(EnvironmentTest, PathElementsNonEmpty) {
  auto paths = env::path();
  for (std::size_t i = 0; i < paths.size(); ++i) {
    ASSERT_FALSE(paths[i].empty())
        << "path() element at index " << i << " is empty";
  }
}

TEST(EnvironmentTest, PathReconstructFromGet) {
  // Each element returned by path() should appear as a substring in the
  // original PATH value.
  auto original_path = env::get("PATH");
  ASSERT_TRUE(original_path.has_value());
  auto paths = env::path();

  // Reconstruct PATH by joining with the platform separator.
  std::string reconstructed;
#if defined(_WIN32)
  constexpr char sep = ';';
#else
  constexpr char sep = ':';
#endif
  for (std::size_t i = 0; i < paths.size(); ++i) {
    if (i > 0) {
      reconstructed += sep;
    }
    reconstructed += paths[i];
  }

  // Every path element should appear in the original PATH.
  for (auto const& p : paths) {
    ASSERT_NE(original_path.value().find(p), std::string::npos)
        << "path element '" << p << "' not found in PATH";
  }

  // The reconstructed string shouldn't exceed the original PATH in length
  // (it may be shorter due to collapsing consecutive separators).
  ASSERT_LE(reconstructed.size(), original_path.value().size());
}

TEST(EnvironmentTest, PathWithCustomValue) {
  constexpr char key[] = "PATH";

#if defined(_WIN32)
  constexpr char custom_path[] = "C:\\foo;C:\\bar\\baz;C:\\qux";
  constexpr char sep = ';';
#else
  constexpr char custom_path[] = "/usr/bin:/bin:/usr/local/bin";
  constexpr char sep = ':';
#endif

  env::with_env(key, custom_path, [&]() {
    auto paths = env::path();
    // Reconstruct and verify
    std::string reconstructed;
    for (std::size_t i = 0; i < paths.size(); ++i) {
      if (i > 0) {
        reconstructed += sep;
      }
      reconstructed += paths[i];
    }
    ASSERT_EQ(reconstructed, custom_path);
  });
}

TEST(EnvironmentTest, PathSingleElement) {
  constexpr char key[] = "PATH";

#if defined(_WIN32)
  constexpr char custom_path[] = "C:\\only\\this\\dir";
#else
  constexpr char custom_path[] = "/only/this/dir";
#endif

  env::with_env(key, custom_path, [&]() {
    auto paths = env::path();
    ASSERT_EQ(paths.size(), 1);
    ASSERT_EQ(paths[0], custom_path);
  });
}

TEST(EnvironmentTest, PathWithConsecutiveSeparators) {
  constexpr char key[] = "PATH";

  // detail::split skips empty elements caused by consecutive delimiters,
  // and also skips the trailing empty element if the string ends with the
  // delimiter.
#if defined(_WIN32)
  env::with_env(key, "C:\\a;;C:\\b;", []() {
    auto paths = env::path();
    ASSERT_EQ(paths.size(), 2);
    ASSERT_EQ(paths[0], "C:\\a");
    ASSERT_EQ(paths[1], "C:\\b");
  });
#else
  env::with_env(key, "/usr/bin::/bin:", []() {
    auto paths = env::path();
    ASSERT_EQ(paths.size(), 2);
    ASSERT_EQ(paths[0], "/usr/bin");
    ASSERT_EQ(paths[1], "/bin");
  });
#endif
}

TEST(EnvironmentTest, PathEmptyPath) {
  constexpr char key[] = "PATH";

  env::with_env(key, "", []() {
    auto paths = env::path();
    ASSERT_TRUE(paths.empty());
  });
}
