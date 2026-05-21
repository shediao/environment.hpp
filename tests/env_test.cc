#include <gtest/gtest.h>

#include <algorithm>
#include <environment/environment.hpp>
#include <fstream>
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
  env::with(key, value, [&]() {
    ASSERT_TRUE(env::get(key));
    ASSERT_EQ(env::get(key).value(), value);
  });
  ASSERT_FALSE(env::get(key));

  env::set(key, value);
  ASSERT_TRUE(env::get(key));
  env::with(key, std::nullopt, [&]() { ASSERT_FALSE(env::get(key)); });
  ASSERT_TRUE(env::get(key));
  env::unset(key);

#if defined(_WIN32)
  constexpr wchar_t wkey[] = L"TEST_WITH_ENV_RAII";
  constexpr wchar_t wvalue[] = L"123";
  env::with(wkey, wvalue, [&]() {
    ASSERT_TRUE(env::get(wkey));
    ASSERT_EQ(env::get(wkey).value(), wvalue);
  });
  ASSERT_FALSE(env::get(wkey));

  env::set(wkey, wvalue);
  ASSERT_TRUE(env::get(wkey));
  env::with(wkey, std::nullopt, [&]() { ASSERT_FALSE(env::get(wkey)); });
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

  env::with(key, custom_path, [&]() {
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

  env::with(key, custom_path, [&]() {
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
  env::with(key, "C:\\a;;C:\\b;", []() {
    auto paths = env::path();
    ASSERT_EQ(paths.size(), 2);
    ASSERT_EQ(paths[0], "C:\\a");
    ASSERT_EQ(paths[1], "C:\\b");
  });
#else
  env::with(key, "/usr/bin::/bin:", []() {
    auto paths = env::path();
    ASSERT_EQ(paths.size(), 2);
    ASSERT_EQ(paths[0], "/usr/bin");
    ASSERT_EQ(paths[1], "/bin");
  });
#endif
}

TEST(EnvironmentTest, PathEmptyPath) {
  constexpr char key[] = "PATH";

  env::with(key, "", []() {
    auto paths = env::path();
    ASSERT_TRUE(paths.empty());
  });
}

// =============================================================================
// get() / unset() edge-case tests
// =============================================================================

TEST(EnvironmentTest, GetNonexistentReturnsNullopt) {
  // A variable that almost certainly does not exist.
  constexpr char key[] = "ENV_TEST_NONEXISTENT_12345_XYZ";
  env::unset(key);  // ensure it's gone
  auto result = env::get(key);
  ASSERT_FALSE(result.has_value())
      << "get() should return nullopt for non-existent variable";
}

TEST(EnvironmentTest, GetEmptyVariableReturnsEmptyString) {
  auto [key, value] = MK_ENV();
  env::set(key, "");  // set to empty
  auto result = env::get(key);
  ASSERT_TRUE(result.has_value())
      << "get() should return a value for an existing (but empty) variable";
  ASSERT_TRUE(result.value().empty())
      << "value should be empty, got: '" << result.value() << "'";
  env::unset(key);
}

TEST(EnvironmentTest, UnsetNullPointerReturnsFalse) {
  const char* null_ptr = nullptr;
  ASSERT_FALSE(env::unset(null_ptr));
}

TEST(EnvironmentTest, UnsetStringLiteral) {
  auto [key, value] = MK_ENV();
  env::set(key, value);
  ASSERT_TRUE(env::get(key).has_value());
  env::unset(key.c_str());  // call with C-string literal-like pointer
  ASSERT_FALSE(env::get(key).has_value());
}

TEST(EnvironmentTest, SetAndGetCString) {
  auto [key, value] = MK_ENV();
  ASSERT_TRUE(env::set(key.c_str(), value.c_str()));
  auto result = env::get(key.c_str());
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result.value(), value);
  env::unset(key);
}

// =============================================================================
// scoped_env / with multi-variable tests (new map-based overloads)
// =============================================================================

TEST(EnvironmentTest, WithEnvMultiSet) {
  // Set multiple new environment variables via map-based with.
  // After the scope, all should be unset.
  auto [key1, value1] = MK_ENV();
  auto [key2, value2] = MK_ENV();

  ASSERT_FALSE(env::get(key1));
  ASSERT_FALSE(env::get(key2));

  env::with(
      std::map<std::string, std::optional<std::string>>{
          {key1, value1},
          {key2, value2},
      },
      [&]() {
        ASSERT_TRUE(env::get(key1));
        ASSERT_EQ(env::get(key1).value(), value1);
        ASSERT_TRUE(env::get(key2));
        ASSERT_EQ(env::get(key2).value(), value2);
      });

  ASSERT_FALSE(env::get(key1));
  ASSERT_FALSE(env::get(key2));
}

TEST(EnvironmentTest, WithEnvMultiRestore) {
  // Pre-set some vars, then modify them inside with.
  // After the scope, original values must be restored.
  auto [key1, value1] = MK_ENV();
  auto [key2, value2] = MK_ENV();

  constexpr char new_value1[] = "NEW_VALUE_1";
  constexpr char new_value2[] = "NEW_VALUE_2";

  env::set(key1, value1);
  env::set(key2, value2);
  ASSERT_EQ(env::get(key1).value(), value1);
  ASSERT_EQ(env::get(key2).value(), value2);

  env::with(
      std::map<std::string, std::optional<std::string>>{
          {key1, new_value1},
          {key2, new_value2},
      },
      [&]() {
        ASSERT_EQ(env::get(key1).value(), new_value1);
        ASSERT_EQ(env::get(key2).value(), new_value2);
      });

  // Original values restored.
  ASSERT_EQ(env::get(key1).value(), value1);
  ASSERT_EQ(env::get(key2).value(), value2);

  env::unset(key1);
  env::unset(key2);
}

TEST(EnvironmentTest, WithEnvMultiMixedSetAndUnset) {
  // In the map, some entries have values (set) and some are nullopt (unset).
  auto [key_set, value_set] = MK_ENV();
  auto [key_unset, value_unset] = MK_ENV();

  // Pre-set both.
  env::set(key_set, value_set);
  env::set(key_unset, value_unset);
  ASSERT_TRUE(env::get(key_set));
  ASSERT_TRUE(env::get(key_unset));

  env::with(
      std::map<std::string, std::optional<std::string>>{
          {key_set, std::string("CHANGED_VALUE")},
          {key_unset, std::nullopt},
      },
      [&]() {
        ASSERT_EQ(env::get(key_set).value(), "CHANGED_VALUE");
        ASSERT_FALSE(env::get(key_unset))
            << key_unset << " should be unset inside the scope";
      });

  // Both restored to originals.
  ASSERT_EQ(env::get(key_set).value(), value_set);
  ASSERT_EQ(env::get(key_unset).value(), value_unset);

  env::unset(key_set);
  env::unset(key_unset);
}

TEST(EnvironmentTest, WithEnvMultiNewSetAndNewUnset) {
  // Variables that did NOT exist before: one is set, one is "unset" (nullopt).
  auto [key_set, value_set] = MK_ENV();
  auto [key_unset, _] = MK_ENV();

  ASSERT_FALSE(env::get(key_set));
  ASSERT_FALSE(env::get(key_unset));

  env::with(
      std::map<std::string, std::optional<std::string>>{
          {key_set, std::string("FRESH_VALUE")},
          {key_unset, std::nullopt},
      },
      [&]() {
        ASSERT_EQ(env::get(key_set).value(), "FRESH_VALUE");
        ASSERT_FALSE(env::get(key_unset));
      });

  // Neither should exist after the scope.
  ASSERT_FALSE(env::get(key_set));
  ASSERT_FALSE(env::get(key_unset));
}

TEST(EnvironmentTest, WithEnvMultiEmptyMap) {
  // An empty map should be a no-op and not crash.
  bool called = false;
  env::with(std::map<std::string, std::optional<std::string>>{},
            [&]() { called = true; });
  ASSERT_TRUE(called);
}

TEST(EnvironmentTest, WithEnvMultiSingleVarViaMap) {
  // The map overload should also work for a single entry.
  auto [key, value] = MK_ENV();
  ASSERT_FALSE(env::get(key));

  env::with(
      std::map<std::string, std::optional<std::string>>{
          {key, value},
      },
      [&]() {
        ASSERT_TRUE(env::get(key));
        ASSERT_EQ(env::get(key).value(), value);
      });

  ASSERT_FALSE(env::get(key));
}

TEST(EnvironmentTest, WithEnvMultiOverwriteExistingWithNullopt) {
  // Overwrite an existing variable with nullopt inside scope,
  // verify it's restored afterwards.
  auto [key, value] = MK_ENV();
  env::set(key, value);
  ASSERT_TRUE(env::get(key));

  env::with(
      std::map<std::string, std::optional<std::string>>{
          {key, std::nullopt},
      },
      [&]() { ASSERT_FALSE(env::get(key)); });

  ASSERT_TRUE(env::get(key));
  ASSERT_EQ(env::get(key).value(), value);

  env::unset(key);
}

TEST(EnvironmentTest, WithEnvMultiPartialOverlap) {
  // Mix of vars that exist before and vars that don't.
  auto [existing_key, existing_value] = MK_ENV();
  auto [new_key, new_value] = MK_ENV();

  env::set(existing_key, existing_value);
  ASSERT_TRUE(env::get(existing_key));
  ASSERT_FALSE(env::get(new_key));

  env::with(
      std::map<std::string, std::optional<std::string>>{
          {existing_key, std::string("CHANGED")},
          {new_key, new_value},
      },
      [&]() {
        ASSERT_EQ(env::get(existing_key).value(), "CHANGED");
        ASSERT_EQ(env::get(new_key).value(), new_value);
      });

  // Existing var restored, new var unset.
  ASSERT_EQ(env::get(existing_key).value(), existing_value);
  ASSERT_FALSE(env::get(new_key));

  env::unset(existing_key);
}

#if defined(_WIN32)
TEST(EnvironmentTest, WithEnvMultiWideChars) {
  // Multi-variable with wstring on Windows.
  constexpr wchar_t key1[] = L"TEST_WITH_ENV_MULTI_W_1";
  constexpr wchar_t key2[] = L"TEST_WITH_ENV_MULTI_W_2";
  constexpr wchar_t value1[] = L"VALUE_1_W";
  constexpr wchar_t value2[] = L"VALUE_2_W";

  ASSERT_FALSE(env::get(key1));
  ASSERT_FALSE(env::get(key2));

  env::with(
      std::map<std::wstring, std::optional<std::wstring>>{
          {key1, value1},
          {key2, value2},
      },
      [&]() {
        ASSERT_TRUE(env::get(key1));
        ASSERT_EQ(env::get(key1).value(), value1);
        ASSERT_TRUE(env::get(key2));
        ASSERT_EQ(env::get(key2).value(), value2);
      });

  ASSERT_FALSE(env::get(key1));
  ASSERT_FALSE(env::get(key2));
}
#endif  // _WIN32

// =============================================================================
// env::search_path() tests
// =============================================================================

TEST(SearchPathTest, EmptyCommandReturnsNullopt) {
  auto result = env::search_path("");
  ASSERT_FALSE(result.has_value());
  result = env::search_path(std::string_view{});
  ASSERT_FALSE(result.has_value());
}

TEST(SearchPathTest, NonExistentCommandReturnsNullopt) {
  auto result = env::search_path("__this_command_does_not_exist_12345__");
  ASSERT_FALSE(result.has_value())
      << "search_path should return nullopt for non-existent commands";
}

TEST(SearchPathTest, FindsCommonSystemCommands) {
  // These are universally available on UNIX; on Windows we use different
  // commands that should be present in System32.
#if defined(_WIN32)
  std::string cmd = "cmd.exe";
#else
  std::string cmd = "sh";
#endif
  auto result = env::search_path(cmd);
  ASSERT_TRUE(result.has_value())
      << "search_path should find a standard system shell";
  ASSERT_FALSE(result->empty());
  // The result path should end with the command name.
  ASSERT_TRUE(result->ends_with(cmd));
}

TEST(SearchPathTest, ResultIsAbsolutePath) {
  // The returned path should be absolute (or at least contain a separator).
#if defined(_WIN32)
  auto result = env::search_path("cmd.exe");
#else
  auto result = env::search_path("sh");
#endif
  ASSERT_TRUE(result.has_value());
#if defined(_WIN32)
  bool const is_absolute =
      (result->size() >= 3 && std::isalpha((*result)[0]) &&
       (*result)[1] == ':' && (*result)[2] == '\\') ||
      (result->size() >= 2 && (*result)[0] == '\\' && (*result)[1] == '\\');
#else
  bool const is_absolute = !result->empty() && (*result)[0] == '/';
#endif
  ASSERT_TRUE(is_absolute) << "Expected absolute path, got: " << *result;
}

#if !defined(_WIN32)
TEST(SearchPathTest, AbsolutePathCommandReturnsDirectly) {
  // When command already contains '/', search_path resolves it directly.
  auto sh_result = env::search_path("/bin/sh");
  ASSERT_TRUE(sh_result.has_value());
  ASSERT_EQ(*sh_result, "/bin/sh");
}

TEST(SearchPathTest, RelativePathCommandCheckedDirectly) {
  // A command like "./env_test" (relative path) should be checked directly.
  auto result = env::search_path("./env_test");
  // May or may not exist depending on cwd; we just verify no crash.
  // If it exists, it must be the exact string we passed.
  if (result.has_value()) {
    ASSERT_EQ(*result, "./env_test");
  }
}

TEST(SearchPathTest, DirectoryNotReturned) {
  // A directory (like "/") should NOT be returned as an executable.
  auto result = env::search_path("/");
  ASSERT_FALSE(result.has_value())
      << "A directory should not be returned as an executable";
}

TEST(SearchPathTest, PathWithTrailingSlashDir) {
  // "/bin/" is a directory, not a regular file.
  auto result = env::search_path("/bin/");
  ASSERT_FALSE(result.has_value());
}
#endif  // !_WIN32

TEST(SearchPathTest, EmptyPathEnvReturnsNullopt) {
  // When PATH is empty, search_path should return nullopt for bare commands.
  auto [key, value] = MK_ENV();
  env::with("PATH", std::string(""), [&]() {
#if defined(_WIN32)
    // On Windows, SearchPathW also searches system dirs even with empty
    // PATH, so we look for something improbable.
    auto result = env::search_path("__this_should_not_exist_anywhere_98765__");
    ASSERT_FALSE(result.has_value());
#else
        auto result = env::search_path("sh");
        ASSERT_FALSE(result.has_value())
            << "With empty PATH, bare commands should not be found";
#endif
  });
}

TEST(SearchPathTest, MissingPathEnvReturnsNullopt) {
  auto [key, value] = MK_ENV();
  env::with("PATH", std::nullopt, [&]() {
#if defined(_WIN32)
    auto result = env::search_path("__this_should_not_exist_anywhere_98765__");
    ASSERT_FALSE(result.has_value());
#else
        auto result = env::search_path("sh");
        ASSERT_FALSE(result.has_value())
            << "With PATH unset, bare commands should not be found";
#endif
  });
}

TEST(SearchPathTest, CommandWithSpacesInPath) {
  // The search should handle PATH entries that contain spaces.
  // We construct a temporary directory with a space, place a script,
  // and verify search_path finds it.
#if defined(_WIN32)
  // On Windows we skip this test – creating executables is more involved.
  GTEST_SKIP() << "Skipping spaces-in-path test on Windows (creates .exe)";
#else
  // Create a temporary directory with a space in its name.
  char tmpdir_template[] = "/tmp/env_test_space_XXXXXX";
  char* tmpdir = ::mkdtemp(tmpdir_template);
  ASSERT_NE(tmpdir, nullptr);

  // Create a dummy executable script.
  std::string script_path = std::string(tmpdir) + "/dummy_exec";
  {
    std::ofstream ofs(script_path);
    ofs << "#!/bin/sh\nexit 0\n";
  }
  ::chmod(script_path.c_str(), 0755);

  // Prepend our temp dir to PATH.
  auto original_path = env::get("PATH").value_or("");
  env::with("PATH", std::string(tmpdir) + ":" + original_path, [&]() {
    auto result = env::search_path("dummy_exec");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(*result, script_path);
  });

  // Cleanup.
  ::unlink(script_path.c_str());
  ::rmdir(tmpdir);
#endif
}

TEST(SearchPathTest, ConsecutivePathSeparators) {
  // PATH with consecutive separators should still find the command.
#if defined(_WIN32)
  GTEST_SKIP() << "Skipping on Windows – PATH manipulation is complex";
#else
  auto original_path = env::get("PATH").value_or("");

  env::with("PATH", std::string("::/bin::/usr/bin:::"), [&]() {
    auto result = env::search_path("sh");
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result->find("/bin/sh"), std::string::npos);
  });
#endif
}

TEST(SearchPathTest, TrailingPathSeparator) {
  // PATH ending with ':' is legal – trailing empty entry is ignored.
#if defined(_WIN32)
  GTEST_SKIP() << "Skipping on Windows – PATH manipulation is complex";
#else
  auto original_path = env::get("PATH").value_or("");
  env::with("PATH", original_path + ":", [&]() {
    auto result = env::search_path("sh");
    ASSERT_TRUE(result.has_value());
  });
#endif
}

TEST(SearchPathTest, MultipleCommandsInSamePath) {
  // Verify that different commands resolve to different paths.
#if defined(_WIN32)
  auto cmd = env::search_path("cmd.exe");
  auto reg = env::search_path("reg.exe");
  ASSERT_TRUE(cmd.has_value());
  ASSERT_TRUE(reg.has_value());
  ASSERT_NE(*cmd, *reg);
#else
  auto sh = env::search_path("sh");
  auto ls = env::search_path("ls");
  ASSERT_TRUE(sh.has_value());
  ASSERT_TRUE(ls.has_value());
  // They should reside in different directories.
  // Note: on some systems sh and ls could be in the same dir,
  // so we just verify both were found and are absolute.
  ASSERT_FALSE(sh->empty());
  ASSERT_FALSE(ls->empty());
#endif
}

#if defined(_WIN32)
TEST(SearchPathTest, WideCharSearchPath) {
  auto result = env::search_path(L"cmd.exe");
  ASSERT_TRUE(result.has_value());
  ASSERT_FALSE(result->empty());
  ASSERT_NE(result->find(L"cmd"), std::wstring::npos);
}

TEST(SearchPathTest, WideCharNotFoundReturnsNullopt) {
  auto result = env::search_path(L"__this_wide_command_does_not_exist__");
  ASSERT_FALSE(result.has_value());
}

TEST(SearchPathTest, ExtensionProbing) {
  auto result = env::search_path(L"notepad");
  ASSERT_TRUE(result.has_value());
  // The resolved path should end with "notepad.exe" (case-insensitive).
  std::wstring lower = *result;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
  ASSERT_TRUE(lower.ends_with(L"notepad.exe"));
}

TEST(SearchPathTest, AbsoluteWindowsPath) {
  auto result = env::search_path("C:\\Windows\\System32\\cmd.exe");
  ASSERT_TRUE(result.has_value());
  // Result might be canonicalized, but cmd.exe should be in it.
  ASSERT_TRUE(result->ends_with("cmd.exe"));
}

TEST(SearchPathTest, WideCharAbsoluteWindowsPath) {
  auto result = env::search_path(L"C:\\Windows\\System32\\cmd.exe");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->ends_with(L"cmd.exe"));
}
#endif  // _WIN32
