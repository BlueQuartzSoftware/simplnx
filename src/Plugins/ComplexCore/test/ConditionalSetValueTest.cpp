#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ConditionalSetValue.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include <string>

using namespace complex;
using namespace complex::Constants;

namespace
{
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

TEST_CASE("ConditionalSetValue: Instantiate Filter", "[ConditionalSetValue]")
{
  ConditionalSetValue filter;
  DataStructure dataGraph;
  Arguments args;

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(!executeResult.result.valid());
}

TEST_CASE("ConditionalSetValue: Missing/Empty DataPaths", "[ConditionalSetValue]")
{
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  Arguments args;
  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  ConditionalSetValue filter;

  // Preflight the filter and check result with empty values
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>(""));
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Invalid numeric value
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("asfasdf"));
  preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Valid numeric value, but the boolean array and the input array are not set, should fail
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("5.0"));
  preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Set the mask array but should still fail
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Set the input array, now should pass preflight.
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));
  preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == true);
}

TEST_CASE("ConditionalSetValue: Test Algorithm Bool", "[ConditionalSetValue]")
{
  DataStructure dataGraph = UnitTest::CreateDataStructure();
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
  std::vector<usize> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  BoolArray* conditionalArray = UnitTest::CreateTestDataArray<bool>(dataGraph, k_ConditionalArray, tupleShape, {1}, dataGraph.getId(ebsdScanPath).value());
  conditionalArray->fill(true);

  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

#if 1
  // Write out the DataStructure for later viewing/debugging
  std::string filePath = fmt::format("{}/ConditionalSetValueTest.dream3d", unit_test::k_BinaryDir);
  // std::cout << "Writing file to: " << filePath << std::endl;
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
#endif

  REQUIRE(RequireDataArrayEqualZero(*ciDataArray));
}

TEST_CASE("ConditionalSetValue: Test Algorithm UInt8", "[ConditionalSetValue]")
{
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  const ImageGeom& imageGeometry = dataGraph.getDataRefAs<ImageGeom>(geomPath);
  complex::SizeVec3 imageGeomDims = imageGeometry.getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  auto& float32DataArray = dataGraph.getDataRefAs<Float32Array>(ciDataPath);
  // Fill every value with 10.0 into the ciArray
  float32DataArray.fill(10.0);

  // Create a bool array where every value is TRUE
  std::vector<usize> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  UInt8Array* conditionalArray = UnitTest::CreateTestDataArray<uint8>(dataGraph, k_ConditionalArray, tupleShape, {1}, dataGraph.getId(ebsdScanPath).value());
  conditionalArray->fill(1);

  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(RequireDataArrayEqualZero(float32DataArray));
}

TEST_CASE("ConditionalSetValue: Test Algorithm Int8", "[ConditionalSetValue]")
{
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  DataPath ebsdScanPath = DataPath({k_SmallIN100, k_EbsdScanData});
  DataPath geomPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ImageGeometry});
  const ImageGeom& imageGeometry = dataGraph.getDataRefAs<ImageGeom>(geomPath);
  complex::SizeVec3 imageGeomDims = imageGeometry.getDimensions();

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  auto& float32DataArray = dataGraph.getDataRefAs<Float32Array>(ciDataPath);
  // Fill every value with 10.0 into the ciArray
  float32DataArray.fill(10.0);

  // Create a bool array where every value is TRUE
  std::vector<usize> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  Int8Array* conditionalArray = UnitTest::CreateTestDataArray<int8>(dataGraph, k_ConditionalArray, tupleShape, {1}, dataGraph.getId(ebsdScanPath).value());
  conditionalArray->fill(1);

  ConditionalSetValue filter;
  Arguments args;
  // Replace every value with a zero
  args.insertOrAssign(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<std::string>("0.0"));
  args.insertOrAssign(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_ConditionalArray})));
  args.insertOrAssign(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(RequireDataArrayEqualZero(float32DataArray));
}

