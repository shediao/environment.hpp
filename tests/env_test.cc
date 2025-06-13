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
  auto envs = env::environs();
  ASSERT_GT(envs.size(), 0);
#if defined(_WIN32) && defined(UNICODE)
  ASSERT_TRUE(
      (std::is_same_v<decltype(envs)::value_type::second_type, std::wstring>));
#else
  ASSERT_TRUE(
      (std::is_same_v<decltype(envs)::value_type::second_type, std::string>));
#endif
  for (auto const& [key, value] : envs) {
    auto env_ = env::getenv(key);
    ASSERT_TRUE(env_.has_value())
        << "environment: key=" << key << ", value='" << value << "'";
    ASSERT_EQ(value, env_.value());
  }
}

TEST(EnvironmentTest, GenEnv1) {
  ASSERT_TRUE(env::getenv("PATH").has_value());
#if defined(_WIN32)
  ASSERT_TRUE(env::getenv("USERPROFILE").has_value());
#if defined(UNICODE) || defined(_UNICODE)
  ASSERT_TRUE(env::getenv(L"USERPROFILE").has_value());
#endif
#else
  ASSERT_TRUE(env::getenv("HOME").has_value());
  ASSERT_TRUE(env::getenv("USER").has_value());
#endif
}

TEST(EnvironmentTest, SetEnv1) {
  auto [key, value] = MK_ENV();
  env::setenv(key, value);
  std::vector<char> stdout_;
#if defined(_WIN32)
  process::run("cmd.exe", "/c", "<nul set /p=%" + key + "%&exit /b 0",
               $stdout > stdout_);
#else
  process::run("bash", "-c", "echo -n $" + key, $stdout > stdout_);
#endif
  ASSERT_EQ(std::string_view(stdout_.data(), stdout_.size()), value);

  ASSERT_TRUE(env::getenv(key).has_value());
  ASSERT_EQ(env::getenv(key).value_or(""), value);
}

TEST(EnvironmentTest, UnsetEnv1) {
  auto [key, value] = MK_ENV();
  env::setenv(key, value);
  std::vector<char> stdout_;
#if defined(_WIN32)
  process::run("cmd.exe", "/c", "<nul set /p=%" + key + "%&exit /b 0",
               $stdout > stdout_);
#else
  process::run("bash", "-c", "echo -n $" + key, $stdout > stdout_);
#endif
  ASSERT_EQ(std::string_view(stdout_.data(), stdout_.size()), value);

  env::unsetenv(key);
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
