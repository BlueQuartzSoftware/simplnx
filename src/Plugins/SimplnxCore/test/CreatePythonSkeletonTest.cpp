#include <algorithm>
#include <catch2/catch.hpp>
#include <fstream>
#include <sstream>

#include "SimplnxCore/Filters/CreatePythonSkeletonFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "SimplnxCore/utils/PythonPluginTemplateFile.hpp"
//
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
/**
 * @brief Creates a new folder with a unique name in the test output folder and removes it at the end of the scope.
 */
class ScopedOutputDirectory
{
public:
  explicit ScopedOutputDirectory(const std::string& prefix)
  : m_Path(fs::path(unit_test::k_BinaryTestOutputDir.str()) / fmt::format("{}_{}", prefix, Uuid::GenerateV4().str()))
  {
    REQUIRE(fs::create_directories(m_Path));
  }

  ~ScopedOutputDirectory() noexcept
  {
    std::error_code errorCode;
    fs::remove_all(m_Path, errorCode);
  }

  ScopedOutputDirectory(const ScopedOutputDirectory&) = delete;
  ScopedOutputDirectory(ScopedOutputDirectory&&) = delete;
  ScopedOutputDirectory& operator=(const ScopedOutputDirectory&) = delete;
  ScopedOutputDirectory& operator=(ScopedOutputDirectory&&) = delete;

  const fs::path& path() const
  {
    return m_Path;
  }

private:
  fs::path m_Path;
};
} // namespace

TEST_CASE("SimplnxCore::GeneratePythonSkeleton")
{
  UnitTest::LoadPlugins();

  // DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/generate_vector_colors/6_6_generate_vector_colors.dream3d", unit_test::k_TestFilesDir)));
  DataStructure dataStructure;
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    CreatePythonSkeletonFilter filter;
    Arguments args;

    const std::filesystem::path k_OutputDirectory = fs::path(unit_test::k_BinaryTestOutputDir.str());
    const std::string k_PluginName = "ExampleToolbox";

    // Generate filters into a new plugin
    {
      const std::string k_PluginHumanName = "Example Toolbox";
      const std::string k_FilterNames = "FirstFilter,SecondFilter";

      // Create default Parameters for the filter.
      args.insertOrAssign(CreatePythonSkeletonFilter::k_UseExistingPlugin_Key, false);
      args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginName_Key, std::make_any<std::string>(k_PluginName));
      args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginHumanName_Key, std::make_any<std::string>(k_PluginHumanName));
      args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginOutputDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(k_OutputDirectory.string()));
      args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, std::make_any<std::string>(k_FilterNames));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    // Generate filters into an existing plugin
    {
      const std::string k_FilterNames = "ThirdFilter,FourthFilter";
      const std::filesystem::path k_InputDirectory = k_OutputDirectory / k_PluginName;

      // Create default Parameters for the filter.
      args.insertOrAssign(CreatePythonSkeletonFilter::k_UseExistingPlugin_Key, true);
      args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginInputDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(k_InputDirectory.string()));
      args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, std::make_any<std::string>(k_FilterNames));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreatePythonSkeletonFilter: Trimmed filter names", "[SimplnxCore][CreatePythonSkeletonFilter][L15]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  CreatePythonSkeletonFilter filter;
  const ScopedOutputDirectory outputDir("CreatePythonSkeleton_TrimmedNames");
  Arguments args;
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginOutputDirectory_Key, outputDir.path());
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginName_Key, std::string("TrimmedPlugin"));
  const auto pluginDir = outputDir.path() / "TrimmedPlugin";
  const auto sourceDir = pluginDir / "src" / "TrimmedPlugin";
  const bool existing = GENERATE(false, true);
  if(existing)
  {
    args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, std::string("SeedFilter"));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    args.insertOrAssign(CreatePythonSkeletonFilter::k_UseExistingPlugin_Key, true);
    args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginInputDirectory_Key, pluginDir);
  }
  const std::string names = GENERATE(std::string("FirstFilter, SecondFilter"), std::string(" \tFirstFilter\t ,\tSecondFilter \t"), std::string("FirstFilter,\r\nSecondFilter\r\n"));
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, names);
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  CHECK(preflight.outputValues.at(0).value.find((sourceDir / "SecondFilter.py").string()) != std::string::npos);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CHECK(fs::exists(sourceDir / "FirstFilter.py"));
  CHECK(fs::exists(sourceDir / "SecondFilter.py"));
  for(const auto& filename : {"Plugin.py", "__init__.py"})
  {
    std::ifstream input(sourceDir / filename);
    std::stringstream buffer;
    buffer << input.rdbuf();
    const auto text = buffer.str();
    CHECK(text.find("from .SecondFilter import SecondFilter\n") != std::string::npos);
    CHECK(text.find(" SecondFilter'") == std::string::npos);
    CHECK(text.find(". SecondFilter") == std::string::npos);
    if(std::string(filename) == "Plugin.py")
    {
      CHECK(text.find("_filters.append(SecondFilter)\n") != std::string::npos);
    }
    else
    {
      CHECK(text.find("__all__.append('SecondFilter')\n") != std::string::npos);
    }
  }
}

