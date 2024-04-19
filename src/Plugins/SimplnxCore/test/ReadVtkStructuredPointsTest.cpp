#include <catch2/catch.hpp>

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "SimplnxCore/Filters/Algorithms/ReadVtkStructuredPoints.hpp"
#include "SimplnxCore/Filters/ReadVtkStructuredPointsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

const std::string k_TestDirName = "read_vtk_structured_points_test";
const std::string k_CompressedTestDirName = k_TestDirName + ".tar.gz";
const fs::path k_InputDirPath = fs::path(unit_test::k_TestFilesDir.view()) / k_TestDirName;

template <typename T>
void validateReadVtkStructuredPointsFilter(const DataStructure& ds, const DataPath& cellDataArrayPath, T cellDataValue, const DataPath& pointDataArrayPath, T pointDataValue)
{
  auto& cellDataArray = ds.getDataRefAs<DataArray<T>>(cellDataArrayPath);
  REQUIRE(std::all_of(cellDataArray.begin(), cellDataArray.end(), [&cellDataValue](const auto& value) { return value == cellDataValue; }));

  auto& pointDataArray = ds.getDataRefAs<DataArray<T>>(pointDataArrayPath);
  REQUIRE(std::all_of(pointDataArray.begin(), pointDataArray.end(), [&pointDataValue](const auto& value) { return value == pointDataValue; }));
}

Arguments createArgs(const fs::path& filePath)
{
  Arguments args;

  const std::string cellDataContainerName = "VTK Cell Data";
  const std::string cellAttrMatrixName = "Cell Data";
  const std::string pointDataContainerName = "VTK Point Data";
  const std::string pointAttrMatrixName = "Vertex Data";

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadVtkStructuredPointsFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(filePath));
  args.insertOrAssign(ReadVtkStructuredPointsFilter::k_ReadPointData_Key, std::make_any<BoolParameter::ValueType>(true));
  args.insertOrAssign(ReadVtkStructuredPointsFilter::k_ReadCellData_Key, std::make_any<BoolParameter::ValueType>(true));
  args.insertOrAssign(ReadVtkStructuredPointsFilter::k_CreatedVertexGeometryPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(DataPath{{pointDataContainerName}}));
  args.insertOrAssign(ReadVtkStructuredPointsFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(DataPath{{cellDataContainerName}}));
  args.insertOrAssign(ReadVtkStructuredPointsFilter::k_VertexAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(pointAttrMatrixName));
  args.insertOrAssign(ReadVtkStructuredPointsFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(cellAttrMatrixName));

  return args;
}

