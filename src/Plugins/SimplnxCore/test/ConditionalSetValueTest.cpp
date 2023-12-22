#include "SimplnxCore/Filters/ConditionalSetValue.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
template <typename T>
void ConditionalSetValueOverFlowTest(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& conditionalPath, const std::string& value)
{
  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>(value));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(conditionalPath));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(selectedDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());
}

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

TEST_CASE("SimplnxCore::ConditionalSetValue: Missing/Empty DataPaths", "[ConditionalSetValue]")
{
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;
  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  ConditionalSetValue filter;

  args.insertOrAssign(ConditionalSetValue::k_UseConditional_Key, std::make_any<bool>(true));

  // Preflight the filter and check result with empty values
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>(""));
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Invalid numeric value
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("asfasdf"));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Valid numeric value, but the boolean array and the input array are not set, should fail
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("5.0"));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Set the mask array but should still fail
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Set the input array, now should pass preflight.
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid() == true);
}

TEST_CASE("SimplnxCore::ConditionalSetValue: Test Algorithm Bool", "[ConditionalSetValue]")
{
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  std::shared_ptr<ImageGeom> imageGeometry = dataStructure.getSharedDataAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry->getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  DataObject* ciDataObject = dataStructure.getData(ciDataPath);

  DataArray<float32>* ciDataArray = dynamic_cast<Float32Array*>(ciDataObject);
  // Fill every value with 10.0 into the ciArray
  ciDataArray->fill(10.0);

  // Create a bool array where every value is TRUE
  std::vector<usize> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  BoolArray& conditionalArray = dataStructure.getDataRefAs<BoolArray>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray}));
  conditionalArray.fill(true);

  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  REQUIRE(RequireDataArrayEqualZero(*ciDataArray));

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/ConditionalSetValueTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::ConditionalSetValue: Test Algorithm UInt8", "[ConditionalSetValue]")
{
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  const ImageGeom& imageGeometry = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry.getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  auto& float32DataArray = dataStructure.getDataRefAs<Float32Array>(ciDataPath);
  // Fill every value with 10.0 into the ciArray
  float32DataArray.fill(10.0);

  // Create a bool array where every value is TRUE
  std::vector<usize> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  BoolArray& conditionalArray = dataStructure.getDataRefAs<BoolArray>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray}));
  conditionalArray.fill(true);

  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(RequireDataArrayEqualZero(float32DataArray));
}

TEST_CASE("SimplnxCore::ConditionalSetValue: Test Algorithm Int8", "[ConditionalSetValue]")
{
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  const ImageGeom& imageGeometry = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry.getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  auto& float32DataArray = dataStructure.getDataRefAs<Float32Array>(ciDataPath);
  // Fill every value with 10.0 into the ciArray
  float32DataArray.fill(10.0);

  // Create a bool array where every value is TRUE
  std::vector<usize> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  BoolArray& conditionalArray = dataStructure.getDataRefAs<BoolArray>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray}));
  conditionalArray.fill(true);

  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(RequireDataArrayEqualZero(float32DataArray));
}

