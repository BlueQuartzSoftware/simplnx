#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/WriteVtkRectilinearGridFilter.hpp"

#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
constexpr usize k_BoolCount = 4097;
const DataPath k_BulkImagePath({"BulkImage"});
const DataPath k_BulkCellDataPath = k_BulkImagePath.createChildPath("CellData");
const DataPath k_BulkBoolPath = k_BulkCellDataPath.createChildPath("BoolValues");

class VtkFailOnLaterReadStore : public DataStore<bool>
{
public:
  explicit VtkFailOnLaterReadStore(int32 errorCode)
  : DataStore<bool>(ShapeType{k_BoolCount}, ShapeType{1}, false)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyIntoBuffer(usize offset, nonstd::span<bool> buffer) const override
  {
    if(++m_ReadCount == 2)
    {
      return MakeErrorResult(m_ErrorCode, "Injected VTK writer second-page read failure");
    }
    return DataStore<bool>::copyIntoBuffer(offset, buffer);
  }

private:
  int32 m_ErrorCode;
  mutable usize m_ReadCount = 0;
};

BoolArray& CreateBulkBoolInput(DataStructure& ds, std::shared_ptr<AbstractDataStore<bool>> store = nullptr)
{
  auto* image = ImageGeom::Create(ds, k_BulkImagePath.getTargetName());
  REQUIRE(image != nullptr);
  image->setDimensions({k_BoolCount, 1, 1});
  image->setSpacing({1.0F, 1.0F, 1.0F});
  image->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cellAM = AttributeMatrix::Create(ds, k_BulkCellDataPath.getTargetName(), ShapeType{1, 1, k_BoolCount}, image->getId());
  REQUIRE(cellAM != nullptr);
  image->setCellData(*cellAM);
  if(store == nullptr)
  {
    store = std::make_shared<DataStore<bool>>(cellAM->getShape(), ShapeType{1}, false);
  }
  auto* values = BoolArray::Create(ds, k_BulkBoolPath.getTargetName(), std::move(store), cellAM->getId());
  REQUIRE(values != nullptr);
  for(usize valueIdx = 0; valueIdx < k_BoolCount; valueIdx++)
  {
    (*values)[valueIdx] = valueIdx % 3 == 1;
  }
  return *values;
}

Arguments CreateWriterArguments(const fs::path& outputPath, bool binary)
{
  Arguments args;
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_WriteBinaryFile_Key, std::make_any<bool>(binary));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_BulkImagePath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_BulkBoolPath}));
  return args;
}

void AppendCoordinatesAscii(std::string& expected, const std::string& axis, usize count)
{
  expected += fmt::format("{} {} float\n", axis, count);
  for(usize idx = 0; idx < count; idx++)
  {
    expected += fmt::format("{:.6f} ", static_cast<float32>(idx) - 0.5F);
    if(idx % 20 == 0 && idx != 0)
    {
      expected += "\n";
    }
  }
  expected += "\n";
}

void AppendBigEndianFloat(std::string& expected, float32 value)
{
  const uint32 bits = std::bit_cast<uint32>(value);
  expected.push_back(static_cast<char>((bits >> 24U) & 0xFFU));
  expected.push_back(static_cast<char>((bits >> 16U) & 0xFFU));
  expected.push_back(static_cast<char>((bits >> 8U) & 0xFFU));
  expected.push_back(static_cast<char>(bits & 0xFFU));
}

void AppendCoordinatesBinary(std::string& expected, const std::string& axis, usize count)
{
  expected += fmt::format("{} {} float\n", axis, count);
  for(usize idx = 0; idx < count; idx++)
  {
    AppendBigEndianFloat(expected, static_cast<float32>(idx) - 0.5F);
  }
  expected += "\n";
}

std::string ExpectedBulkVtk(bool binary)
{
  std::string expected =
      fmt::format("# vtk DataFile Version 2.0\nData set from DREAM3D-NX SimplnxCore version 7.0.0\n{}\n\nDATASET RECTILINEAR_GRID\nDIMENSIONS 4098 2 2\n", binary ? "BINARY" : "ASCII");
  if(binary)
  {
    AppendCoordinatesBinary(expected, "X_COORDINATES", k_BoolCount + 1);
    AppendCoordinatesBinary(expected, "Y_COORDINATES", 2);
    AppendCoordinatesBinary(expected, "Z_COORDINATES", 2);
  }
  else
  {
    AppendCoordinatesAscii(expected, "X_COORDINATES", k_BoolCount + 1);
    AppendCoordinatesAscii(expected, "Y_COORDINATES", 2);
    AppendCoordinatesAscii(expected, "Z_COORDINATES", 2);
  }
  expected += "CELL_DATA 4097\nSCALARS BoolValues char 1\nLOOKUP_TABLE default\n";
  for(usize valueIdx = 0; valueIdx < k_BoolCount; valueIdx++)
  {
    const int value = valueIdx % 3 == 1 ? 1 : 0;
    if(binary)
    {
      expected.push_back(static_cast<char>(value));
    }
    else
    {
      if(valueIdx % 20 == 0 && valueIdx > 0)
      {
        expected += "\n";
      }
      expected += fmt::format(" {:d}", value);
    }
  }
  expected += "\n";
  return expected;
}
} // namespace

