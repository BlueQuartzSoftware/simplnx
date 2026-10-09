#pragma once

#include "SimplnxCore/Filters/WriteVtkStructuredPointsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <any>
#include <array>
#include <filesystem>
#include <fstream>
#include <locale>
#include <string>
#include <system_error>

/**
 * @namespace WriteVtkStructuredPointsTest
 * @brief Checks the same ASCII file contract for resident and disk-backed inputs.
 */
namespace WriteVtkStructuredPointsTest
{
/**
 * @brief Writes two arrays and checks every output field and unchanged input value.
 * @tparam ScopeT Provides the selected algorithm and storage checks.
 * @param scope Owns the storage policy before this function creates arrays.
 * @param expectedFormat Identifies the required backing format for both input arrays.
 */
template <typename ScopeT>
void CheckAsciiOutput(ScopeT& scope, const std::string& expectedFormat)
{
  using namespace nx::core;
  namespace fs = std::filesystem;

  constexpr std::array<int8, 8> k_ExpectedSignedValues = {-128, 127, -7, 9, 0, 42, 100, -100};
  constexpr std::array<float32, 4> k_ExpectedFloatValues = {0.25F, -1.5F, 2.0F, 100.0F};
  const DataPath geomPath({"ASCII ImageGeom"});
  const DataPath cellDataPath = geomPath.createChildPath("Cell Data");
  const DataPath signedPath = cellDataPath.createChildPath("Signed Components");
  const DataPath floatPath = cellDataPath.createChildPath("Float Scalars");
  const ShapeType tupleShape = {1, 2, 2};

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ASCII ImageGeom");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({2, 2, 1});
  imageGeom->setOrigin({-1.0F, 2.0F, 3.0F});
  imageGeom->setSpacing({0.5F, 1.5F, 2.0F});
  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", tupleShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);

  auto signedStore = DataStoreUtilities::CreateDataStore<int8>(dataStructure, signedPath, tupleShape, {2});
  REQUIRE(signedStore != nullptr);
  auto* signedArray = Int8Array::Create(dataStructure, "Signed Components", signedStore, cellData->getId());
  REQUIRE(signedArray != nullptr);
  const auto signedWrite = signedStore->copyFromBuffer(0, nonstd::span<const int8>(k_ExpectedSignedValues.data(), k_ExpectedSignedValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(signedWrite);

  auto floatStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, floatPath, tupleShape, {1});
  REQUIRE(floatStore != nullptr);
  auto* floatArray = Float32Array::Create(dataStructure, "Float Scalars", floatStore, cellData->getId());
  REQUIRE(floatArray != nullptr);
  const auto floatWrite = floatStore->copyFromBuffer(0, nonstd::span<const float32>(k_ExpectedFloatValues.data(), k_ExpectedFloatValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(floatWrite);

  scope.requireExpectedStore(*signedArray);
  scope.requireExpectedStore(*floatArray);
  REQUIRE(signedArray->getDataFormat() == expectedFormat);
  REQUIRE(floatArray->getDataFormat() == expectedFormat);

  const fs::path outputBase = fs::path(unit_test::k_BinaryTestOutputDir.view());
  fs::create_directories(outputBase);
  const fs::path outputRoot = outputBase / ("WriteVtkAscii-" + Uuid::GenerateV4().str());
  REQUIRE(fs::create_directory(outputRoot));
  auto outputGuard = MakeScopeGuard([&outputRoot]() noexcept {
    std::error_code errorCode;
    fs::remove_all(outputRoot, errorCode);
  });
  const fs::path outputPath = outputRoot / "output.vtk";

  WriteVtkStructuredPointsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_WriteBinaryFile_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WriteVtkStructuredPointsFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{signedPath, floatPath}));
  const auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  {
    std::ifstream vtkFile(outputPath, std::ios::binary);
    REQUIRE(vtkFile.is_open());
    vtkFile.imbue(std::locale::classic());
    constexpr std::array<const char*, 10> k_ExpectedHeaderLines = {"# vtk DataFile Version 3.0", "vtk output",    "ASCII",       "DATASET STRUCTURED_POINTS",        "DIMENSIONS 3 3 2",
                                                                   "SPACING 0.5 1.5 2",          "ORIGIN -1 2 3", "CELL_DATA 4", "SCALARS Signed_Components char 2", "LOOKUP_TABLE default"};
    std::string line;
    for(const char* expectedLine : k_ExpectedHeaderLines)
    {
      REQUIRE(std::getline(vtkFile, line));
      REQUIRE(line == expectedLine);
    }

    // Integer extraction must reject character output for signed 8-bit values.
    for(const int8 expectedValue : k_ExpectedSignedValues)
    {
      int actualValue = 0;
      REQUIRE(vtkFile >> actualValue);
      REQUIRE(actualValue == static_cast<int>(expectedValue));
    }
    vtkFile >> std::ws;
    REQUIRE(std::getline(vtkFile, line));
    REQUIRE(line == "SCALARS Float_Scalars float 1");
    REQUIRE(std::getline(vtkFile, line));
    REQUIRE(line == "LOOKUP_TABLE default");
    for(const float32 expectedValue : k_ExpectedFloatValues)
    {
      double actualValue = 0.0;
      REQUIRE(vtkFile >> actualValue);
      REQUIRE(actualValue == static_cast<double>(expectedValue));
    }
    vtkFile >> std::ws;
    REQUIRE(vtkFile.peek() == std::ifstream::traits_type::eof());
    REQUIRE_FALSE(vtkFile.bad());
  }

  scope.requireExpectedStore(*signedArray);
  scope.requireExpectedStore(*floatArray);
  REQUIRE(signedArray->getDataFormat() == expectedFormat);
  REQUIRE(floatArray->getDataFormat() == expectedFormat);
  REQUIRE(signedArray->getDataType() == DataType::int8);
  REQUIRE(floatArray->getDataType() == DataType::float32);
  REQUIRE(signedArray->getTupleShape() == tupleShape);
  REQUIRE(floatArray->getTupleShape() == tupleShape);
  REQUIRE(signedArray->getComponentShape() == ShapeType{2});
  REQUIRE(floatArray->getComponentShape() == ShapeType{1});
  std::array<int8, 8> actualSignedValues = {};
  const auto signedRead = signedArray->getDataStoreRef().copyIntoBuffer(0, nonstd::span<int8>(actualSignedValues.data(), actualSignedValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(signedRead);
  REQUIRE(actualSignedValues == k_ExpectedSignedValues);
  std::array<float32, 4> actualFloatValues = {};
  const auto floatRead = floatArray->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(actualFloatValues.data(), actualFloatValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(floatRead);
  REQUIRE(actualFloatValues == k_ExpectedFloatValues);

  const SizeVec3 expectedDims = {2, 2, 1};
  const FloatVec3 expectedOrigin = {-1.0F, 2.0F, 3.0F};
  const FloatVec3 expectedSpacing = {0.5F, 1.5F, 2.0F};
  REQUIRE(imageGeom->getDimensions() == expectedDims);
  REQUIRE(imageGeom->getOrigin() == expectedOrigin);
  REQUIRE(imageGeom->getSpacing() == expectedSpacing);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace WriteVtkStructuredPointsTest
