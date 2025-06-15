
#include <gtest/gtest.h>

#include <environment/environment.hpp>

TEST(TestEnv, Unicode) {
  auto key = "名字";
  auto value = "哇哈哈";

#if defined(_WIN32) && defined(UNICODE)
  auto wkey = L"名字";
  auto wvalue = L"哇哈哈";
#endif

  ASSERT_FALSE(env::get(key).has_value());
  ASSERT_TRUE(env::set(key, value));
  ASSERT_TRUE(env::get(key).has_value());
  ASSERT_EQ(env::get(key).value(), value);
#if defined(UNICODE)
  ASSERT_TRUE(env::get(wkey).has_value());
  ASSERT_EQ(env::get(wkey).value(), wvalue);
#endif

  env::unset(key);
  ASSERT_FALSE(env::get(key).has_value());
}

TEST(TestEnv, Unicode2) {
  ASSERT_FALSE(env::all().empty());
#if defined(_WIN32) && defined(UNICODE)
  ASSERT_FALSE(env::all<std::wstring>().empty());
#endif
}

TEST(TestEnv, MoreUnicode) {
  // Test with a mix of different Unicode characters
  auto key_mixed = "EnvVar_你好_你好_Привіт_こんにちは";
  auto value_mixed = "Value_你好_你好_Привіт_こんにちは_😊";
  env::set(key_mixed, value_mixed);
  auto retrieved_value = env::get(key_mixed);
  ASSERT_TRUE(retrieved_value.has_value());
  ASSERT_EQ(retrieved_value.value(), value_mixed);
  env::unset(key_mixed);
  ASSERT_FALSE(env::get(key_mixed).has_value());
}

TEST(TestEnv, UnicodeEmptyValue) {
  auto key = "UnicodeKeyWithEmptyValue";
  auto value = "";
  ASSERT_TRUE(env::set(key, value));
  auto retrieved_value = env::get(key);
  ASSERT_TRUE(retrieved_value.has_value());
  ASSERT_EQ(retrieved_value.value(), value);
  env::unset(key);
}

TEST(TestEnv, UnicodeNoOverwrite) {
  auto key = "UnicodeNoOverwrite";
  auto value1 = "InitialValue";
  auto value2 = "NewValue";

  ASSERT_TRUE(env::set(key, value1));
  ASSERT_TRUE(env::get(key).has_value());
  ASSERT_EQ(env::get(key).value(), value1);

  // Try to set with overwrite = false, it should not change the value
  ASSERT_TRUE(env::set(key, value2, false));
  ASSERT_TRUE(env::get(key).has_value());
  ASSERT_EQ(env::get(key).value(), value1);

  // Now set with overwrite = true
  ASSERT_TRUE(env::set(key, value2, true));
  ASSERT_TRUE(env::get(key).has_value());
  ASSERT_EQ(env::get(key).value(), value2);

  env::unset(key);
}

#if defined(_WIN32) && defined(UNICODE)
TEST(TestEnv, EnvironmentsWithUnicode) {
  auto key = "你好世界";
  auto value = "Hello World From Unicode";
  ASSERT_TRUE(env::set(key, value));

  std::wstring wkey = L"你好世界";
  // On Windows, environment variable keys are case-insensitive.
  // The environs() function returns upper-cased keys.
  std::transform(wkey.begin(), wkey.end(), wkey.begin(), ::towupper);

  auto envs_w = env::all<std::wstring>();
  auto it_w = envs_w.find(wkey);
  ASSERT_NE(it_w, envs_w.end());
  ASSERT_EQ(it_w->second, L"Hello World From Unicode");

  // Also check the std::string version
  // This is a bit tricky for non-ASCII uppercase. For this test, we assume
  // that the key is found in its original form if uppercasing is complex.
  // A better approach would be a proper UTF-8 uppercasing function if needed.
  // For now, let's test with an ASCII key and Unicode value.

  auto key_ascii = "ASCII_KEY_你好";
  auto value_unicode = "Unicode_Value_你好_Привіт";
  ASSERT_TRUE(env::set(key_ascii, value_unicode));
  auto envs_s = env::all<std::string>();

  // transform key_ascii to upper case
  std::string upper_key_ascii = key_ascii;
  std::transform(upper_key_ascii.begin(), upper_key_ascii.end(),
                 upper_key_ascii.begin(), ::toupper);

  auto it_s = envs_s.find(upper_key_ascii);
  ASSERT_NE(it_s, envs_s.end());
  ASSERT_EQ(it_s->second, value_unicode);

  env::unset(key);
  env::unset(key_ascii);
}
#endif