TEST_CASE("ConditionalSetValue: Overflow/Underflow", "[ConditionalSetValue]")
{
  std::vector<size_t> imageDims = {40, 60, 80};
  FloatVec3 imageSpacing = {0.10F, 2.0F, 33.0F};
  FloatVec3 imageOrigin = {
      0.0f,
      22.0f,
      77.0f,
  };
  DataStructure dataGraph = UnitTest::CreateAllPrimitiveTypes(imageDims);

  // Get the DataGroups that we are going to add an Image Geometry into
  DataGroup* levelOneGroup = dataGraph.getDataAs<DataGroup>(DataPath({k_LevelZero, k_LevelOne}));
  REQUIRE(levelOneGroup != nullptr);
  DataGroup* levelTwoGroup = dataGraph.getDataAs<DataGroup>(DataPath({k_LevelZero, k_LevelTwo}));
  REQUIRE(levelTwoGroup != nullptr);

  // Add an ImageGeometry into each DataGroup
  UnitTest::AddImageGeometry(dataGraph, imageDims, imageSpacing, imageOrigin, *levelOneGroup);
  UnitTest::AddImageGeometry(dataGraph, imageDims, imageSpacing, imageOrigin, *levelTwoGroup);

  // Create a bool array where every value is TRUE
  std::vector<usize> tupleShape = {imageDims[2], imageDims[1], imageDims[0]};
  BoolArray* conditionalArray1 = UnitTest::CreateTestDataArray<bool>(dataGraph, k_ConditionalArray, tupleShape, {1}, levelOneGroup->getId());
  conditionalArray1->fill(true);
  BoolArray* conditionalArray2 = UnitTest::CreateTestDataArray<bool>(dataGraph, k_ConditionalArray, tupleShape, {1}, levelTwoGroup->getId());
  conditionalArray2->fill(true);

  DataPath conditionalDataPath({k_LevelZero, k_LevelOne, k_ConditionalArray});

  DataPath selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int8DataSet});
  ConditionalSetValueOverFlowTest<int8>(dataGraph, selectedDataPath, conditionalDataPath, "-130"); // underflow
  ConditionalSetValueOverFlowTest<int8>(dataGraph, selectedDataPath, conditionalDataPath, "130");  // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint8DataSet});
  ConditionalSetValueOverFlowTest<uint8>(dataGraph, selectedDataPath, conditionalDataPath, "-1");  // underflow
  ConditionalSetValueOverFlowTest<uint8>(dataGraph, selectedDataPath, conditionalDataPath, "260"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int16DataSet});
  ConditionalSetValueOverFlowTest<int16>(dataGraph, selectedDataPath, conditionalDataPath, "-32770"); // underflow
  ConditionalSetValueOverFlowTest<int16>(dataGraph, selectedDataPath, conditionalDataPath, "32770");  // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint16DataSet});
  ConditionalSetValueOverFlowTest<uint16>(dataGraph, selectedDataPath, conditionalDataPath, "-1");    // underflow
  ConditionalSetValueOverFlowTest<uint16>(dataGraph, selectedDataPath, conditionalDataPath, "65537"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int32DataSet});
  ConditionalSetValueOverFlowTest<int32>(dataGraph, selectedDataPath, conditionalDataPath, "-2147483649"); // underflow
  ConditionalSetValueOverFlowTest<int32>(dataGraph, selectedDataPath, conditionalDataPath, "2147483649");  // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint32DataSet});
  ConditionalSetValueOverFlowTest<uint32>(dataGraph, selectedDataPath, conditionalDataPath, "-1");         // underflow
  ConditionalSetValueOverFlowTest<uint32>(dataGraph, selectedDataPath, conditionalDataPath, "4294967297"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Int64DataSet});
  ConditionalSetValueOverFlowTest<int64>(dataGraph, selectedDataPath, conditionalDataPath, "-92233720368547758080"); // underflow
  ConditionalSetValueOverFlowTest<int64>(dataGraph, selectedDataPath, conditionalDataPath, "92233720368547758080");  // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Uint64DataSet});
  ConditionalSetValueOverFlowTest<uint64>(dataGraph, selectedDataPath, conditionalDataPath, "-1");                    // underflow
  ConditionalSetValueOverFlowTest<uint64>(dataGraph, selectedDataPath, conditionalDataPath, "184467440737095516150"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Float32DataSet});
  ConditionalSetValueOverFlowTest<float32>(dataGraph, selectedDataPath, conditionalDataPath, "1.17549e-039");  // underflow
  ConditionalSetValueOverFlowTest<float32>(dataGraph, selectedDataPath, conditionalDataPath, "3.40282e+039");  // overflow
  ConditionalSetValueOverFlowTest<float32>(dataGraph, selectedDataPath, conditionalDataPath, "-1.17549e-039"); // underflow
  ConditionalSetValueOverFlowTest<float32>(dataGraph, selectedDataPath, conditionalDataPath, "-3.40282e+039"); // overflow

  selectedDataPath = DataPath({k_LevelZero, k_LevelOne, k_Float64DataSet});
  ConditionalSetValueOverFlowTest<float64>(dataGraph, selectedDataPath, conditionalDataPath, "2.22507e-309");  // underflow
  ConditionalSetValueOverFlowTest<float64>(dataGraph, selectedDataPath, conditionalDataPath, "1.79769e+309");  // overflow
  ConditionalSetValueOverFlowTest<float64>(dataGraph, selectedDataPath, conditionalDataPath, "-2.22507e-309"); // underflow
  ConditionalSetValueOverFlowTest<float64>(dataGraph, selectedDataPath, conditionalDataPath, "-1.79769e+309"); // overflow
}
