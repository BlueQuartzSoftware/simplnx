#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/CreatePythonSkeletonFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
//
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

TEST_CASE("SimplnxCore::CreatePythonSkeleton")
{

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
      REQUIRE(preflightResult.outputActions.valid());

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      REQUIRE(executeResult.result.valid());
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
      REQUIRE(preflightResult.outputActions.valid());

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      REQUIRE(executeResult.result.valid());
    }
  }
}
