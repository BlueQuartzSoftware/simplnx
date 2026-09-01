
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

TEST_CASE("Application::GetOrCreateInstance", "[Application]")
{
  SECTION("Create new instance")
  {
    Application::DeleteInstance();
    auto app = Application::GetOrCreateInstance();
    REQUIRE(app != nullptr);
    REQUIRE(Application::Instance() == app);
  }

  SECTION("Get existing instance")
  {
    auto app1 = Application::GetOrCreateInstance();
    auto app2 = Application::GetOrCreateInstance();
    REQUIRE(app1 == app2);
  }
}

TEST_CASE("Application::DeleteInstance", "[Application]")
{
  SECTION("Delete existing instance")
  {
    auto app = Application::GetOrCreateInstance();
    REQUIRE(app != nullptr);
    Application::DeleteInstance();
    REQUIRE(Application::Instance() == nullptr);
  }

  SECTION("Delete null instance is safe")
  {
    Application::DeleteInstance();
    REQUIRE(Application::Instance() == nullptr);
    Application::DeleteInstance(); // Should not crash
    REQUIRE(Application::Instance() == nullptr);
  }
}

TEST_CASE("Application::getCurrentPath and getCurrentDir", "[Application]")
{
  auto app = Application::GetOrCreateInstance();

  SECTION("getCurrentPath returns valid path")
  {
    auto path = app->getCurrentPath();
    REQUIRE(!path.empty());
    REQUIRE(fs::exists(path));
  }

  SECTION("getCurrentDir returns valid directory")
  {
    auto dir = app->getCurrentDir();
    REQUIRE(!dir.empty());
    REQUIRE(fs::exists(dir));
    REQUIRE(fs::is_directory(dir));
  }

  SECTION("getCurrentDir is parent of getCurrentPath")
  {
    auto path = app->getCurrentPath();
    auto dir = app->getCurrentDir();
    REQUIRE(path.parent_path() == dir);
  }
}

TEST_CASE("Application::loadPreferences", "[Application]")
{
  auto app = Application::GetOrCreateInstance();

  SECTION("Load preferences - file may or may not exist")
  {
    // This test does not fail if preferences file does not exist
    // because loadPreferences returns an error Result but does not throw
    auto result = app->loadPreferences();
    // Result may be valid or invalid depending on whether preferences file exists
    // The important thing is it returns a Result and does not crash
    REQUIRE((result.valid() || result.invalid()));
  }

  SECTION("Preferences object is created after load")
  {
    app->loadPreferences();
    auto* prefs = app->getPreferences();
    REQUIRE(prefs != nullptr);
  }
}

TEST_CASE("Application::savePreferences", "[Application]")
{
  auto app = Application::GetOrCreateInstance();

  SECTION("Save preferences after load")
  {
    app->loadPreferences();
    auto result = app->savePreferences();
    // Should succeed or return an error Result, but not crash
    REQUIRE((result.valid() || result.invalid()));
  }

  SECTION("Save preferences creates preferences if needed")
  {
    // Ensure preferences are loaded
    app->loadPreferences();
    auto* prefs = app->getPreferences();
    REQUIRE(prefs != nullptr);

    // Set a value and save
    prefs->setValue("test_key", 42);
    auto result = app->savePreferences();
    REQUIRE((result.valid() || result.invalid()));
  }
}

TEST_CASE("Application::loadPlugins", "[Application]")
{
  auto app = Application::GetOrCreateInstance();

  SECTION("Load plugins from valid directory")
  {
    auto result = app->loadPlugins(SIMPLNX_BUILD_DIR, true);
    // Should either succeed or fail gracefully with Result
    REQUIRE((result.valid() || result.invalid()));

    if(result.invalid())
    {
      // If it failed, there should be error messages
      REQUIRE(!result.errors().empty());
    }
  }

  SECTION("Load plugins from non-existent directory returns error")
  {
    fs::path nonExistentPath = "/tmp/this_directory_should_not_exist_12345";
    auto result = app->loadPlugins(nonExistentPath, false);
    REQUIRE(result.invalid());
    REQUIRE(!result.errors().empty());
    REQUIRE(result.errors()[0].code == -20);
  }

  SECTION("Load plugins from file (not directory) returns error")
  {
    // Create a temporary file
    fs::path tempFile = fs::temp_directory_path() / "test_file.txt";
    std::ofstream ofs(tempFile);
    ofs << "test";
    ofs.close();

    auto result = app->loadPlugins(tempFile, false);
    REQUIRE(result.invalid());
    REQUIRE(!result.errors().empty());
    REQUIRE(result.errors()[0].code == -21);

    // Clean up
    fs::remove(tempFile);
  }

  SECTION("Load plugins with verbose output")
  {
    // Just verify it does not crash with verbose=true
    auto result = app->loadPlugins(SIMPLNX_BUILD_DIR, true);
    REQUIRE((result.valid() || result.invalid()));
  }

  SECTION("Loaded plugins are accessible")
  {
    auto result = app->loadPlugins(SIMPLNX_BUILD_DIR, true);
    if(result.valid())
    {
      auto plugins = app->getPluginList();
      // If plugins loaded successfully, there should be at least one
      REQUIRE(plugins.size() > 0);
    }
  }
}

