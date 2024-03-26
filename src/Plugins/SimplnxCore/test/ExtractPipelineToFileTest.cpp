#include "SimplnxCore/Filters/ExtractPipelineToFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::ExtractPipelineToFileFilter", "[SimplnxCore][ExtractPipelineToFileFilter]")
{
  //// Instantiate the filter, a DataStructure object and an Arguments Object
  // DataStructure dataStructure;
  // Arguments args;
  // ExtractPipelineToFileFilter filter;
  //
  //// Create default Parameters for the filter.
  // args.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  // args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputDir, std::make_any<FileSystemPathParameter::ValueType>(outputDir));
  // args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFileName, std::make_any<std::string>(outputFileName));
  //
  //// Preflight the filter and check result
  // auto preflightResult = filter.preflight(dataStructure, args);
  // SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  //
  //// Execute the filter and check the result
  // auto executeResult = filter.execute(dataStructure, args);
  // SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}
