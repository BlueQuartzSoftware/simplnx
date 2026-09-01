#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/WriteVtkStructuredPointsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
constexpr usize k_BenchmarkDim = 200;
constexpr usize k_BenchmarkSliceTuples = k_BenchmarkDim * k_BenchmarkDim;
constexpr usize k_BenchmarkTotalTuples = k_BenchmarkDim * k_BenchmarkDim * k_BenchmarkDim;
constexpr usize k_BenchmarkPayloadBytes = k_BenchmarkTotalTuples * sizeof(uint16);
const std::string k_BenchmarkGeomName = "Benchmark ImageGeom";
const std::string k_BenchmarkCellDataName = "Cell Data";
const std::string k_BenchmarkScalarsName = "Benchmark Scalars";
const DataPath k_BenchmarkGeomPath({k_BenchmarkGeomName});
const DataPath k_BenchmarkCellDataPath = k_BenchmarkGeomPath.createChildPath(k_BenchmarkCellDataName);
const DataPath k_BenchmarkScalarsPath = k_BenchmarkCellDataPath.createChildPath(k_BenchmarkScalarsName);

uint16 ExpectedValue(usize tupleIndex)
{
  return static_cast<uint16>(((tupleIndex * 17) + 23) % 65521);
}
} // namespace

TEST_CASE("SimplnxCore::WriteVtkStructuredPointsFilter: Binary correctness", "[SimplnxCore][WriteVtkStructuredPointsFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize k_DimX = 2;
  constexpr usize k_DimY = 2;
  constexpr usize k_DimZ = 1;
  constexpr std::array<uint16, 4> k_ExpectedValues = {0x0123, 0x4567, 0x89AB, 0xCDEF};
  const std::string geomName = "Small ImageGeom";
  const std::string cellDataName = "Cell Data";
  const std::string scalarsName = "Small Scalars";
  const DataPath geomPath({geomName});
  const DataPath cellDataPath = geomPath.createChildPath(cellDataName);
  const DataPath scalarsPath = cellDataPath.createChildPath(scalarsName);

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, geomName);
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({k_DimX, k_DimY, k_DimZ});
  imageGeom->setOrigin({-1.0F, 2.0F, 3.0F});
  imageGeom->setSpacing({0.5F, 1.5F, 2.0F});

  const ShapeType tupleShape = {k_DimZ, k_DimY, k_DimX};
  auto* cellData = AttributeMatrix::Create(dataStructure, cellDataName, tupleShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);

  auto scalarsStore = DataStoreUtilities::CreateDataStore<uint16>(dataStructure, scalarsPath, tupleShape, {1}, IDataAction::Mode::Execute);
  auto* scalars = UInt16Array::Create(dataStructure, scalarsName, scalarsStore, cellData->getId());
  REQUIRE(scalars != nullptr);
  SIMPLNX_RESULT_REQUIRE_VALID(scalarsStore->copyFromBuffer(0, nonstd::span<const uint16>(k_ExpectedValues.data(), k_ExpectedValues.size())));
  scope.requireExpectedStore(*scalars);

  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteVtkStructuredPointsCorrectness.vtk";
  auto outputFileGuard = MakeScopeGuard([&outputPath]() noexcept {
    std::error_code errorCode;
    fs::remove(outputPath, errorCode);
  });

  WriteVtkStructuredPointsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_WriteBinaryFile_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{scalarsPath}));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  std::ifstream vtkFile(outputPath, std::ios::binary);
  REQUIRE(vtkFile.is_open());
  constexpr std::array<const char*, 10> k_ExpectedHeaderLines = {"# vtk DataFile Version 3.0",
                                                                 "vtk output",
                                                                 "BINARY",
                                                                 "DATASET STRUCTURED_POINTS",
                                                                 "DIMENSIONS 3 3 2",
                                                                 "SPACING 0.5 1.5 2",
                                                                 "ORIGIN -1 2 3",
                                                                 "CELL_DATA 4",
                                                                 "SCALARS Small_Scalars unsigned_short 1",
                                                                 "LOOKUP_TABLE default"};
  std::string line;
  for(const char* expectedLine : k_ExpectedHeaderLines)
  {
    REQUIRE(std::getline(vtkFile, line));
    REQUIRE(line == expectedLine);
  }

  for(const uint16 expected : k_ExpectedValues)
  {
    std::array<uint8, sizeof(uint16)> bytes = {};
    vtkFile.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    REQUIRE(vtkFile.gcount() == static_cast<std::streamsize>(bytes.size()));
    const uint16 actual = static_cast<uint16>((static_cast<uint16>(bytes[0]) << 8U) | bytes[1]);
    CHECK(actual == expected);
  }

  char terminator = '\0';
  REQUIRE(vtkFile.get(terminator));
  REQUIRE(terminator == '\n');
  REQUIRE(vtkFile.peek() == std::ifstream::traits_type::eof());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
