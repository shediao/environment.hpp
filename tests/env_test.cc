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
  auto envs = environment::allenv();
  ASSERT_GT(envs.size(), 0);
#if defined(_WIN32) && defined(UNICODE)
  ASSERT_TRUE(
      (std::is_same_v<decltype(envs)::value_type::second_type, std::wstring>));
#else
  ASSERT_TRUE(
      (std::is_same_v<decltype(envs)::value_type::second_type, std::string>));
#endif
  for (auto const& [key, value] : envs) {
    auto env_ = environment::getenv(key);
    ASSERT_TRUE(env_.has_value());
    ASSERT_EQ(value, env_.value());
  }
}

TEST(EnvironmentTest, GenEnv1) {
  ASSERT_TRUE(environment::getenv("PATH").has_value());
#if defined(_WIN32)
  ASSERT_TRUE(environment::getenv("USERPROFILE").has_value());
#else
  ASSERT_TRUE(environment::getenv("HOME").has_value());
  ASSERT_TRUE(environment::getenv("USER").has_value());
#endif
}

TEST(EnvironmentTest, SetEnv1) {
  auto [key, value] = MK_ENV();
  environment::setenv(key, value);
  std::vector<char> stdout_;
#if defined(_WIN32)
  process::run("cmd.exe", "/c", "<nul set /p=%" + key + "%&exit /b 0",
               $stdout > stdout_);
#else
  process::run("bash", "-c", "echo -n $" + key, $stdout > stdout_);
#endif
  ASSERT_EQ(std::string_view(stdout_.data(), stdout_.size()), value);

  ASSERT_TRUE(environment::getenv(key).has_value());
  ASSERT_EQ(environment::getenv(key).value_or(""), value);
}

TEST(EnvironmentTest, UnsetEnv1) {
  auto [key, value] = MK_ENV();
  environment::setenv(key, value);
  std::vector<char> stdout_;
#if defined(_WIN32)
  process::run("cmd.exe", "/c", "<nul set /p=%" + key + "%&exit /b 0",
               $stdout > stdout_);
#else
  process::run("bash", "-c", "echo -n $" + key, $stdout > stdout_);
#endif
  ASSERT_EQ(std::string_view(stdout_.data(), stdout_.size()), value);

  environment::unsetenv(key);
  stdout_.clear();
#if defined(_WIN32)
  process::run("cmd.exe", "/c", "<nul set /p=%" + key + "%&exit /b 0",
               $stdout > stdout_);
#else
  process::run("bash", "-c", "echo -n $" + key, $stdout > stdout_);
#endif
  ASSERT_TRUE(stdout_.empty());
}
