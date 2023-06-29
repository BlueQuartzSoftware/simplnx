#include "OrientationAnalysis/Filters/ReadCtfDataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

TEST_CASE("OrientationAnalysis::ReadCtfData: Valid Execution", "[OrientationAnalysis][ReadCtfData][.][UNIMPLEMENTED][!mayfail]")
{
  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_read_ctf_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadCtfDataFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const fs::path inputCtfFile(fmt::format("{}/Data/Textured_Copper/Cugrid_after 2nd_15kv_2kx_2.ctf", unit_test::k_DREAM3DDataDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadCtfDataFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(inputCtfFile));
  args.insertOrAssign(ReadCtfDataFilter::k_DegreesToRadians_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadCtfDataFilter::k_EdaxHexagonalAlignment_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadCtfDataFilter::k_DataContainerName_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ReadCtfDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));
  args.insertOrAssign(ReadCtfDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_EnsembleAttributeMatrix));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_CellAttributeMatrix, k_ExemplarDataContainer);
}
