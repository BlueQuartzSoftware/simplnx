#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

#include "SimplnxCore/Filters/Algorithms/ReadVtkStructuredPoints.hpp"
#include "SimplnxCore/Filters/ReadVtkStructuredPointsFilter.hpp"

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
  // Configure the filter arguments.
  ReadVtkStructuredPointsFilter filter;
  DataStructure ds;
  Arguments args = createArgs(filePath);

  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == expectedErrCode);
}

namespace
{
template <typename T>
void WriteSectionValues(std::ostream& output, nonstd::span<const T> values, bool binary)
{
  for(const T value : values)
  {
    if(binary)
    {
      // Legacy VTK stores the most significant byte first, independently of host byte order.
      for(usize byte = sizeof(T); byte > 0; byte--)
      {
        output.put(static_cast<char>((static_cast<uint64>(value) >> (8 * (byte - 1))) & 0xff));
      }
    }
    else
    {
      output << value << ' ';
    }
  }
  output << '\n';
}
} // namespace

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Section Selection", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  UnitTest::LoadPlugins();

  const bool binary = GENERATE(false, true);
  const bool pointsFirst = GENERATE(false, true);
  // Exercise the regression (second only), plus first-only, both, and neither controls.
  const int32 selection = GENERATE(0, 1, 2, 3);
  const bool readPoints = selection == 2 || (selection == 0 && !pointsFirst) || (selection == 1 && pointsFirst);
  const bool readCells = selection == 2 || (selection == 0 && pointsFirst) || (selection == 1 && !pointsFirst);
  CAPTURE(binary, pointsFirst, selection);

  // Smallest nondegenerate 3D grid: 2*2*2 = 8 points, (2-1)^3 = 1 cell.
  // All expected values are literal input values; no reader or writer supplies the oracle.
  const std::array<int16, 8> pointScalars = {-32760, 513, 7, -9, 11, 13, 15, 17};
  const std::array<int16, 1> cellScalars = {-257};
  const std::array<int32, 24> pointVectors = {1, -2, 3, 4, 5, -6, 7, 8, 9, -10, 11, 12, 13, 14, 15, 16, -17, 18, 19, 20, 21, 22, 23, -24};
  const std::array<int32, 3> cellVectors = {65537, -2, 3};
  const fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / fmt::format("ReadVtkSectionSelection_{}_{}.vtk", binary, pointsFirst);
  {
    std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
    REQUIRE(output.is_open());
    output << "# vtk DataFile Version 3.0\nSection selection\n" << (binary ? "BINARY\n" : "ASCII\n");
    output << "DATASET STRUCTURED_POINTS\nDIMENSIONS 2 2 2\nSPACING 1 1 1\nORIGIN 0 0 0\n";
    for(const bool pointSection : {pointsFirst, !pointsFirst})
    {
      output << (pointSection ? "POINT_DATA 8\n" : "CELL_DATA 1\n");
      output << "SCALARS Scalars short 1\nLOOKUP_TABLE default\n";
      WriteSectionValues<int16>(output, pointSection ? nonstd::span<const int16>(pointScalars) : nonstd::span<const int16>(cellScalars), binary);
      output << "VECTORS Vectors int\n";
      WriteSectionValues<int32>(output, pointSection ? nonstd::span<const int32>(pointVectors) : nonstd::span<const int32>(cellVectors), binary);
    }
    output.close();
    REQUIRE(output.good());
  }

  ReadVtkStructuredPointsFilter filter;
  DataStructure ds;
  Arguments args = createArgs(filePath);
  args.insertOrAssign(ReadVtkStructuredPointsFilter::k_ReadPointData_Key, std::make_any<bool>(readPoints));
  args.insertOrAssign(ReadVtkStructuredPointsFilter::k_ReadCellData_Key, std::make_any<bool>(readCells));
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  for(const bool pointSection : {false, true})
  {
    const DataPath geomPath({pointSection ? "VTK Point Data" : "VTK Cell Data"});
    if(!(pointSection ? readPoints : readCells))
    {
      REQUIRE(ds.getData(geomPath) == nullptr);
      continue;
    }
    const auto* geom = ds.getDataAs<ImageGeom>(geomPath);
    REQUIRE(geom != nullptr);
    REQUIRE(geom->getDimensions() == (pointSection ? SizeVec3{2, 2, 2} : SizeVec3{1, 1, 1}));
    const DataPath matrixPath = geomPath.createChildPath(pointSection ? "Vertex Data" : "Cell Data");
    const auto* scalars = ds.getDataAs<Int16Array>(matrixPath.createChildPath("Scalars"));
    const auto* vectors = ds.getDataAs<Int32Array>(matrixPath.createChildPath("Vectors"));
    REQUIRE(scalars != nullptr);
    REQUIRE(vectors != nullptr);
    const auto expectedScalars = pointSection ? nonstd::span<const int16>(pointScalars) : nonstd::span<const int16>(cellScalars);
    const auto expectedVectors = pointSection ? nonstd::span<const int32>(pointVectors) : nonstd::span<const int32>(cellVectors);
    REQUIRE(scalars->getNumberOfTuples() == expectedScalars.size());
    REQUIRE(scalars->getNumberOfComponents() == 1);
    REQUIRE(vectors->getNumberOfTuples() == expectedScalars.size());
    REQUIRE(vectors->getNumberOfComponents() == 3);
    for(usize index = 0; index < expectedScalars.size(); index++)
    {
      REQUIRE((*scalars)[index] == expectedScalars[index]);
    }
    for(usize index = 0; index < expectedVectors.size(); index++)
    {
      REQUIRE((*vectors)[index] == expectedVectors[index]);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  // Configure the filter arguments.
  ReadVtkStructuredPointsFilter filter;
  DataStructure ds;
  Arguments args = createArgs(k_InputDirPath / "exemplary.vtk");

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

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

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Dimensions Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  test_invalid_case(k_InputDirPath / "dims_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DimsKeywordErr));
  test_invalid_case(k_InputDirPath / "dims_number_convert_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "dims_number_convert_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "dims_number_convert_err3.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "dims_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DimsWordCountErr));
  test_invalid_case(k_InputDirPath / "dims_word_count_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DimsWordCountErr));
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Origin Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

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
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

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
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  test_invalid_case(k_InputDirPath / "read_lookup_table_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadLookupTableKeywordErr));
  test_invalid_case(k_InputDirPath / "read_lookup_table_keyword_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadLookupTableKeywordErr));
  test_invalid_case(k_InputDirPath / "read_lookup_table_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadLookupTableWordCountErr));
  test_invalid_case(k_InputDirPath / "read_lookup_table_word_count_err2.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadLookupTableWordCountErr));
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Dataset Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

  test_invalid_case(k_InputDirPath / "dataset_data_type_number_conversion_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::NumberConvertErr));
  test_invalid_case(k_InputDirPath / "dataset_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetKeywordErr));
  test_invalid_case(k_InputDirPath / "dataset_structured_pts_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetStructuredPtsErr));
  test_invalid_case(k_InputDirPath / "dataset_type_keyword_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetTypeKeywordErr));
  test_invalid_case(k_InputDirPath / "dataset_type_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetTypeWordCountErr));
  test_invalid_case(k_InputDirPath / "dataset_word_count_err.vtk", to_underlying(ReadVtkStructuredPoints::ErrorCodes::DatasetWordCountErr));
}

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: Other Errors", "[SimplnxCore][ReadVtkStructuredPointsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_CompressedTestDirName, k_TestDirName);

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

TEST_CASE("SimplnxCore::ReadVtkStructuredPointsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReadVtkStructuredPointsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadVtkStructuredPointsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadVtkStructuredPointsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ReadVtkStructuredPointsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(ReadVtkStructuredPointsFilter::k_CellAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<FileSystemPathParameter::ValueType>(ReadVtkStructuredPointsFilter::k_InputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<bool>(ReadVtkStructuredPointsFilter::k_ReadCellData_Key) == true);
      CHECK(args.value<bool>(ReadVtkStructuredPointsFilter::k_ReadPointData_Key) == true);
      CHECK(args.value<std::string>(ReadVtkStructuredPointsFilter::k_VertexAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<DataPath>(ReadVtkStructuredPointsFilter::k_CreatedVertexGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ReadVtkStructuredPointsFilter::k_CreatedImageGeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