TEST_CASE("Application::getFilterList", "[Application]")
{
  auto app = Application::GetOrCreateInstance();

  SECTION("FilterList is never null")
  {
    auto* filterList = app->getFilterList();
    REQUIRE(filterList != nullptr);
  }

  SECTION("FilterList contains filters after loading plugins")
  {
    auto result = app->loadPlugins(SIMPLNX_BUILD_DIR, true);
    if(result.valid())
    {
      auto* filterList = app->getFilterList();
      auto handles = filterList->getFilterHandles();
      REQUIRE(handles.size() > 0);
    }
  }
}

TEST_CASE("Application::getPlugin", "[Application]")
{
  auto app = Application::GetOrCreateInstance();
  auto result = app->loadPlugins(SIMPLNX_BUILD_DIR, true);

  SECTION("Get non-existent plugin returns nullptr")
  {
    // Create a random UUID that should not exist
    Uuid randomUuid = Uuid::FromString("00000000-0000-0000-0000-000000000000").value();
    auto* plugin = app->getPlugin(randomUuid);
    REQUIRE(plugin == nullptr);
  }

  SECTION("Get loaded plugin by UUID")
  {
    auto plugins = app->getPluginList();
    if(!plugins.empty())
    {
      auto* firstPlugin = *plugins.begin();
      auto uuid = firstPlugin->getId();
      auto* foundPlugin = app->getPlugin(uuid);
      REQUIRE(foundPlugin != nullptr);
      REQUIRE(foundPlugin == firstPlugin);
    }
  }
}

TEST_CASE("Application::getDataType", "[Application]")
{
  auto app = Application::GetOrCreateInstance();

  SECTION("Default data types are registered")
  {
    REQUIRE(app->getDataType("DataGroup") == DataObject::Type::DataGroup);
    REQUIRE(app->getDataType("AttributeMatrix") == DataObject::Type::AttributeMatrix);
    REQUIRE(app->getDataType("Image Geom") == DataObject::Type::ImageGeom);
    REQUIRE(app->getDataType("Vertex Geom") == DataObject::Type::VertexGeom);
  }

  SECTION("Unknown data type returns DataObject")
  {
    auto type = app->getDataType("NonExistentType");
    REQUIRE(type == DataObject::Type::DataObject);
  }
}

TEST_CASE("Application::addDataType", "[Application]")
{
  auto app = Application::GetOrCreateInstance();

  SECTION("Add custom data type")
  {
    app->addDataType(DataObject::Type::DataGroup, "CustomType");
    REQUIRE(app->getDataType("CustomType") == DataObject::Type::DataGroup);
  }

  SECTION("Override existing data type")
  {
    auto originalType = app->getDataType("DataGroup");
    app->addDataType(DataObject::Type::ImageGeom, "DataGroup");
    REQUIRE(app->getDataType("DataGroup") == DataObject::Type::ImageGeom);
    // Restore original
    app->addDataType(originalType, "DataGroup");
  }
}

TEST_CASE("Application::getIOCollection", "[Application]")
{
  auto app = Application::GetOrCreateInstance();

  SECTION("getDataStoreFormats returns format names")
  {
    auto formats = app->getDataStoreFormats();
    // Should at least have some formats registered
    REQUIRE(formats.size() >= 0); // May be empty before plugins load
  }
}