void test_invalid_case(const fs::path& filePath, int32 expectedErrCode)
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadVtkStructuredPointsFilter filter;
  DataStructure ds;
  Arguments args = createArgs(filePath);

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == expectedErrCode);
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadVtkStructuredPointsFilter filter;
  DataStructure ds;
  Arguments args = createArgs(k_InputDirPath / "exemplary.vtk");

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string cellDataContainerName = "VTK Cell Data";
  const std::string cellAttrMatrixName = "Cell Data";
  const std::string pointDataContainerName = "VTK Point Data";
  const std::string pointAttrMatrixName = "Vertex Data";
  DataPath cellAttrMatrixPath = DataPath({cellDataContainerName, cellAttrMatrixName});
  DataPath pointAttrMatrixPath = DataPath({pointDataContainerName, pointAttrMatrixName});

  validateReadVtkStructuredPointsFilter<uint8>(ds, cellAttrMatrixPath.createChildPath("cell_u8_data"), 0, pointAttrMatrixPath.createChildPath("point_u8_data"), 3);
  validateReadVtkStructuredPointsFilter<uint16>(ds, cellAttrMatrixPath.createChildPath("cell_u16_data"), 2, pointAttrMatrixPath.createChildPath("point_u16_data"), 0);
  validateReadVtkStructuredPointsFilter<uint32>(ds, cellAttrMatrixPath.createChildPath("cell_u32_data"), 4, pointAttrMatrixPath.createChildPath("point_u32_data"), 1);
  validateReadVtkStructuredPointsFilter<uint64>(ds, cellAttrMatrixPath.createChildPath("cell_u64_data"), 5, pointAttrMatrixPath.createChildPath("point_u64_data"), 2);
  validateReadVtkStructuredPointsFilter<int8>(ds, cellAttrMatrixPath.createChildPath("cell_i8_data"), 6, pointAttrMatrixPath.createChildPath("point_i8_data"), 4);
  validateReadVtkStructuredPointsFilter<int16>(ds, cellAttrMatrixPath.createChildPath("cell_i16_data"), 7, pointAttrMatrixPath.createChildPath("point_i16_data"), 5);
  validateReadVtkStructuredPointsFilter<int32>(ds, cellAttrMatrixPath.createChildPath("cell_i32_data"), 8, pointAttrMatrixPath.createChildPath("point_i32_data"), 6);
  validateReadVtkStructuredPointsFilter<int64>(ds, cellAttrMatrixPath.createChildPath("cell_i64_data"), 9, pointAttrMatrixPath.createChildPath("point_i64_data"), 7);
  validateReadVtkStructuredPointsFilter<float32>(ds, cellAttrMatrixPath.createChildPath("cell_float_data"), 3, pointAttrMatrixPath.createChildPath("point_float_data"), 8);
  validateReadVtkStructuredPointsFilter<float64>(ds, cellAttrMatrixPath.createChildPath("cell_double_data"), 1, pointAttrMatrixPath.createChildPath("point_double_data"), 9);
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Dimensions Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  test_invalid_case(k_InputDirPath / "dims_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DimsKeywordErr));
  test_invalid_case(k_InputDirPath / "dims_number_convert_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "dims_number_convert_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "dims_number_convert_err3.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "dims_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DimsWordCountErr));
  test_invalid_case(k_InputDirPath / "dims_word_count_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DimsWordCountErr));
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Origin Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  test_invalid_case(k_InputDirPath / "origin_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::OriginKeywordErr));
  test_invalid_case(k_InputDirPath / "origin_number_convert_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "origin_number_convert_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "origin_number_convert_err3.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "origin_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::OriginWordCountErr));
  test_invalid_case(k_InputDirPath / "origin_word_count_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::OriginWordCountErr));
  test_invalid_case(k_InputDirPath / "origin_word_count_err3.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::OriginWordCountErr));
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Spacing Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  test_invalid_case(k_InputDirPath / "spacing_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::SpacingKeywordErr));
  test_invalid_case(k_InputDirPath / "spacing_number_convert_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "spacing_number_convert_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "spacing_number_convert_err3.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "spacing_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::SpacingWordCountErr));
  test_invalid_case(k_InputDirPath / "spacing_word_count_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::SpacingWordCountErr));
  test_invalid_case(k_InputDirPath / "spacing_word_count_err3.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::SpacingWordCountErr));
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Lookup Table Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  test_invalid_case(k_InputDirPath / "read_lookup_table_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadLookupTableKeywordErr));
  test_invalid_case(k_InputDirPath / "read_lookup_table_keyword_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadLookupTableKeywordErr));
  test_invalid_case(k_InputDirPath / "read_lookup_table_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadLookupTableWordCountErr));
  test_invalid_case(k_InputDirPath / "read_lookup_table_word_count_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadLookupTableWordCountErr));
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Dataset Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  test_invalid_case(k_InputDirPath / "dataset_data_type_number_conversion_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "dataset_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetKeywordErr));
  test_invalid_case(k_InputDirPath / "dataset_structured_pts_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetStructuredPtsErr));
  test_invalid_case(k_InputDirPath / "dataset_type_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetTypeKeywordErr));
  test_invalid_case(k_InputDirPath / "dataset_type_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetTypeWordCountErr));
  test_invalid_case(k_InputDirPath / "dataset_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetWordCountErr));
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Other Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  test_invalid_case(k_InputDirPath / "convert_vtk_data_type_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ConvertVtkDataTypeErr));
  test_invalid_case(k_InputDirPath / "file_type_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::FileTypeErr));
  test_invalid_case(k_InputDirPath / "mismatched_cells_and_tuples_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::MismatchedCellsAndTuplesErr));
  test_invalid_case(k_InputDirPath / "mismatched_points_and_tuples_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::MismatchedPointsAndTuplesErr));
  test_invalid_case(k_InputDirPath / "read_data_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "read_data_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "read_scalar_header_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadScalarHeaderWordCountErr));
  test_invalid_case(k_InputDirPath / "read_scalar_header_word_count_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadScalarHeaderWordCountErr));
  test_invalid_case(k_InputDirPath / "unknown_section_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::UnknownSectionKeywordErr));
  test_invalid_case(k_InputDirPath / "unknown_section_keyword_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::UnknownSectionKeywordErr));
  test_invalid_case(k_InputDirPath / "unknown_section_keyword_err3.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::UnknownSectionKeywordErr));
  test_invalid_case(k_InputDirPath / "unknown_section_keyword_err4.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::UnknownSectionKeywordErr));
}
