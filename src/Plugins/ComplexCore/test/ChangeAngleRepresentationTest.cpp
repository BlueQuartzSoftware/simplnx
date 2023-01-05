#include <catch2/catch.hpp>

#include "ComplexCore/Filters/ChangeAngleRepresentation.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("ComplexCore::ChangeAngleRepresentation: Invalid Execution", "[OrientationAnalysis][ChangeAngleRepresentation]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ChangeAngleRepresentation filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  // This should fail
  args.insertOrAssign(ChangeAngleRepresentation::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ChangeAngleRepresentation::k_AnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // This should fail because parameter is out of range
  args.insertOrAssign(ChangeAngleRepresentation::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(2));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
}

TEST_CASE("ComplexCore::ChangeAngleRepresentation: Degrees To Radians")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ChangeAngleRepresentation filter;
  DataStructure dataStructure;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroup->getId());

  std::vector<size_t> tupleShape = {10};
  std::vector<size_t> componentShape = {3};

  Float32Array* angles = UnitTest::CreateTestDataArray<float>(dataStructure, Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      (*angles)[t * componentShape[0] + c] = static_cast<float>(t * c);
    }
  }

  // Create default Parameters for the filter.
  // This should fail
  args.insertOrAssign(ChangeAngleRepresentation::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ChangeAngleRepresentation::k_AnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  // Check the results
  float d2r = complex::numbers::pi / 180.0f;
  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      REQUIRE((*angles)[t * componentShape[0] + c] == static_cast<float>(t * c) * d2r);
    }
  }
}

TEST_CASE("ComplexCore::ChangeAngleRepresentation: Radians To Degrees")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ChangeAngleRepresentation filter;
  DataStructure dataStructure;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroup->getId());

  std::vector<size_t> tupleShape = {10};
  std::vector<size_t> componentShape = {3};

  Float32Array* angles = UnitTest::CreateTestDataArray<float>(dataStructure, Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      (*angles)[t * componentShape[0] + c] = static_cast<float>(t * c);
    }
  }

  // Create default Parameters for the filter.
  // This should fail
  args.insertOrAssign(ChangeAngleRepresentation::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(ChangeAngleRepresentation::k_AnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  // Check the results
  float r2d = 180.0f / complex::numbers::pi;
  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      REQUIRE((*angles)[t * componentShape[0] + c] == static_cast<float>(t * c) * r2d);
    }
  }
}