TEST_CASE("SimplnxCore::ConditionalSetValue: Overflow/Underflow", "[ConditionalSetValue]")
{
  std::vector<size_t> imageDims = {40, 60, 80};
  FloatVec3 imageSpacing = {0.10F, 2.0F, 33.0F};
  FloatVec3 imageOrigin = {
      0.0f,
      22.0f,
      77.0f,
  };
  DataStructure dataStructure = UnitTest::CreateAllPrimitiveTypes(imageDims);

  // Get the DataGroups that we are going to add an Image Geometry into
  DataGroup* levelOneGroup = dataStructure.getDataAs<DataGroup>(DataPath({k_LevelZero, k_LevelOne}));
  REQUIRE(levelOneGroup != nullptr);
  DataGroup* levelTwoGroup = dataStructure.getDataAs<DataGroup>(DataPath({k_LevelZero, k_LevelTwo}));
  REQUIRE(levelTwoGroup != nullptr);

  // Add an ImageGeometry into each DataGroup
  UnitTest::AddImageGeometry(dataStructure, imageDims, imageSpacing, imageOrigin, *levelOneGroup);
  UnitTest::AddImageGeometry(dataStructure, imageDims, imageSpacing, imageOrigin, *levelTwoGroup);

  // Create a bool array where every value is TRUE
  std::vector<usize> tupleShape = {imageDims[2], imageDims[1], imageDims[0]};
  BoolArray* conditionalArray1 = UnitTest::CreateTestDataArray<bool>(dataStructure, k_ConditionalArray, tupleShape, {1}, levelOneGroup->getId());
  conditionalArray1->fill(true);
  BoolArray* conditionalArray2 = UnitTest::CreateTestDataArray<bool>(dataStructure, k_ConditionalArray, tupleShape, {1}, levelTwoGroup->getId());
  conditionalArray2->fill(true);

  DataPath conditionalDataPath({k_LevelZero, k_LevelOne, k_ConditionalArray});

  DataPath selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int8DataSet});
  ConditionalSetValueOverFlowTest<int8>(dataStructure, selectedDataPath, conditionalDataPath, "-130"); // underflow
  ConditionalSetValueOverFlowTest<int8>(dataStructure, selectedDataPath, conditionalDataPath, "130");  // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint8DataSet});
  ConditionalSetValueOverFlowTest<uint8>(dataStructure, selectedDataPath, conditionalDataPath, "-1");  // underflow
  ConditionalSetValueOverFlowTest<uint8>(dataStructure, selectedDataPath, conditionalDataPath, "260"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int16DataSet});
  ConditionalSetValueOverFlowTest<int16>(dataStructure, selectedDataPath, conditionalDataPath, "-32770"); // underflow
  ConditionalSetValueOverFlowTest<int16>(dataStructure, selectedDataPath, conditionalDataPath, "32770");  // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint16DataSet});
  ConditionalSetValueOverFlowTest<uint16>(dataStructure, selectedDataPath, conditionalDataPath, "-1");    // underflow
  ConditionalSetValueOverFlowTest<uint16>(dataStructure, selectedDataPath, conditionalDataPath, "65537"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int32DataSet});
  ConditionalSetValueOverFlowTest<int32>(dataStructure, selectedDataPath, conditionalDataPath, "-2147483649"); // underflow
  ConditionalSetValueOverFlowTest<int32>(dataStructure, selectedDataPath, conditionalDataPath, "2147483649");  // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint32DataSet});
  ConditionalSetValueOverFlowTest<uint32>(dataStructure, selectedDataPath, conditionalDataPath, "-1");         // underflow
  ConditionalSetValueOverFlowTest<uint32>(dataStructure, selectedDataPath, conditionalDataPath, "4294967297"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int64DataSet});
  ConditionalSetValueOverFlowTest<int64>(dataStructure, selectedDataPath, conditionalDataPath, "-92233720368547758080"); // underflow
  ConditionalSetValueOverFlowTest<int64>(dataStructure, selectedDataPath, conditionalDataPath, "92233720368547758080");  // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint64DataSet});
  ConditionalSetValueOverFlowTest<uint64>(dataStructure, selectedDataPath, conditionalDataPath, "-1");                    // underflow
  ConditionalSetValueOverFlowTest<uint64>(dataStructure, selectedDataPath, conditionalDataPath, "184467440737095516150"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Float32DataSet});
  ConditionalSetValueOverFlowTest<float32>(dataStructure, selectedDataPath, conditionalDataPath, "1.17549e-039");  // underflow
  ConditionalSetValueOverFlowTest<float32>(dataStructure, selectedDataPath, conditionalDataPath, "3.40282e+039");  // overflow
  ConditionalSetValueOverFlowTest<float32>(dataStructure, selectedDataPath, conditionalDataPath, "-1.17549e-039"); // underflow
  ConditionalSetValueOverFlowTest<float32>(dataStructure, selectedDataPath, conditionalDataPath, "-3.40282e+039"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Float64DataSet});
  ConditionalSetValueOverFlowTest<float64>(dataStructure, selectedDataPath, conditionalDataPath, "2.22507e-309");  // underflow
  ConditionalSetValueOverFlowTest<float64>(dataStructure, selectedDataPath, conditionalDataPath, "1.79769e+309");  // overflow
  ConditionalSetValueOverFlowTest<float64>(dataStructure, selectedDataPath, conditionalDataPath, "-2.22507e-309"); // underflow
  ConditionalSetValueOverFlowTest<float64>(dataStructure, selectedDataPath, conditionalDataPath, "-1.79769e+309"); // overflow
}

TEST_CASE("SimplnxCore::ConditionalSetValue: No Conditional", "[ConditionalSetValue]")
{
  ConditionalSetValue filter;
  Arguments args;

  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  std::shared_ptr<ImageGeom> imageGeometry = dataStructure.getSharedDataAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry->getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  DataObject* ciDataObject = dataStructure.getData(ciDataPath);

  DataArray<float32>* ciDataArray = dynamic_cast<Float32Array*>(ciDataObject);
  // Fill every value with 10.0 into the ciArray
  ciDataArray->fill(10.0);

  const std::string removeStr = "10.0";
  const auto removeVal = static_cast<float32>(ConvertTo<float32>::convert(removeStr).value());

  args.insertOrAssign(ConditionalSetValue::k_UseConditional_Key, std::make_any<bool>(false));
  args.insertOrAssign(ConditionalSetValue::k_RemoveValue_Key, std::make_any<std::string>(removeStr));
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  const auto& alteredArray = dataStructure.getDataRefAs<Float32Array>(ciDataPath);

  for(const auto& value : alteredArray)
  {
    REQUIRE(value != removeVal);
  }
}

TEST_CASE("SimplnxCore::ConditionalSetValue: Test Inverted Mask Algorithm Bool", "[ConditionalSetValue]")
{
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  const ImageGeom& imageGeometry = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  nx::core::SizeVec3 imageGeomDims = imageGeometry.getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  auto& float32DataArray = dataStructure.getDataRefAs<Float32Array>(ciDataPath);
  // Fill every value with 10.0 into the ciArray
  float32DataArray.fill(10.0);

  // Create a bool array where every value is TRUE
  std::vector<usize> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  BoolArray& conditionalArray = dataStructure.getDataRefAs<BoolArray>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray}));
  conditionalArray.fill(false);

  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_UseConditional_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValue::k_InvertMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(RequireDataArrayEqualZero(float32DataArray));
}
