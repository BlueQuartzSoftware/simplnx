#include "SimplnxCore/Filters/ConditionalSetValueFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <nonstd/span.hpp>

#include <any>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <utility>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
/**
 * @brief Requires preflight to reject one replacement value for a selected array type.
 * @tparam T Specifies the selected array element type.
 * @param dataStructure Contains the selected array and mask.
 * @param selectedDataPath Array whose type constrains the replacement value.
 * @param conditionalPath Boolean mask array path.
 * @param value Replacement value text that must be outside the selected type's range.
 */
template <typename T>
void ConditionalSetValueOverFlowTest(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& conditionalPath, const std::string& value)
{
  ConditionalSetValueFilter filter;
  Arguments args;
  args.insertOrAssign(ConditionalSetValueFilter::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>(value));
  args.insertOrAssign(ConditionalSetValueFilter::k_ConditionalArrayPath_Key, std::make_any<DataPath>(conditionalPath));
  args.insertOrAssign(ConditionalSetValueFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(selectedDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

/**
 * @brief Tests whether each tuple's first value equals zero.
 * @tparam T Specifies the array element type.
 * @param data Array to inspect.
 * @return True if each tested value equals zero.
 */
template <class T>
bool RequireDataArrayEqualZero(const DataArray<T>& data)
{
  usize numTuples = data.getNumberOfTuples();
  for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
  {
    if(data.at(tupleIndex) != 0.0)
    {
      return false;
    }
  }

  return true;
}

Arguments convertLegacyNumericArguments(float64 value, bool hasRemoveValue)
{
  const nlohmann::json arrayPath = {{"Data Container Name", "DataContainer"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "TestArray"}};
  nlohmann::json legacyJson = {{"ReplaceValue", value}};
  if(hasRemoveValue)
  {
    legacyJson["RemoveValue"] = value;
    legacyJson["SelectedArray"] = arrayPath;
  }
  else
  {
    legacyJson["ConditionalArrayPath"] = arrayPath;
    legacyJson["SelectedArrayPath"] = arrayPath;
  }

  const auto conversionResult = ConditionalSetValueFilter::FromSIMPLJson(legacyJson);
  SIMPLNX_RESULT_REQUIRE_VALID(conversionResult);
  return conversionResult.value();
}

template <typename T>
void requireLegacyIntegerValue(float64 value, bool hasRemoveValue, T expected)
{
  const auto args = convertLegacyNumericArguments(value, hasRemoveValue);
  for(const auto key : std::array<StringLiteral, 2>{ConditionalSetValueFilter::k_ReplaceValue_Key, ConditionalSetValueFilter::k_RemoveValue_Key})
  {
    if(!hasRemoveValue && key.view() == ConditionalSetValueFilter::k_RemoveValue_Key.view())
    {
      continue;
    }
    const auto text = args.value<std::string>(key);
    CAPTURE(value, text, expected, hasRemoveValue);
    const auto result = StringInterpretationUtilities::Convert<T>(text);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK((result.value() == expected));
  }
}

template <typename T>
void requireLegacyIntegerError(float64 value, bool hasRemoveValue, int32 expectedCode)
{
  const auto args = convertLegacyNumericArguments(value, hasRemoveValue);
  for(const auto key : std::array<StringLiteral, 2>{ConditionalSetValueFilter::k_ReplaceValue_Key, ConditionalSetValueFilter::k_RemoveValue_Key})
  {
    if(!hasRemoveValue && key.view() == ConditionalSetValueFilter::k_RemoveValue_Key.view())
    {
      continue;
    }
    const auto text = args.value<std::string>(key);
    CAPTURE(value, text, expectedCode, hasRemoveValue);
    const auto result = StringInterpretationUtilities::Convert<T>(text);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE((result.errors().size() == 1));
    CHECK((result.errors().front().code == expectedCode));
  }
}

Arguments convertLegacyBooleanArguments(float64 removeValue, bool masked, float64 replaceValue)
{
  const nlohmann::json selectedPath = {{"Data Container Name", "DataContainer"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "TestArray"}};
  nlohmann::json legacyJson = {{"ReplaceValue", replaceValue}};
  if(masked)
  {
    legacyJson["SelectedArrayPath"] = selectedPath;
    legacyJson["ConditionalArrayPath"] = {{"Data Container Name", "DataContainer"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Mask"}};
  }
  else
  {
    legacyJson["SelectedArray"] = selectedPath;
    legacyJson["RemoveValue"] = removeValue;
  }
  const auto conversionResult = ConditionalSetValueFilter::FromSIMPLJson(legacyJson);
  SIMPLNX_RESULT_REQUIRE_VALID(conversionResult);
  auto args = conversionResult.value();
  // Legacy conversion does not set this flag. Select the mask explicitly to test masked execution.
  args.insertOrAssign(ConditionalSetValueFilter::k_UseConditional_Key, std::make_any<bool>(masked));
  return args;
}

template <typename Execute>
void requireBooleanExecution(const Arguments& args, bool removeTruth, bool replaceTruth, bool masked, bool outOfCore, const Execute& execute)
{
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "DataContainer");
  REQUIRE((groupPtr != nullptr));
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {4}, groupPtr->getId());
  REQUIRE((cellDataPtr != nullptr));
  const DataPath selectedPath({"DataContainer", "CellData", "TestArray"});
  const DataPath maskPath = selectedPath.replaceName("Mask");
  auto selectedStore = DataStoreUtilities::CreateDataStore<bool>(dataStructure, selectedPath, {4}, {1});
  auto maskStore = DataStoreUtilities::CreateDataStore<bool>(dataStructure, maskPath, {4}, {1});
  REQUIRE((selectedStore != nullptr));
  REQUIRE((maskStore != nullptr));
  auto* selectedPtr = BoolArray::Create(dataStructure, "TestArray", selectedStore, cellDataPtr->getId());
  auto* maskPtr = BoolArray::Create(dataStructure, "Mask", maskStore, cellDataPtr->getId());
  REQUIRE((selectedPtr != nullptr));
  REQUIRE((maskPtr != nullptr));
  const auto expectedStore = outOfCore ? IDataStore::StoreType::OutOfCore : IDataStore::StoreType::InMemory;
  REQUIRE((selectedStore->getStoreType() == expectedStore));
  REQUIRE((maskStore->getStoreType() == expectedStore));
  if(outOfCore)
  {
    REQUIRE((selectedStore->getDataFormat() == "HDF5-OOC"));
    REQUIRE((maskStore->getDataFormat() == "HDF5-OOC"));
  }
  const std::array<bool, 4> initial = {false, true, false, true};
  const std::array<bool, 4> mask = {true, true, false, false};
  const auto initializeValues = selectedStore->copyFromBuffer(0, nonstd::span<const bool>(initial.data(), initial.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(initializeValues);
  const auto initializeMask = maskStore->copyFromBuffer(0, nonstd::span<const bool>(mask.data(), mask.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(initializeMask);

  const ConditionalSetValueFilter filter;
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execution = execute(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execution.result);
  std::array<bool, 4> actual = {};
  const auto readValues = selectedStore->copyIntoBuffer(0, nonstd::span<bool>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readValues);
  for(usize index = 0; index < initial.size(); ++index)
  {
    CAPTURE(index, removeTruth, replaceTruth, masked, outOfCore);
    const bool shouldReplace = masked ? mask.at(index) : initial.at(index) == removeTruth;
    const bool expected = shouldReplace ? replaceTruth : initial.at(index);
    CHECK((actual.at(index) == expected));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Missing/Empty DataPaths", "[ConditionalSetValueFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;
  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  ConditionalSetValueFilter filter;

  args.insertOrAssign(ConditionalSetValueFilter::k_UseConditional_Key, std::make_any<bool>(true));

  // Empty replacement text must fail before array paths are available.
  args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>(""));
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Nonnumeric replacement text must fail before array paths are available.
  args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>("asfasdf"));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Valid replacement text still requires both selected and conditional arrays.
  args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>("5.0"));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // A conditional array alone is not sufficient.
  args.insertOrAssign(ConditionalSetValueFilter::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Both required paths make the configuration valid.
  args.insertOrAssign(ConditionalSetValueFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid() == true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Test Algorithm Bool", "[ConditionalSetValueFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  std::shared_ptr<ImageGeom> imageGeometry = dataStructure.getSharedDataAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry->getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  DataObject* ciDataObject = dataStructure.getData(ciDataPath);

  DataArray<float32>* ciDataArray = dynamic_cast<Float32Array*>(ciDataObject);
  // Initialize the selected values before conditional replacement.
  ciDataArray->fill(10.0);

  // A true mask selects every tuple.
  ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  BoolArray& conditionalArray = dataStructure.getDataRefAs<BoolArray>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray}));
  conditionalArray.fill(true);

  ConditionalSetValueFilter filter;
  Arguments args;
  // Replace all selected values with zero.
  args.insertOrAssign(ConditionalSetValueFilter::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValueFilter::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValueFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(RequireDataArrayEqualZero(*ciDataArray));

  // The optional output supports manual inspection of the replaced values.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/ConditionalSetValueTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Test Algorithm UInt8", "[ConditionalSetValueFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  const ImageGeom& imageGeometry = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry.getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  auto& float32DataArray = dataStructure.getDataRefAs<Float32Array>(ciDataPath);
  // Initialize the selected values before conditional replacement.
  float32DataArray.fill(10.0);

  // A true mask selects every tuple.
  ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  BoolArray& conditionalArray = dataStructure.getDataRefAs<BoolArray>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray}));
  conditionalArray.fill(true);

  ConditionalSetValueFilter filter;
  Arguments args;
  // Replace all selected values with zero.
  args.insertOrAssign(ConditionalSetValueFilter::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValueFilter::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValueFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(RequireDataArrayEqualZero(float32DataArray));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Test Algorithm Int8", "[ConditionalSetValueFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  const ImageGeom& imageGeometry = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry.getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  auto& float32DataArray = dataStructure.getDataRefAs<Float32Array>(ciDataPath);
  // Initialize the selected values before conditional replacement.
  float32DataArray.fill(10.0);

  // A true mask selects every tuple.
  ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  BoolArray& conditionalArray = dataStructure.getDataRefAs<BoolArray>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray}));
  conditionalArray.fill(true);

  ConditionalSetValueFilter filter;
  Arguments args;
  // Replace all selected values with zero.
  args.insertOrAssign(ConditionalSetValueFilter::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValueFilter::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValueFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(RequireDataArrayEqualZero(float32DataArray));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Overflow/Underflow", "[ConditionalSetValueFilter]")
{
  UnitTest::LoadPlugins();

  std::vector<size_t> imageDims = {40, 60, 80};
  FloatVec3 imageSpacing = {0.10F, 2.0F, 33.0F};
  FloatVec3 imageOrigin = {
      0.0f,
      22.0f,
      77.0f,
  };
  DataStructure dataStructure = UnitTest::CreateAllPrimitiveTypes(imageDims);

  // Create arrays of each numeric type for the range checks.
  DataGroup* levelOneGroup = dataStructure.getDataAs<DataGroup>(DataPath({k_LevelZero, k_LevelOne}));
  REQUIRE(levelOneGroup != nullptr);
  DataGroup* levelTwoGroup = dataStructure.getDataAs<DataGroup>(DataPath({k_LevelZero, k_LevelTwo}));
  REQUIRE(levelTwoGroup != nullptr);

  UnitTest::AddImageGeometry(dataStructure, imageDims, imageSpacing, imageOrigin, *levelOneGroup);
  UnitTest::AddImageGeometry(dataStructure, imageDims, imageSpacing, imageOrigin, *levelTwoGroup);

  // A true mask makes each range check reach the selected array.
  ShapeType tupleShape = {imageDims[2], imageDims[1], imageDims[0]};
  BoolArray* conditionalArray1 = UnitTest::CreateTestDataArray<bool>(dataStructure, k_ConditionalArray, tupleShape, {1}, levelOneGroup->getId());
  conditionalArray1->fill(true);
  BoolArray* conditionalArray2 = UnitTest::CreateTestDataArray<bool>(dataStructure, k_ConditionalArray, tupleShape, {1}, levelTwoGroup->getId());
  conditionalArray2->fill(true);

  DataPath conditionalDataPath({k_LevelZero, k_LevelOne, k_ConditionalArray});

  DataPath selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int8DataSet});
  ConditionalSetValueOverFlowTest<int8>(dataStructure, selectedDataPath, conditionalDataPath, "-130"); // The value is below the supported range.
  ConditionalSetValueOverFlowTest<int8>(dataStructure, selectedDataPath, conditionalDataPath, "130");  // The value is above the supported range.

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint8DataSet});
  ConditionalSetValueOverFlowTest<uint8>(dataStructure, selectedDataPath, conditionalDataPath, "-1");  // The value is below the supported range.
  ConditionalSetValueOverFlowTest<uint8>(dataStructure, selectedDataPath, conditionalDataPath, "260"); // The value is above the supported range.

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int16DataSet});
  ConditionalSetValueOverFlowTest<int16>(dataStructure, selectedDataPath, conditionalDataPath, "-32770"); // The value is below the supported range.
  ConditionalSetValueOverFlowTest<int16>(dataStructure, selectedDataPath, conditionalDataPath, "32770");  // The value is above the supported range.

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint16DataSet});
  ConditionalSetValueOverFlowTest<uint16>(dataStructure, selectedDataPath, conditionalDataPath, "-1");    // The value is below the supported range.
  ConditionalSetValueOverFlowTest<uint16>(dataStructure, selectedDataPath, conditionalDataPath, "65537"); // The value is above the supported range.

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int32DataSet});
  ConditionalSetValueOverFlowTest<int32>(dataStructure, selectedDataPath, conditionalDataPath, "-2147483649"); // The value is below the supported range.
  ConditionalSetValueOverFlowTest<int32>(dataStructure, selectedDataPath, conditionalDataPath, "2147483649");  // The value is above the supported range.

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint32DataSet});
  ConditionalSetValueOverFlowTest<uint32>(dataStructure, selectedDataPath, conditionalDataPath, "-1");         // The value is below the supported range.
  ConditionalSetValueOverFlowTest<uint32>(dataStructure, selectedDataPath, conditionalDataPath, "4294967297"); // The value is above the supported range.

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int64DataSet});
  ConditionalSetValueOverFlowTest<int64>(dataStructure, selectedDataPath, conditionalDataPath, "-92233720368547758080"); // The value is below the supported range.
  ConditionalSetValueOverFlowTest<int64>(dataStructure, selectedDataPath, conditionalDataPath, "92233720368547758080");  // The value is above the supported range.

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint64DataSet});
  ConditionalSetValueOverFlowTest<uint64>(dataStructure, selectedDataPath, conditionalDataPath, "-1");                    // The value is below the supported range.
  ConditionalSetValueOverFlowTest<uint64>(dataStructure, selectedDataPath, conditionalDataPath, "184467440737095516150"); // The value is above the supported range.

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32) || defined(_MSC_VER)
  /*
   * Windows string conversion accepts the selected subnormal values. Unix-like
   * conversion reports underflow for these values. Run these platform-specific
   * underflow expectations only on non-Windows systems.
   */