TEST_CASE("SimplnxCore::WriteVtkRectilinearGridFilter: Valid Filter Execution", "[SimplnxCore][WriteVtkRectilinearGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "vtk_rectilinear_grid_writer.tar.gz", "vtk_rectilinear_grid_writer");
  // Read input DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, and an Arguments Object
  WriteVtkRectilinearGridFilter filter;
  Arguments args;

  fs::path exemplarOutputPath = fs::path(fmt::format("{}/vtk_rectilinear_grid_writer/vtk_rectilinear_grid.vtk", unit_test::k_TestFilesDir));
  fs::path computedOutputPath(fmt::format("{}/vtk_rectilinear_grid_writer/vtk_rectilinear_grid.vtk", unit_test::k_BinaryTestOutputDir));
  fs::path exemplarBinaryOutputPath = fs::path(fmt::format("{}/vtk_rectilinear_grid_writer/vtk_rectilinear_grid_binary.vtk", unit_test::k_TestFilesDir));
  fs::path computedBinaryOutputPath(fmt::format("{}/vtk_rectilinear_grid_writer/vtk_rectilinear_grid_binary.vtk", unit_test::k_BinaryTestOutputDir));

  MultiArraySelectionParameter::ValueType selectedArrayPaths = {k_ConfidenceIndexArrayPath, k_EulersArrayPath, k_FitArrayPath, k_ImageQualityArrayPath, k_PhasesArrayPath, k_SEMSignalArrayPath};

  // write out ascii vtk file
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedOutputPath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_WriteBinaryFile_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedArrayPaths));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // write out binary vtk file
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedBinaryOutputPath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_WriteBinaryFile_Key, std::make_any<bool>(true));

  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  std::vector<size_t> linesToSkip{1}; // skip the version line
  std::ifstream computedFile(computedOutputPath);
  std::ifstream exemplarFile(exemplarOutputPath);
  UnitTest::CompareAsciiFiles(computedFile, exemplarFile, linesToSkip);
  std::ifstream computedBinaryFile(computedBinaryOutputPath, std::ios::binary);
  std::ifstream exemplarBinaryFile(exemplarBinaryOutputPath, std::ios::binary);
  UnitTest::CompareAsciiFiles(computedBinaryFile, exemplarBinaryFile, linesToSkip);

  // Remove the test files since they can get quite large.
  std::error_code errorCode;
  std::filesystem::remove_all(fmt::format("{}/vtk_rectilinear_grid_writer", unit_test::k_BinaryTestOutputDir), errorCode);

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::WriteVtkRectilinearGridFilter: InValid Filter Execution", "[SimplnxCore][WriteVtkRectilinearGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Read input DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, and an Arguments Object
  WriteVtkRectilinearGridFilter filter;
  Arguments args;

  fs::path computedOutputPath(fmt::format("{}/NX_vtk_rectilinear_grid.vtk", unit_test::k_BinaryTestOutputDir));

  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedOutputPath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_WriteBinaryFile_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));

  SECTION("No Selected Arrays")
  {
    args.insertOrAssign(WriteVtkRectilinearGridFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  }
  SECTION("Mismatching Tuples in Selected Arrays")
  {
    args.insertOrAssign(WriteVtkRectilinearGridFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{
                            k_CrystalStructuresArrayPath, k_ConfidenceIndexArrayPath, k_EulersArrayPath, k_FitArrayPath, k_ImageQualityArrayPath, k_PhasesArrayPath, k_SEMSignalArrayPath}));
  }
  SECTION("Selected Arrays are not Cell Level Arrays")
  {
    args.insertOrAssign(WriteVtkRectilinearGridFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_CrystalStructuresArrayPath}));
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::WriteVtkRectilinearGridFilter: Bulk bool page bytes and failure propagation", "[SimplnxCore][WriteVtkRectilinearGridFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("binary and ASCII output have independent exact bool payload oracles")
  {
    DataStructure ds;
    auto& source = CreateBulkBoolInput(ds);
    WriteVtkRectilinearGridFilter filter;

    for(const bool binary : {false, true})
    {
      const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / (binary ? "bulk_bool_binary.vtk" : "bulk_bool_ascii.vtk");
      const auto executeResult = filter.execute(ds, CreateWriterArguments(outputPath, binary));
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
      std::ifstream output(outputPath, std::ios::binary);
      const std::string actual(std::istreambuf_iterator<char>(output), {});
      REQUIRE(actual == ExpectedBulkVtk(binary));
      std::error_code error;
      fs::remove(outputPath, error);
    }

    for(usize valueIdx = 0; valueIdx < k_BoolCount; valueIdx++)
    {
      REQUIRE(source[valueIdx] == (valueIdx % 3 == 1));
    }
    UnitTest::CheckArraysInheritTupleDims(ds);
  }

  SECTION("second bool source page failure reaches the filter result")
  {
    constexpr int32 k_SourceError = -91931;
    DataStructure ds;
    auto failingStore = std::make_shared<VtkFailOnLaterReadStore>(k_SourceError);
    CreateBulkBoolInput(ds, failingStore);
    const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "bulk_bool_read_failure.vtk";

    WriteVtkRectilinearGridFilter filter;
    const auto executeResult = filter.execute(ds, CreateWriterArguments(outputPath, true));
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
    REQUIRE(std::any_of(executeResult.result.errors().cbegin(), executeResult.result.errors().cend(), [](const Error& error) { return error.code == -2090; }));
    REQUIRE(std::any_of(executeResult.result.errors().cbegin(), executeResult.result.errors().cend(), [](const Error& error) { return error.code == -2091; }));
    std::error_code error;
    fs::remove(outputPath, error);
  }
}

TEST_CASE("SimplnxCore::WriteVtkRectilinearGridFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteVtkRectilinearGridFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteVtkRectilinearGridFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteVtkRectilinearGridFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WriteVtkRectilinearGridFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteVtkRectilinearGridFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<bool>(WriteVtkRectilinearGridFilter::k_WriteBinaryFile_Key) == true);
      // Complex type (DataContainerFromMultiSelectionFilterParameterConverter) - verified by successful pipeline loading
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
