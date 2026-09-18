#include <catch2/catch.hpp>

#include <fmt/format.h>

#include "SimplnxCore/Filters/ErodeDilateMaskFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr ChoicesParameter::ValueType k_Dilate = 0ULL;
constexpr ChoicesParameter::ValueType k_Erode = 1ULL;

const std::string k_GeomName("ImageGeom");
const std::string k_CellDataName("CellData");

const DataPath k_GeomPath({k_GeomName});
const DataPath k_CellDataPath = k_GeomPath.createChildPath(k_CellDataName);
const DataPath k_MaskPath = k_CellDataPath.createChildPath("Mask");

const std::string k_EbsdScanDataName("EBSD Scan Data");
const DataPath k_InputDataPath({"Input Data"});
const DataPath k_EbsdScanDataPath = k_InputDataPath.createChildPath(k_EbsdScanDataName);
const DataPath k_ExemplarMaskPath = k_EbsdScanDataPath.createChildPath("Mask");

struct MaskLiteralFixture
{
  DataStructure dataStructure;
  DataPath geometryPath;
  DataPath maskPath;
  DataPath siblingValuesPath;
};

/**
 * @brief Creates a literal mask fixture with an unchanged sibling array.
 * @param dimX Specifies the image X dimension.
 * @param dimY Specifies the image Y dimension.
 * @param dimZ Specifies the image Z dimension.
 * @param maskValues Specifies the four input mask tuples.
 * @return In-memory fixture that contains the input values.
 */
MaskLiteralFixture CreateMaskLiteralFixture(const usize dimX, const usize dimY, const usize dimZ, const std::array<bool, 4>& maskValues)
{
  MaskLiteralFixture fixture;
  fixture.geometryPath = DataPath({"Mask Literal Geometry"});
  const DataPath cellDataPath = fixture.geometryPath.createChildPath("Cell Data");
  fixture.maskPath = cellDataPath.createChildPath("Mask");
  fixture.siblingValuesPath = cellDataPath.createChildPath("Sibling Values");
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};

  auto* imageGeomPtr = ImageGeom::Create(fixture.dataStructure, fixture.geometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cellDataPtr = AttributeMatrix::Create(fixture.dataStructure, cellDataPath.getTargetName(), cellTupleShape, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);

  auto maskStore = DataStoreUtilities::CreateDataStore<bool>(fixture.dataStructure, fixture.maskPath, cellTupleShape, {1});
  auto* maskArrayPtr = BoolArray::Create(fixture.dataStructure, fixture.maskPath.getTargetName(), maskStore, cellDataPtr->getId());
  REQUIRE(maskArrayPtr != nullptr);
  maskStore->fill(false);
  for(usize tupleIdx = 0; tupleIdx < maskValues.size(); tupleIdx++)
  {
    (*maskStore)[tupleIdx] = maskValues[tupleIdx];
  }

  auto siblingValuesStore = DataStoreUtilities::CreateDataStore<int32>(fixture.dataStructure, fixture.siblingValuesPath, cellTupleShape, {1});
  auto* siblingValuesArrayPtr = Int32Array::Create(fixture.dataStructure, fixture.siblingValuesPath.getTargetName(), siblingValuesStore, cellDataPtr->getId());
  REQUIRE(siblingValuesArrayPtr != nullptr);
  constexpr std::array<int32, 4> k_SiblingValues = {10, 20, 30, 40};
  for(usize tupleIdx = 0; tupleIdx < k_SiblingValues.size(); tupleIdx++)
  {
    (*siblingValuesStore)[tupleIdx] = k_SiblingValues[tupleIdx];
  }

  return fixture;
}

/**
 * @brief Executes mask morphology with the selected literal fixture.
 * @param fixture Contains the input mask and sibling array.
 * @param operation Selects dilation or erosion.
 * @param iterations Specifies the number of synchronous passes.
 * @param xDirOn Enables X neighbors.
 * @param yDirOn Enables Y neighbors.
 * @param zDirOn Enables Z neighbors.
 */
void ExecuteMaskLiteralFixture(MaskLiteralFixture& fixture, const ChoicesParameter::ValueType operation, const int32 iterations, const bool xDirOn, const bool yDirOn, const bool zDirOn)
{
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath));
  const auto& inputMaskArrayRef = fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath);
  REQUIRE(inputMaskArrayRef.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);

  ErodeDilateMaskFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(iterations));
  args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(xDirOn));
  args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(yDirOn));
  args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(zDirOn));
  args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(fixture.maskPath));
  args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(fixture.geometryPath));

  const auto preflightResult = filter.preflight(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}

/**
 * @brief Checks all values in a four-tuple mask array.
 * @param arrayRef Contains the actual mask values.
 * @param expectedValues Contains the literal expected mask values.
 */
