
#include <gtest/gtest.h>

#include <environment/environment.hpp>

TEST(TestEnv, Unicode) {
  auto key = "名字";
  auto value = "哇哈哈";

#if defined(_WIN32) && (defined(UNICODE) || defined(_UNICODE))
  auto wkey = L"名字";
  auto wvalue = L"哇哈哈";
#endif

  ASSERT_FALSE(environment::getenv(key).has_value());
  ASSERT_TRUE(environment::setenv(key, value));
  ASSERT_TRUE(environment::getenv(key).has_value());
  ASSERT_EQ(environment::getenv(key).value(), value);
#if defined(UNICODE) || defined(_UNICODE)
  ASSERT_TRUE(environment::getenv(wkey).has_value());
  ASSERT_EQ(environment::getenv(wkey).value(), wvalue);
#endif

  environment::unsetenv(key);
  ASSERT_FALSE(environment::getenv(key).has_value());
}

TEST(TestEnv, Unicode2) {
  ASSERT_FALSE(environment::environments().empty());
#if defined(_WIN32) && (defined(UNICODE) || defined(_UNICODE))
  ASSERT_FALSE(environment::environments<std::wstring>().empty());
#endif
}