#else
  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Float32DataSet});
  ConditionalSetValueOverFlowTest<float32>(dataStructure, selectedDataPath, conditionalDataPath, "1.17549e-039");  // The value is below the supported range.
  ConditionalSetValueOverFlowTest<float32>(dataStructure, selectedDataPath, conditionalDataPath, "3.40282e+039");  // The value is above the supported range.
  ConditionalSetValueOverFlowTest<float32>(dataStructure, selectedDataPath, conditionalDataPath, "-1.17549e-039"); // The value is below the supported range.
  ConditionalSetValueOverFlowTest<float32>(dataStructure, selectedDataPath, conditionalDataPath, "-3.40282e+039"); // The value is above the supported range.

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Float64DataSet});
  ConditionalSetValueOverFlowTest<float64>(dataStructure, selectedDataPath, conditionalDataPath, "2.22507e-309");  // The value is below the supported range.
  ConditionalSetValueOverFlowTest<float64>(dataStructure, selectedDataPath, conditionalDataPath, "1.79769e+309");  // The value is above the supported range.
  ConditionalSetValueOverFlowTest<float64>(dataStructure, selectedDataPath, conditionalDataPath, "-2.22507e-309"); // The value is below the supported range.
  ConditionalSetValueOverFlowTest<float64>(dataStructure, selectedDataPath, conditionalDataPath, "-1.79769e+309"); // The value is above the supported range.
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: No Conditional", "[ConditionalSetValueFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  ConditionalSetValueFilter filter;
  Arguments args;

  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  std::shared_ptr<ImageGeom> imageGeometry = dataStructure.getSharedDataAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry->getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  auto* ciDataArray = dataStructure.getDataAs<Float32Array>(ciDataPath);
  // Initialize the selected values before unconditional replacement.
  ciDataArray->fill(10.0);

  const std::string removeStr = "10.0";
  const auto removeVal = static_cast<float32>(StringInterpretationUtilities::Convert<float32>(removeStr).value());

  args.insertOrAssign(ConditionalSetValueFilter::k_UseConditional_Key, std::make_any<bool>(false));
  args.insertOrAssign(ConditionalSetValueFilter::k_RemoveValue_Key, std::make_any<std::string>(removeStr));
  args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValueFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& alteredArray = dataStructure.getDataRefAs<Float32Array>(ciDataPath);

  for(const auto& value : alteredArray)
  {
    REQUIRE(value != removeVal);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Test Inverted Mask Algorithm Bool", "[ConditionalSetValueFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  const ImageGeom& imageGeometry = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry.getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  auto& float32DataArray = dataStructure.getDataRefAs<Float32Array>(ciDataPath);
  // Initialize the selected values before inverted-mask replacement.
  float32DataArray.fill(10.0);

  // A false mask selects every tuple when mask inversion is enabled.
  ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  BoolArray& conditionalArray = dataStructure.getDataRefAs<BoolArray>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray}));
  conditionalArray.fill(false);

  ConditionalSetValueFilter filter;
  Arguments args;
  // Replace all inverted-mask selections with zero.
  args.insertOrAssign(ConditionalSetValueFilter::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValueFilter::k_InvertMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValueFilter::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValueFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(RequireDataArrayEqualZero(float32DataArray));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Legacy numeric arguments preserve precision", "[SimplnxCore][ConditionalSetValueFilter][BackwardsCompatibility]")
{
  const bool hasRemoveValue = GENERATE(true, false);
  for(const auto& [removeValue, replaceValue] : std::array<std::pair<float64, float64>, 2>{{{1e-7, -1e-7}, {1.0000000596046446, -13.25}}})
  {
    CAPTURE(hasRemoveValue, removeValue, replaceValue);
    const nlohmann::json arrayPath = {{"Data Container Name", "DataContainer"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "TestArray"}};
    nlohmann::json legacyJson = {{"ReplaceValue", replaceValue}};
    if(hasRemoveValue)
    {
      legacyJson["RemoveValue"] = removeValue;
      legacyJson["SelectedArray"] = arrayPath;
    }
    else
    {
      legacyJson["ConditionalArrayPath"] = arrayPath;
      legacyJson["SelectedArrayPath"] = arrayPath;
    }

    const auto conversionResult = ConditionalSetValueFilter::FromSIMPLJson(legacyJson);
    SIMPLNX_RESULT_REQUIRE_VALID(conversionResult);
    const auto& args = conversionResult.value();
    const auto replacementResult = StringInterpretationUtilities::Convert<float64>(args.value<std::string>(ConditionalSetValueFilter::k_ReplaceValue_Key));
    SIMPLNX_RESULT_REQUIRE_VALID(replacementResult);
    CHECK((replacementResult.value() == replaceValue));
    if(hasRemoveValue)
    {
      const auto removalResult = StringInterpretationUtilities::Convert<float64>(args.value<std::string>(ConditionalSetValueFilter::k_RemoveValue_Key));
      SIMPLNX_RESULT_REQUIRE_VALID(removalResult);
      CHECK((removalResult.value() == removeValue));
    }
  }
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Legacy numeric arguments retain integer interpretation", "[SimplnxCore][ConditionalSetValueFilter][BackwardsCompatibility]")
{
  const bool hasRemoveValue = GENERATE(true, false);
  requireLegacyIntegerValue<int64>(1e-7, hasRemoveValue, 0);
  requireLegacyIntegerValue<int64>(-1e-7, hasRemoveValue, 0);
  requireLegacyIntegerValue<int8>(1e-7, hasRemoveValue, 0);
  requireLegacyIntegerValue<int8>(-1e-7, hasRemoveValue, 0);
  requireLegacyIntegerValue<uint64>(1e-7, hasRemoveValue, 0);
  requireLegacyIntegerValue<uint8>(1e-7, hasRemoveValue, 0);
  requireLegacyIntegerValue<int64>(1e18, hasRemoveValue, 1000000000000000000);
  requireLegacyIntegerValue<uint64>(1e18, hasRemoveValue, 1000000000000000000ULL);
  requireLegacyIntegerValue<int64>(1000000000000000256.0, hasRemoveValue, 1000000000000000256);
  requireLegacyIntegerValue<uint64>(1000000000000000256.0, hasRemoveValue, 1000000000000000256ULL);
  requireLegacyIntegerValue<int64>(-9223372036854775808.0, hasRemoveValue, std::numeric_limits<int64>::lowest());
  requireLegacyIntegerError<int64>(9223372036854775808.0, hasRemoveValue, -10352);
  requireLegacyIntegerError<int64>(-9223372036854777856.0, hasRemoveValue, -10352);
  requireLegacyIntegerError<uint64>(18446744073709551616.0, hasRemoveValue, -10352);
  requireLegacyIntegerError<uint64>(-1e-7, hasRemoveValue, -10350);
  requireLegacyIntegerError<int8>(1e18, hasRemoveValue, -10353);
  requireLegacyIntegerError<uint8>(1e18, hasRemoveValue, -10353);
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Legacy Boolean numeric execution", "[SimplnxCore][ConditionalSetValueFilter][BackwardsCompatibility]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  for(const auto& [value, truth] : std::array<std::pair<float64, bool>, 6>{{{0.99999999, true}, {-0.99999999, true}, {1e-7, true}, {-1e-7, true}, {0.0, false}, {-0.0, false}}})
  {
    CAPTURE(value, truth);
    for(const bool masked : {false, true})
    {
      const auto args = convertLegacyBooleanArguments(0.0, masked, value);
      requireBooleanExecution(args, false, truth, masked, false,
                              [&](const auto& filter, auto& dataStructure, const auto& executionArgs) { return scope.executeFilter(filter, dataStructure, executionArgs); });
    }
    const auto args = convertLegacyBooleanArguments(value, false, truth ? 0.0 : 1.0);
    requireBooleanExecution(args, truth, !truth, false, false,
                            [&](const auto& filter, auto& dataStructure, const auto& executionArgs) { return scope.executeFilter(filter, dataStructure, executionArgs); });
  }
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: Boolean text execution controls", "[SimplnxCore][ConditionalSetValueFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  for(const auto& [text, truth] : std::array<std::pair<std::string, bool>, 10>{
          {{"TRUE", true}, {"True", true}, {"true", true}, {"FALSE", false}, {"False", false}, {"false", false}, {"not numeric", true}, {"0suffix", false}, {"1e-999", true}, {"-1e-999", true}}})
  {
    CAPTURE(text, truth);
    for(const bool masked : {false, true})
    {
      auto args = convertLegacyBooleanArguments(0.0, masked, 0.0);
      args.insertOrAssign(ConditionalSetValueFilter::k_ReplaceValue_Key, std::make_any<std::string>(text));
      requireBooleanExecution(args, false, truth, masked, false,
                              [&](const auto& filter, auto& dataStructure, const auto& executionArgs) { return scope.executeFilter(filter, dataStructure, executionArgs); });
    }
    auto args = convertLegacyBooleanArguments(0.0, false, truth ? 0.0 : 1.0);
    args.insertOrAssign(ConditionalSetValueFilter::k_RemoveValue_Key, std::make_any<std::string>(text));
    requireBooleanExecution(args, truth, !truth, false, false,
                            [&](const auto& filter, auto& dataStructure, const auto& executionArgs) { return scope.executeFilter(filter, dataStructure, executionArgs); });
  }
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: real-store legacy Boolean execution", "[SimplnxCore][ConditionalSetValueFilter][.OocStoreContract]")
{
  REQUIRE((Application::GetOrCreateInstance()->getIOManager("HDF5-OOC") != nullptr));
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  for(const bool masked : {false, true})
  {
    const auto args = convertLegacyBooleanArguments(0.0, masked, 0.99999999);
    requireBooleanExecution(args, false, true, masked, true, [](const auto& filter, auto& dataStructure, const auto& executionArgs) { return filter.execute(dataStructure, executionArgs); });
  }
  const auto args = convertLegacyBooleanArguments(-0.99999999, false, 0.0);
  requireBooleanExecution(args, true, false, false, true, [](const auto& filter, auto& dataStructure, const auto& executionArgs) { return filter.execute(dataStructure, executionArgs); });
}

TEST_CASE("SimplnxCore::ConditionalSetValueFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ConditionalSetValueFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ConditionalSetValueFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ConditionalSetValueFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ConditionalSetValueFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ConditionalSetValueFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ConditionalSetValueFilter::k_ConditionalArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      const auto replacementResult = StringInterpretationUtilities::Convert<float64>(args.value<std::string>(ConditionalSetValueFilter::k_ReplaceValue_Key));
      SIMPLNX_RESULT_REQUIRE_VALID(replacementResult);
      CHECK((replacementResult.value() == 3.5));
    }
  }
}
