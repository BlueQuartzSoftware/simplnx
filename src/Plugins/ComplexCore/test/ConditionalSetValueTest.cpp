

#include <catch2/catch.hpp>

#include "UnitTestCommon.hpp"

#include "ComplexCore/Filters/ConditionalSetValue.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include <iostream>
#include <string>

using namespace complex;
using namespace ComplexUnitTest::Constants;

TEST_CASE("ConditionalSetValue: Instantiate Filter", "[ConditionalSetValue]")
{
  ConditionalSetValue filter;
  DataStructure dataGraph;
  Arguments args;

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, "ConditionalArray"})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == false);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid() == false);
}

TEST_CASE("ConditionalSetValue: Missing/Empty DataPaths", "[ConditionalSetValue]")
{
  DataStructure dataGraph = ComplexUnitTest::CreateDataStructure();
  Arguments args;
  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  ConditionalSetValue filter;

  // Preflight the filter and check result with empty values
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>(""));
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == false);

  // Invalid numeric value
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("asfasdf"));
  preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == false);

  // Valid numeric value, but the boolean array and the input array are not set, should fail
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("5.0"));
  preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == false);

  // Set the mask array but should still fail
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, "ConditionalArray"})));
  preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == false);

  // Set the input array, now should pass preflight.
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));
  preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == true);
}

TEST_CASE("ConditionalSetValue: Test Algorithm", "[ConditionalSetValue]")
{
  DataStructure dataGraph = ComplexUnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  std::shared_ptr<ImageGeom> imageGeometry = dataGraph.getSharedDataAs<ImageGeom>(geomPath);
  complex::SizeVec3 imageGeomDims = imageGeometry->getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  DataObject* ciDataObject = dataGraph.getData(ciDataPath);

  DataArray<float32>* ciDataArray = dynamic_cast<Float32Array*>(ciDataObject);
  // Fill every value with 10.0 into the ciArray
  ciDataArray->fill(10.0);

  // Create a bool array where every value is TRUE
  std::vector<size_t> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  BoolArray* conditionalArray = ComplexUnitTest::CreateTestDataArray<bool>(dataGraph, "ConditionalArray", tupleShape, {1}, dataGraph.getId(ebsdScanPath).value());
  conditionalArray->fill(true);

  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, "ConditionalArray"})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  size_t numTuples = ciDataArray->getNumberOfTuples();
  for(size_t tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
  {
    REQUIRE(ciDataArray->at(tupleIndex) == 0.0);
  }
}

template <typename T>
void ConditionalSetValueOverFlowTest(DataStructure& dataGraph, const DataPath& selectedDataPath, const DataPath& conditionalPath, const std::string& value)
{
  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>(value));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(conditionalPath));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(selectedDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == false);
}