TEST_CASE("SimplnxCore::CreatePythonSkeletonFilter: Invalid filter names", "[SimplnxCore][CreatePythonSkeletonFilter][L15]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  CreatePythonSkeletonFilter filter;
  const std::string badEntry = GENERATE("1Bad", "Bad-Name", "Bad Name", "", " \t", "class", "None", "__debug__");
  const std::string names = GENERATE_COPY("A," + badEntry + ",B", "A," + badEntry);
  Arguments args;
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginOutputDirectory_Key, fs::path(unit_test::k_BinaryTestOutputDir.str()));
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, names);
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -105150);
  CHECK(result.outputActions.errors()[0].message.find("'" + badEntry + "'") != std::string::npos);
}

TEST_CASE("SimplnxCore::CreatePythonSkeletonFilter: Empty filter names create a plugin without filters", "[SimplnxCore][CreatePythonSkeletonFilter][L15]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  CreatePythonSkeletonFilter filter;
  const ScopedOutputDirectory outputDir("CreatePythonSkeleton_EmptyNames");
  const std::string names = GENERATE(std::string(""), std::string(" \t"));
  Arguments args;
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginOutputDirectory_Key, outputDir.path());
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginName_Key, std::string("EmptyPlugin"));
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, names);
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto sourceDir = outputDir.path() / "EmptyPlugin" / "src" / "EmptyPlugin";
  std::ifstream input(sourceDir / "Plugin.py");
  REQUIRE(input.is_open());
  std::stringstream buffer;
  buffer << input.rdbuf();
  const auto text = buffer.str();
  CHECK(text.find("_filters = []") != std::string::npos);
  CHECK(text.find("from .") == std::string::npos);
  CHECK(text.find("_filters.append(") == std::string::npos);
}

TEST_CASE("SimplnxCore::CreatePythonSkeletonFilter: Plugin writer rejects invalid names without preflight", "[SimplnxCore][CreatePythonSkeletonFilter][L15]")
{
  const ScopedOutputDirectory outputDir("CreatePythonSkeleton_WriterValidation");
  const bool existing = GENERATE(false, true);
  Result<> result;
  if(existing)
  {
    result = WritePythonFiltersToPlugin(outputDir.path(), "A, 1Bad");
    CHECK_FALSE(fs::exists(outputDir.path() / "A.py"));
  }
  else
  {
    result = WritePythonPluginFiles(outputDir.path(), "WriterPlugin", "WriterPlugin", "Description", "A, 1Bad");
    CHECK_FALSE(fs::exists(outputDir.path() / "WriterPlugin"));
  }
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -105150);
  CHECK(result.errors()[0].message.find("' 1Bad'") != std::string::npos);
}

TEST_CASE("SimplnxCore::CreatePythonSkeletonFilter: Embedded NUL is not a separator", "[SimplnxCore][CreatePythonSkeletonFilter][L15]")
{
  UnitTest::LoadPlugins();
  const bool existing = GENERATE(false, true);
  DataStructure dataStructure;
  CreatePythonSkeletonFilter filter;
  const ScopedOutputDirectory outputDir("CreatePythonSkeleton_EmbeddedNul");
  const std::string names("A\0B", 3);
  Arguments args;
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginOutputDirectory_Key, outputDir.path());
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, names);
  // CHECK, not REQUIRE, so that the writer checks below also run when preflight accepts the name.
  auto preflight = filter.preflight(dataStructure, args);
  CHECK(preflight.outputActions.invalid());
  if(preflight.outputActions.invalid())
  {
    REQUIRE(preflight.outputActions.errors().size() == 1);
    CHECK(preflight.outputActions.errors()[0].code == -105150);
  }
  const auto pluginDir = outputDir.path() / "NulPlugin";
  const auto sourceDir = pluginDir / "src" / "NulPlugin";
  Result<> result;
  if(existing)
  {
    // A valid plugin must exist first. Without it, a writer that splits on NUL would fail on the missing plugin files
    // instead of writing A.py and B.py.
    auto seedResult = WritePythonPluginFiles(outputDir.path(), "NulPlugin", "NulPlugin", "Description", "");
    SIMPLNX_RESULT_REQUIRE_VALID(seedResult);
    result = WritePythonFiltersToPlugin(sourceDir, names);
    CHECK_FALSE(fs::exists(sourceDir / "A.py"));
    CHECK_FALSE(fs::exists(sourceDir / "B.py"));
  }
  else
  {
    result = WritePythonPluginFiles(outputDir.path(), "NulPlugin", "NulPlugin", "Description", names);
    CHECK_FALSE(fs::exists(pluginDir));
  }
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -105150);
}