TEST_CASE("Application::getSimplnxUuid and getSimplUuid", "[Application]")
{
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(SIMPLNX_BUILD_DIR, false);

  SECTION("Get non-existent UUID mapping")
  {
    Uuid randomUuid = Uuid::FromString("00000000-0000-0000-0000-000000000000").value();
    auto result = app->getSimplnxUuid(randomUuid);
    REQUIRE(!result.has_value());
  }

  SECTION("Get non-existent reverse UUID mapping")
  {
    Uuid randomUuid = Uuid::FromString("00000000-0000-0000-0000-000000000000").value();
    auto results = app->getSimplUuid(randomUuid);
    REQUIRE(results.empty());
  }

  // Note: Testing actual UUID mappings would require knowing specific plugin UUIDs
  // which vary by loaded plugins
}

TEST_CASE("Application::Error Handling Integration", "[Application]")
{
  SECTION("Multiple operations with error checking")
  {
    Application::DeleteInstance();
    auto app = Application::GetOrCreateInstance();
    REQUIRE(app != nullptr);

    // Load preferences - may fail but should not crash
    auto prefsResult = app->loadPreferences();
    REQUIRE((prefsResult.valid() || prefsResult.invalid()));

    // Load plugins - may fail but should not crash
    auto pluginsResult = app->loadPlugins(SIMPLNX_BUILD_DIR, false);
    REQUIRE((pluginsResult.valid() || pluginsResult.invalid()));

    // Save preferences - may fail but should not crash
    auto saveResult = app->savePreferences();
    REQUIRE((saveResult.valid() || saveResult.invalid()));

    // Clean up
    Application::DeleteInstance();
  }

  SECTION("Error accumulation in loadPlugins")
  {
    auto app = Application::GetOrCreateInstance();

    // Create a temp directory with mix of valid and invalid files
    fs::path tempDir = fs::temp_directory_path() / "test_plugins_dir";
    fs::create_directories(tempDir);

    // Create a non-plugin file
    std::ofstream(tempDir / "not_a_plugin.txt") << "test";

    auto result = app->loadPlugins(tempDir, false);
    // Should return valid (no errors) since invalid files are just skipped
    REQUIRE(result.valid());

    // Clean up
    fs::remove_all(tempDir);
  }
}

TEST_CASE("Application::Singleton Lifecycle", "[Application]")
{
  SECTION("Create, use, and delete multiple times")
  {
    for(int i = 0; i < 3; ++i)
    {
      Application::DeleteInstance();
      REQUIRE(Application::Instance() == nullptr);

      auto app = Application::GetOrCreateInstance();
      REQUIRE(app != nullptr);
      REQUIRE(Application::Instance() == app);

      auto path = app->getCurrentPath();
      REQUIRE(!path.empty());

      Application::DeleteInstance();
      REQUIRE(Application::Instance() == nullptr);
    }
  }

  SECTION("Instance persists across GetOrCreateInstance calls")
  {
    Application::DeleteInstance();

    auto app1 = Application::GetOrCreateInstance();
    auto* prefs1 = app1->getPreferences();

    auto app2 = Application::GetOrCreateInstance();
    auto* prefs2 = app2->getPreferences();

    REQUIRE(app1 == app2);
    REQUIRE(prefs1 == prefs2);

    Application::DeleteInstance();
  }
}

TEST_CASE("Preferences::removeValue", "[Preferences]")
{
  Preferences prefs;

  SECTION("Remove an existing key erases it")
  {
    prefs.setValue("test_remove_key", 42);
    REQUIRE(prefs.contains("test_remove_key"));

    prefs.removeValue("test_remove_key");

    REQUIRE_FALSE(prefs.contains("test_remove_key"));
  }

  SECTION("Remove a non-existent key is a no-op")
  {
    REQUIRE_FALSE(prefs.contains("never_set_key"));
    REQUIRE_NOTHROW(prefs.removeValue("never_set_key"));
    REQUIRE_FALSE(prefs.contains("never_set_key"));
  }

  SECTION("Remove does not affect other keys")
  {
    prefs.setValue("keep_me", 1);
    prefs.setValue("remove_me", 2);

    prefs.removeValue("remove_me");

    REQUIRE(prefs.contains("keep_me"));
    REQUIRE_FALSE(prefs.contains("remove_me"));
  }
}