TEST_CASE("ConditionalSetValue: Overflow/Underflow", "[ConditionalSetValue]")
{

  std::vector<size_t> imageDims = {40, 60, 80};
  FloatVec3 imageSpacing = {0.10F, 2.0F, 33.0F};
  FloatVec3 imageOrigin = {
      0.0F,
      22.0F,
      77.0F,
  };
  DataStructure dataGraph = ComplexUnitTest::CreateAllPrimitiveTypes(imageDims);

  // Get the DataGroups that we are going to add an Image Geometry into
  DataGroup* levelOneGroup = dataGraph.getDataAs<DataGroup>(DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne}));
  DataGroup* levelTwoGroup = dataGraph.getDataAs<DataGroup>(DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelTwo}));

  // Add an ImageGeometry into each DataGroup
  ComplexUnitTest::AddImageGeometry(dataGraph, imageDims, imageSpacing, imageOrigin, levelOneGroup);
  ComplexUnitTest::AddImageGeometry(dataGraph, imageDims, imageSpacing, imageOrigin, levelTwoGroup);

  // Create a bool array where every value is TRUE
  std::vector<size_t> tupleShape = {imageDims[2], imageDims[1], imageDims[0]};
  BoolArray* conditionalArray1 = ComplexUnitTest::CreateTestDataArray<bool>(dataGraph, ComplexUnitTest::Constants::k_ConditionalArray, tupleShape, {1}, levelOneGroup->getId());
  conditionalArray1->fill(true);
  BoolArray* conditionalArray2 = ComplexUnitTest::CreateTestDataArray<bool>(dataGraph, ComplexUnitTest::Constants::k_ConditionalArray, tupleShape, {1}, levelTwoGroup->getId());
  conditionalArray2->fill(true);

  DataPath conditionalDataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_ConditionalArray});

  DataPath selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Int8});
  ConditionalSetValueOverFlowTest<int8_t>(dataGraph, selectedDataPath, conditionalDataPath, "-130"); // underflow
  ConditionalSetValueOverFlowTest<int8_t>(dataGraph, selectedDataPath, conditionalDataPath, "130");  // overflow

  selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Uint8});
  ConditionalSetValueOverFlowTest<uint8_t>(dataGraph, selectedDataPath, conditionalDataPath, "-1");  // underflow
  ConditionalSetValueOverFlowTest<uint8_t>(dataGraph, selectedDataPath, conditionalDataPath, "260"); // overflow

  selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Int16});
  ConditionalSetValueOverFlowTest<int16_t>(dataGraph, selectedDataPath, conditionalDataPath, "-32770"); // underflow
  ConditionalSetValueOverFlowTest<int16_t>(dataGraph, selectedDataPath, conditionalDataPath, "32770");  // overflow

  selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Uint16});
  ConditionalSetValueOverFlowTest<uint16_t>(dataGraph, selectedDataPath, conditionalDataPath, "-1");    // underflow
  ConditionalSetValueOverFlowTest<uint16_t>(dataGraph, selectedDataPath, conditionalDataPath, "65537"); // overflow

  selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Int32});
  ConditionalSetValueOverFlowTest<int32_t>(dataGraph, selectedDataPath, conditionalDataPath, "-2147483649"); // underflow
  ConditionalSetValueOverFlowTest<int32_t>(dataGraph, selectedDataPath, conditionalDataPath, "2147483649");  // overflow

  selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Uint32});
  ConditionalSetValueOverFlowTest<uint32_t>(dataGraph, selectedDataPath, conditionalDataPath, "-1");         // underflow
  ConditionalSetValueOverFlowTest<uint32_t>(dataGraph, selectedDataPath, conditionalDataPath, "4294967297"); // overflow

  selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Int64});
  ConditionalSetValueOverFlowTest<int64_t>(dataGraph, selectedDataPath, conditionalDataPath, "-92233720368547758080"); // underflow
  ConditionalSetValueOverFlowTest<int64_t>(dataGraph, selectedDataPath, conditionalDataPath, "92233720368547758080");  // overflow

  selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Uint64});
  ConditionalSetValueOverFlowTest<uint64_t>(dataGraph, selectedDataPath, conditionalDataPath, "-1");                    // underflow
  ConditionalSetValueOverFlowTest<uint64_t>(dataGraph, selectedDataPath, conditionalDataPath, "184467440737095516150"); // overflow

  selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Float});
  ConditionalSetValueOverFlowTest<float>(dataGraph, selectedDataPath, conditionalDataPath, "1.17549e-039");  // underflow
  ConditionalSetValueOverFlowTest<float>(dataGraph, selectedDataPath, conditionalDataPath, "3.40282e+039");  // overflow
  ConditionalSetValueOverFlowTest<float>(dataGraph, selectedDataPath, conditionalDataPath, "-1.17549e-039"); // underflow
  ConditionalSetValueOverFlowTest<float>(dataGraph, selectedDataPath, conditionalDataPath, "-3.40282e+039"); // overflow

  selectedDataPath = DataPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne, ComplexUnitTest::Constants::k_Double});
  ConditionalSetValueOverFlowTest<double>(dataGraph, selectedDataPath, conditionalDataPath, "2.22507e-309");  // underflow
  ConditionalSetValueOverFlowTest<double>(dataGraph, selectedDataPath, conditionalDataPath, "1.79769e+309");  // overflow
  ConditionalSetValueOverFlowTest<double>(dataGraph, selectedDataPath, conditionalDataPath, "-2.22507e-309"); // underflow
  ConditionalSetValueOverFlowTest<double>(dataGraph, selectedDataPath, conditionalDataPath, "-1.79769e+309"); // overflow
}
