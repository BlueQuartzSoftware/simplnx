

#include <catch2/catch.hpp>

#include "UnitTestCommon.hpp"

#include "ComplexCore/Filters/ConditionalSetValue.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include <iostream>

using namespace complex;
using namespace ComplexUnitTest::Constants;

TEST_CASE("Instantiate ConditionalSetValue Filter", "[ConditionalSetValue]")
{
  ConditionalSetValue filter;
  DataStructure dataGraph;
  Arguments args;

  DataPath ciDataPath = DataPath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  args.insert(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<double>(0.0));
  args.insert(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, "ConditionalArray"})));
  args.insert(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == false);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid() == false);
}

TEST_CASE("Test ConditionalSetValue Algorithm", "[ConditionalSetValue]")
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
  BoolArray* conditionalArray = ComplexUnitTest::CreateTestDataArray<bool>("ConditionalArray", dataGraph, tupleShape, {1}, dataGraph.getId(ebsdScanPath).value());
  conditionalArray->fill(true);

  ConditionalSetValue filter;
  Arguments args;

  args.insert(ConditionalSetValue::k_ReplaceValue_Key, std::make_any<double>(0.0));
  args.insert(ConditionalSetValue::k_ConditionalArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, "ConditionalArray"})));
  args.insert(ConditionalSetValue::k_SelectedArrayPath_Key, std::make_any<DataPath>(ciDataPath));

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
