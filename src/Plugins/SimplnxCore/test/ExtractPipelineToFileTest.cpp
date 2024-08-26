#include "SimplnxCore/Filters/ExtractPipelineToFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
fs::path k_OutputFileName(fmt::format("{}/Small_IN100_Pipeline", unit_test::k_BinaryTestOutputDir));
fs::path k_JsonOutputFile(k_OutputFileName.string() + Pipeline::k_SIMPLExtension.str());
fs::path k_NXOutputFile(k_OutputFileName.string() + Pipeline::k_Extension.str());
} // namespace

TEST_CASE("SimplnxCore::ExtractPipelineToFileFilter: Valid Execution", "[SimplnxCore][ExtractPipelineToFileFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ExtractPipelineToFileFilter filter;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v2.tar.gz", "Small_IN100.dream3d");
  auto exemplarDream3dFile = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, std::make_any<FileSystemPathParameter::ValueType>(exemplarDream3dFile));
  args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFile, std::make_any<FileSystemPathParameter::ValueType>(k_NXOutputFile));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE(fs::exists(k_NXOutputFile));

  fs::remove(k_NXOutputFile);
}

TEST_CASE("SimplnxCore::ExtractPipelineToFileFilter: Valid Execution - incorrect output extension", "[SimplnxCore][ExtractPipelineToFileFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ExtractPipelineToFileFilter filter;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v2.tar.gz", "Small_IN100.dream3d");
  auto exemplarDream3dFile = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, std::make_any<FileSystemPathParameter::ValueType>(exemplarDream3dFile));
  args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFile, std::make_any<FileSystemPathParameter::ValueType>(k_JsonOutputFile));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE(preflightResult.outputActions.warnings().front().code == -2581);
  REQUIRE(fs::exists(k_NXOutputFile));

  fs::remove(k_NXOutputFile);
}

TEST_CASE("SimplnxCore::ExtractPipelineToFileFilter : Invalid Execution - missing output extension", "[SimplnxCore][ExtractPipelineToFileFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ExtractPipelineToFileFilter filter;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v2.tar.gz", "Small_IN100.dream3d");
  auto exemplarDream3dFile = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, std::make_any<FileSystemPathParameter::ValueType>(exemplarDream3dFile));
  args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFile, std::make_any<FileSystemPathParameter::ValueType>(k_OutputFileName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  REQUIRE(!fs::exists(k_JsonOutputFile));
}

TEST_CASE("SimplnxCore::ExtractPipelineToFileFilter : Invalid Execution - invalid input file format", "[SimplnxCore][ExtractPipelineToFileFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ExtractPipelineToFileFilter filter;

  auto testInvalidInputFile = fs::path(fmt::format("{}/TestDataStructureNoPipeline.dream3d", unit_test::k_BinaryTestOutputDir));
  {
    DataStructure testDataStructure = UnitTest::CreateDataStructure();
    auto fileWriterResult = nx::core::HDF5::FileWriter::CreateFile(testInvalidInputFile);
    REQUIRE(fileWriterResult.valid());
    nx::core::HDF5::FileWriter fileWriter = std::move(fileWriterResult.value());
    auto writeResult = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    REQUIRE(writeResult.valid());
  }

  fs::path outputFilePath(fmt::format("{}/TestDataStructureNoPipeline{}", unit_test::k_BinaryTestOutputDir, Pipeline::k_SIMPLExtension.str()));

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, std::make_any<FileSystemPathParameter::ValueType>(testInvalidInputFile));
  args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFile, std::make_any<FileSystemPathParameter::ValueType>(outputFilePath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  REQUIRE(!fs::exists(k_JsonOutputFile));

  fs::remove(testInvalidInputFile);
}
