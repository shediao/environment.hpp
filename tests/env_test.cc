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
