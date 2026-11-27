#include "SimplnxCore/Filters/ExtractPipelineToFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::constants;

namespace
{
fs::path KOutputFileName(fmt::format("{}/Small_IN100_Pipeline", unit_test::k_BinaryTestOutputDir));
fs::path KJsonOutputFile(KOutputFileName.string() + Pipeline::k_SIMPLExtension.str());
fs::path KNxOutputFile(KOutputFileName.string() + Pipeline::k_Extension.str());
} // namespace

TEST_CASE("SimplnxCore::ExtractPipelineToFileFilter: Valid Execution", "[SimplnxCore][ExtractPipelineToFileFilter]")
{
  unit_test::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ExtractPipelineToFileFilter filter;

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");
  auto exemplarDream3dFile = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, std::make_any<FileSystemPathParameter::ValueType>(exemplarDream3dFile));
  args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFile, std::make_any<FileSystemPathParameter::ValueType>(KNxOutputFile));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE(fs::exists(KNxOutputFile));

  fs::remove(KNxOutputFile);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractPipelineToFileFilter: Valid Execution - incorrect output extension", "[SimplnxCore][ExtractPipelineToFileFilter]")
{
  unit_test::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ExtractPipelineToFileFilter filter;

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");
  auto exemplarDream3dFile = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, std::make_any<FileSystemPathParameter::ValueType>(exemplarDream3dFile));
  args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFile, std::make_any<FileSystemPathParameter::ValueType>(KJsonOutputFile));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE(preflightResult.output_actions.warnings().front().code == -2581);
  REQUIRE(fs::exists(KNxOutputFile));

  fs::remove(KNxOutputFile);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractPipelineToFileFilter : Invalid Execution - missing output extension", "[SimplnxCore][ExtractPipelineToFileFilter]")
{
  unit_test::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ExtractPipelineToFileFilter filter;

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");
  auto exemplarDream3dFile = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, std::make_any<FileSystemPathParameter::ValueType>(exemplarDream3dFile));
  args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFile, std::make_any<FileSystemPathParameter::ValueType>(KOutputFileName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  REQUIRE(!fs::exists(KJsonOutputFile));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractPipelineToFileFilter : Invalid Execution - invalid input file format", "[SimplnxCore][ExtractPipelineToFileFilter]")
{
  unit_test::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ExtractPipelineToFileFilter filter;

  auto testInvalidInputFile = fs::path(fmt::format("{}/TestDataStructureNoPipeline.dream3d", unit_test::k_BinaryTestOutputDir));
  {
    DataStructure testDataStructure = unit_test::CreateDataStructure();
    nx::core::hdf5::FileIO fileWriter = nx::core::hdf5::FileIO::WriteFile(testInvalidInputFile);
    auto writeResult = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    REQUIRE(writeResult.valid());
  }

  fs::path outputFilePath(fmt::format("{}/TestDataStructureNoPipeline{}", unit_test::k_BinaryTestOutputDir, Pipeline::k_SIMPLExtension.str()));

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, std::make_any<FileSystemPathParameter::ValueType>(testInvalidInputFile));
  args.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFile, std::make_any<FileSystemPathParameter::ValueType>(outputFilePath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  REQUIRE(!fs::exists(KJsonOutputFile));

  fs::remove(testInvalidInputFile);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