void CheckMaskValues(const BoolArray& arrayRef, const std::array<bool, 4>& expectedValues)
{
  REQUIRE(arrayRef.getNumberOfTuples() == expectedValues.size());
  std::array<bool, 4> actualValues = {};
  for(usize tupleIdx = 0; tupleIdx < expectedValues.size(); tupleIdx++)
  {
    actualValues[tupleIdx] = arrayRef[tupleIdx];
  }
  INFO(fmt::format("{} actual=[{}, {}, {}, {}] expected=[{}, {}, {}, {}]", arrayRef.getName(), actualValues[0], actualValues[1], actualValues[2], actualValues[3], expectedValues[0], expectedValues[1],
                   expectedValues[2], expectedValues[3]));
  for(usize tupleIdx = 0; tupleIdx < expectedValues.size(); tupleIdx++)
  {
    CHECK(actualValues[tupleIdx] == expectedValues[tupleIdx]);
  }
}

/**
 * @brief Checks that morphology does not modify the unrelated sibling array.
 * @param arrayRef Contains the sibling values.
 */
void CheckSiblingValues(const Int32Array& arrayRef)
{
  constexpr std::array<int32, 4> k_ExpectedValues = {10, 20, 30, 40};
  REQUIRE(arrayRef.getNumberOfTuples() == k_ExpectedValues.size());
  std::array<int32, 4> actualValues = {};
  for(usize tupleIdx = 0; tupleIdx < k_ExpectedValues.size(); tupleIdx++)
  {
    actualValues[tupleIdx] = arrayRef[tupleIdx];
  }
  INFO(fmt::format("{} actual=[{}, {}, {}, {}] expected=[{}, {}, {}, {}]", arrayRef.getName(), actualValues[0], actualValues[1], actualValues[2], actualValues[3], k_ExpectedValues[0],
                   k_ExpectedValues[1], k_ExpectedValues[2], k_ExpectedValues[3]));
  for(usize tupleIdx = 0; tupleIdx < k_ExpectedValues.size(); tupleIdx++)
  {
    CHECK(actualValues[tupleIdx] == k_ExpectedValues[tupleIdx]);
  }
}