TEST_CASE("SimplnxCore::CreatePythonSkeletonFilter: Preserves plugin human name", "[SimplnxCore][CreatePythonSkeletonFilter][L14]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  CreatePythonSkeletonFilter filter;
  const ScopedOutputDirectory outputDir("CreatePythonSkeleton_HumanName");
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreatePythonSkeletonFilter::k_UseExistingPlugin_Key, false);
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginOutputDirectory_Key, outputDir.path());
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginName_Key, std::string("LabChecks"));
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginHumanName_Key, std::string("Laboratory Quality Checks"));
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, std::string("CheckFeatureTable,CheckPhaseMetadata"));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  const auto pluginDir = outputDir.path() / "LabChecks";
  const auto sourceDir = pluginDir / "src" / "LabChecks";
  const auto checkMetadata = [&sourceDir]() {
    std::ifstream input(sourceDir / "Plugin.py");
    REQUIRE(input.is_open());
    std::stringstream buffer;
    buffer << input.rdbuf();
    REQUIRE_FALSE(input.bad());
    auto text = buffer.str();
    // A Windows checkout can embed the template with CRLF line endings.
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    // Literal template context distinguishes the user-facing metadata from the machine identifier.
    CHECK(text.find("    return 'Laboratory Quality Checks'\n\n  def vendor") != std::string::npos);
    CHECK(text.find("    return 'LabChecks'\n\n  def vendor") == std::string::npos);
    CHECK(text.find("class LabChecks:") != std::string::npos);
    CHECK(text.find("    return 'LabChecks'\n\n  def description") != std::string::npos);
    CHECK(text.find("from .CheckFeatureTable import CheckFeatureTable\n") != std::string::npos);
    CHECK(text.find("from .CheckPhaseMetadata import CheckPhaseMetadata\n") != std::string::npos);
  };
  checkMetadata();
  for(const auto& relativePath : {"src/LabChecks/Plugin.py", "src/LabChecks/__init__.py", "src/LabChecks/CheckFeatureTable.py", "src/LabChecks/CheckPhaseMetadata.py", "docs/CheckFeatureTable.md",
                                  "docs/CheckPhaseMetadata.md", "environment.yml", "pyproject.toml", "conda/meta.yaml"})
  {
    CAPTURE(relativePath);
    CHECK(fs::is_regular_file(pluginDir / relativePath));
  }

  // Existing-plugin mode adds filters without replacing the plugin's configured metadata.
  args.insertOrAssign(CreatePythonSkeletonFilter::k_UseExistingPlugin_Key, true);
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginInputDirectory_Key, pluginDir);
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginHumanName_Key, std::string("Unused Existing Plugin Label"));
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, std::string("AdditionalCheck"));
  auto existingPreflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(existingPreflight.outputActions);
  auto existingResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(existingResult.result);
  checkMetadata();
  CHECK(fs::is_regular_file(sourceDir / "AdditionalCheck.py"));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreatePythonSkeletonFilter: Escapes plugin human name in Python literal", "[SimplnxCore][CreatePythonSkeletonFilter][L14]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  CreatePythonSkeletonFilter filter;
  const ScopedOutputDirectory outputDir("CreatePythonSkeleton_HumanNameEscape");
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreatePythonSkeletonFilter::k_UseExistingPlugin_Key, false);
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginOutputDirectory_Key, outputDir.path());
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginName_Key, std::string("LabChecks"));
  // Apostrophe, backslash, line break, tab and another control byte would end or break a single-quoted Python string.
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginHumanName_Key, std::string("O'Brien \\ Lab\nTools\tA\x01"));
  args.insertOrAssign(CreatePythonSkeletonFilter::k_PluginFilterNames, std::string("CheckFeatureTable"));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  std::ifstream input(outputDir.path() / "LabChecks" / "src" / "LabChecks" / "Plugin.py");
  REQUIRE(input.is_open());
  std::stringstream buffer;
  buffer << input.rdbuf();
  REQUIRE_FALSE(input.bad());
  auto text = buffer.str();
  text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
  // Python reads this literal back as the exact human name.
  CHECK(text.find("    return 'O\\'Brien \\\\ Lab\\nTools\\tA\\x01'\n\n  def vendor") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreatePythonSkeletonFilter: Preserves template tokens in human name", "[SimplnxCore][CreatePythonSkeletonFilter][L14]")
{
  const auto text = GeneratePythonPlugin("LabChecks", "Lab #PLUGIN_DESCRIPTION# #PLUGIN_FILTER_LIST# #PLUGIN_IMPORT_CODE#", "Description", {"CheckFeatureTable"});
  CHECK(text.find("    return 'Lab #PLUGIN_DESCRIPTION# #PLUGIN_FILTER_LIST# #PLUGIN_IMPORT_CODE#'") != std::string::npos);
  CHECK(text.find("from .CheckFeatureTable import CheckFeatureTable\n") != std::string::npos);
}
