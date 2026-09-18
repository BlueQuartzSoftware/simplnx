#include "SimplnxCore/Filters/ConditionalSetValueFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

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
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/ConditionalSetValueTest.dream3d", unit_test::k_BinaryTestOutputDir)));
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
      CHECK(args.value<std::string>(ConditionalSetValueFilter::k_ReplaceValue_Key) == "3.500000");
    }
  }
}