void BuildTestData(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};
  const usize sliceSize = dimX * dimY;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto maskDataStore = DataStoreUtilities::CreateDataStore<bool>(dataStructure, k_MaskPath, cellTupleShape, {1});
  auto* maskArray = DataArray<bool>::Create(dataStructure, "Mask", maskDataStore, cellAM->getId());
  auto& maskStore = maskArray->getDataStoreRef();

  // Use Z-slice buffered writes. Bool data requires a raw array instead of vector<bool>.
  auto maskBuf = std::make_unique<bool[]>(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize inSlice = y * dimX + x;
        maskBuf[inSlice] = ((x * 7 + y * 13 + z * 29) % 3 != 0);
      }
    }
    auto maskStoreWriteResult = maskStore.copyFromBuffer(z * sliceSize, nonstd::span<const bool>(maskBuf.get(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(maskStoreWriteResult);
  }
}

usize CountTrueVoxels(const DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const auto& mask = dataStructure.getDataRefAs<BoolArray>(k_MaskPath).getDataStoreRef();
  const usize sliceSize = dimX * dimY;
  auto buf = std::make_unique<bool[]>(sliceSize);
  usize count = 0;
  for(usize z = 0; z < dimZ; z++)
  {
    auto maskReadResult = mask.copyIntoBuffer(z * sliceSize, nonstd::span<bool>(buf.get(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(maskReadResult);
    for(usize i = 0; i < sliceSize; i++)
    {
      if(buf[i])
      {
        count++;
      }
    }
  }
  return count;
}
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Generate Test Data", "[SimplnxCore][ErodeDilateMaskFilter][.GenerateTestData]")
{
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "erode_dilate_mask";
  fs::create_directories(outputDir);

  // The small fixture uses a 20-cubed volume.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 20, 20, 20);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "small_input.dream3d");
  }

  // The large fixture uses a 200-cubed volume.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 200, 200, 200);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "large_input.dream3d");
  }
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter(Dilate)", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_erode_dilate_test.tar.gz", "6_6_erode_dilate_test");
  const fs::path exemplarFilePath = fs::path(unit_test::k_TestFilesDir.view()) / "6_6_erode_dilate_test" / "6_6_erode_dilate_mask.dream3d";
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(k_ExemplarMaskPath));
  const auto& maskArrayRef = dataStructure.getDataRefAs<BoolArray>(k_ExemplarMaskPath);
  REQUIRE(maskArrayRef.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);

  const ErodeDilateMaskFilter filter;
  Arguments args;
  args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Dilate));
  args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(2));
  args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(true));
  args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(true));
  args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(true));
  args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_ExemplarMaskPath));
  args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputDataPath));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_EbsdScanDataPath, "Exemplar Mask Dilate");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter(Erode)", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_erode_dilate_test.tar.gz", "6_6_erode_dilate_test");
  const fs::path exemplarFilePath = fs::path(unit_test::k_TestFilesDir.view()) / "6_6_erode_dilate_test" / "6_6_erode_dilate_mask.dream3d";
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(k_ExemplarMaskPath));
  const auto& maskArrayRef = dataStructure.getDataRefAs<BoolArray>(k_ExemplarMaskPath);
  REQUIRE(maskArrayRef.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);

  const ErodeDilateMaskFilter filter;
  Arguments args;
  args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Erode));
  args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(2));
  args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(true));
  args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(true));
  args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(true));
  args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_ExemplarMaskPath));
  args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputDataPath));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_EbsdScanDataPath, "Exemplar Mask Erode");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Literal One-Dimensional Morphology", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  constexpr std::array<bool, 4> k_DilationInput = {false, true, false, false};
  constexpr std::array<bool, 4> k_DilationOneIteration = {true, true, true, false};
  constexpr std::array<bool, 4> k_DilationTwoIterations = {true, true, true, true};
  constexpr std::array<bool, 4> k_ErosionInput = {true, true, false, true};
  constexpr std::array<bool, 4> k_ErosionOneIteration = {true, false, false, false};
  const std::array<std::pair<std::string_view, std::array<usize, 3>>, 2> dimensions = {{{"X", {4, 1, 1}}, {"Z", {1, 1, 4}}}};

  for(const auto& [label, dimension] : dimensions)
  {
    DYNAMIC_SECTION(label)
    {
      DYNAMIC_SECTION("Dilate one iteration")
      {
        MaskLiteralFixture fixture = CreateMaskLiteralFixture(dimension[0], dimension[1], dimension[2], k_DilationInput);
        ExecuteMaskLiteralFixture(fixture, k_Dilate, 1, true, true, true);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath));
        const auto& actualMaskArrayRef = fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath));
        const auto& siblingValuesArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath);
        CheckMaskValues(actualMaskArrayRef, k_DilationOneIteration);
        CheckSiblingValues(siblingValuesArrayRef);
      }

      DYNAMIC_SECTION("Dilate two iterations")
      {
        MaskLiteralFixture fixture = CreateMaskLiteralFixture(dimension[0], dimension[1], dimension[2], k_DilationInput);
        ExecuteMaskLiteralFixture(fixture, k_Dilate, 2, true, true, true);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath));
        const auto& actualMaskArrayRef = fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath));
        const auto& siblingValuesArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath);
        CheckMaskValues(actualMaskArrayRef, k_DilationTwoIterations);
        CheckSiblingValues(siblingValuesArrayRef);
      }

      DYNAMIC_SECTION("Erode one iteration")
      {
        MaskLiteralFixture fixture = CreateMaskLiteralFixture(dimension[0], dimension[1], dimension[2], k_ErosionInput);
        ExecuteMaskLiteralFixture(fixture, k_Erode, 1, true, true, true);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath));
        const auto& actualMaskArrayRef = fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath));
        const auto& siblingValuesArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath);
        CheckMaskValues(actualMaskArrayRef, k_ErosionOneIteration);
        CheckSiblingValues(siblingValuesArrayRef);
      }

      DYNAMIC_SECTION("Disabled active axis")
      {
        MaskLiteralFixture fixture = CreateMaskLiteralFixture(dimension[0], dimension[1], dimension[2], k_DilationInput);
        ExecuteMaskLiteralFixture(fixture, k_Dilate, 1, false, true, false);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath));
        const auto& actualMaskArrayRef = fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath));
        const auto& siblingValuesArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath);
        CheckMaskValues(actualMaskArrayRef, k_DilationInput);
        CheckSiblingValues(siblingValuesArrayRef);
      }

      DYNAMIC_SECTION("Disabled active axis erosion")
      {
        MaskLiteralFixture fixture = CreateMaskLiteralFixture(dimension[0], dimension[1], dimension[2], k_ErosionInput);
        ExecuteMaskLiteralFixture(fixture, k_Erode, 1, false, true, false);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath));
        const auto& actualMaskArrayRef = fixture.dataStructure.getDataRefAs<BoolArray>(fixture.maskPath);
        REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath));
        const auto& siblingValuesArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(fixture.siblingValuesPath);
        CheckMaskValues(actualMaskArrayRef, k_ErosionInput);
        CheckSiblingValues(siblingValuesArrayRef);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ErodeDilateMaskFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";
  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ErodeDilateMaskFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ErodeDilateMaskFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      const auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      const auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);
      const auto* pipelineFilter = dynamic_cast<const PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);
      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ErodeDilateMaskFilter>::uuid);
      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(ErodeDilateMaskFilter::k_Operation_Key) == k_Dilate);
      CHECK(args.value<int32>(ErodeDilateMaskFilter::k_NumIterations_Key) == 5);
      CHECK(args.value<bool>(ErodeDilateMaskFilter::k_XDirOn_Key));
      CHECK(args.value<bool>(ErodeDilateMaskFilter::k_YDirOn_Key));
      CHECK(args.value<bool>(ErodeDilateMaskFilter::k_ZDirOn_Key));
      CHECK(args.value<DataPath>(ErodeDilateMaskFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
