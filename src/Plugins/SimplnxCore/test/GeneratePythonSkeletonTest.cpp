#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/GeneratePythonSkeletonFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ColorPresetsUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

TEST_CASE("SimplnxCore::GeneratePythonSkeleton")
{

  // DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/generate_vector_colors/6_6_generate_vector_colors.dream3d", unit_test::k_TestFilesDir)));
  DataStructure dataStructure;
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    GeneratePythonSkeletonFilter filter;
    Arguments args;

    const std::string k_PluginName = "ExampleToolbox";
    const std::string k_PluginHumanName = "Example Toolbox";
    const std::filesystem::path k_OutputDirectory = {unit_test::k_BinaryTestOutputDir};
    const std::string k_FilterNames = "SomeFilter,OtherFilter,ThirdFilter";

    // Create default Parameters for the filter.
    args.insertOrAssign(GeneratePythonSkeletonFilter::k_PluginName_Key, std::make_any<std::string>(k_PluginName));
    args.insertOrAssign(GeneratePythonSkeletonFilter::k_PluginHumanName_Key, std::make_any<std::string>(k_PluginHumanName));
    args.insertOrAssign(GeneratePythonSkeletonFilter::k_PluginOutputDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>("/Users/mjackson/Workspace1/simplnx/wrapping/python/plugins"));
    args.insertOrAssign(GeneratePythonSkeletonFilter::k_PluginFilterNames, std::make_any<std::string>(k_FilterNames));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }
}
