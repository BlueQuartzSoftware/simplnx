#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"

#include <catch2/catch.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace
{
// This deliberate extra owner tests the runner's failure-retention path.
std::shared_ptr<nx::core::Application> s_RetainedApplication;
} // namespace

TEST_CASE("Test profile saves and reloads an isolated witness", "[.TestProfile]")
{
  namespace fs = std::filesystem;
  const char* active = std::getenv("SIMPLNX_TEST_ACTIVE_PROFILE");
  REQUIRE(active != nullptr);
  const fs::path root(active);
  REQUIRE(root.is_absolute());
  REQUIRE(fs::is_regular_file(root / ".simplnx-test-profile"));
  REQUIRE(std::getenv("SIMPLNX_TEST_PROFILE_DIR") == nullptr);
  REQUIRE(std::getenv("SIMPLNX_TEST_PROFILE_TOKEN") == nullptr);
  REQUIRE(std::getenv("SIMPLNX_TEST_PROFILE_REPORT") == nullptr);
#ifdef _WIN32
  const auto expected = root / "AppData/Local/DREAM3DNX/preferences.json";
#elif defined(__APPLE__)
  const auto expected = root / "Library/Preferences/DREAM3DNX/preferences.json";
#else
  const auto expected = root / ".config/DREAM3DNX/preferences.json";
#endif
  REQUIRE(nx::core::Preferences::DefaultFilePath("DREAM3DNX") == expected);
  auto app = nx::core::Application::GetOrCreateInstance();
  app->getPreferences()->setValue("profile_round_trip", 731);
  REQUIRE(app->savePreferences().valid());
  nx::core::Preferences loaded;
  REQUIRE(loaded.loadFromFile(expected).valid());
  REQUIRE(loaded.value("profile_round_trip") == 731);
}

TEST_CASE("Test profile retains a failed run", "[.TestProfile]")
{
  REQUIRE(std::getenv("SIMPLNX_TEST_ACTIVE_PROFILE") != nullptr);
  FAIL("Deliberate profile retention probe");
}

TEST_CASE("Test profile detects a retained Application owner", "[.TestProfile]")
{
  s_RetainedApplication = nx::core::Application::GetOrCreateInstance();
  REQUIRE(s_RetainedApplication != nullptr);
}

TEST_CASE("Test profile detects an Application save failure", "[.TestProfile]")
{
  auto app = nx::core::Application::GetOrCreateInstance();
  REQUIRE(app != nullptr);
  const auto preferences = nx::core::Preferences::DefaultFilePath("DREAM3DNX");
  REQUIRE(std::filesystem::remove(preferences));
  REQUIRE(std::filesystem::create_directory(preferences));
}
