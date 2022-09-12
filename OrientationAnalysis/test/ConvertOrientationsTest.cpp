/**
 * This file is auto generated from the original OrientationAnalysis/ConvertOrientations
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[ConvertOrientations][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Common/Numbers.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ConvertOrientations.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

void _make_code()
{
  std::vector<std::string> inRep = {"eu", "om", "qu", "ax", "ro", "ho", "cu"};
  std::vector<std::string> outRep = {"eu", "om", "qu", "ax", "ro", "ho", "cu"};
  std::vector<std::string> names = {"Euler", "OrientationMatrix", "Quaternion", "AxisAngle", "Rodrigues", "Homochoric", "Cubochoric"};

  std::vector<int> strides = {3, 9, 4, 4, 4, 3, 3};

  for(size_t i = 0; i < 7; i++)
  {
    for(size_t o = 0; o < 7; o++)
    {
      if(inRep[i] == outRep[o])
      {
        continue;
      }

      std::cout << "else if( inputType == OrientationRepresentation::Type::" << names[i] << " && outputType == OrientationRepresentation::Type::" << names[o] << ")\n"
                << "{\n";
      std::cout << "  messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, \"Converting " << names[i] << " to " << names[o] << "\"});\n";

      if(inRep[i] == "qu")
      {
        std::cout << "  FromQuaternionFunctionType " << inRep[i] << "2" << outRep[o] << " = OrientationTransformation::" << inRep[i] << "2" << outRep[o] << "<QuaternionType, OutputType>;\n";
        std::cout << "  parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, " << strides[i] << ", " << strides[o] << ">(inputDataArray, outputDataArray, " << inRep[i] << "2"
                  << outRep[o] << ", QuaternionType::Order::VectorScalar));\n";
      }
      else if(outRep[o] == "qu")
      {
        std::cout << "  ToQuaternionFunctionType " << inRep[i] << "2" << outRep[o] << " = OrientationTransformation::" << inRep[i] << "2" << outRep[o] << "<InputType, QuaternionType>;\n";
        std::cout << "  parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, " << strides[i] << ", " << strides[o] << ">(inputDataArray, outputDataArray, " << inRep[i] << "2"
                  << outRep[o] << ", QuaternionType::Order::VectorScalar));\n";
      }
      else
      {
        std::cout << "  ConversionFunctionType " << inRep[i] << "2" << outRep[o] << " = OrientationTransformation::" << inRep[i] << "2" << outRep[o] << "<InputType, OutputType>;\n";
        std::cout << "  parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, " << strides[i] << ", " << strides[o] << ">(inputDataArray, outputDataArray, " << inRep[i] << "2"
                  << outRep[o] << "));\n";
        //        std::cout << "  dataAlg.execute(::ConvertOrientationImpl<float, OrientationType, " << strides[i] << ", OrientationType, " << strides[o]
        //                  << ", std::function<OutputType(InputType)>>(inputDataArray, outputDataArray, " << inRep[i] << "2" << outRep[o] << "));\n";
      }
      std::cout << "}\n";
    };
  }
}

TEST_CASE("OrientationAnalysis::ConvertOrientations: Invalid preflight", "[OrientationAnalysis][ConvertOrientations]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ConvertOrientations filter;
  DataStructure dataGraph;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataGraph, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataGraph, Constants::k_EbsdScanData, topLevelGroup->getId());

  std::vector<size_t> tupleShape = {10};
  std::vector<size_t> componentShape = {3};

  args.insertOrAssign(ConvertOrientations::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ConvertOrientations::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(ConvertOrientations::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));
  args.insertOrAssign(ConvertOrientations::k_OutputOrientationArrayName_Key, std::make_any<std::string>(Constants::k_AxisAngles));
  // Create default Parameters for the filter.
  {
    auto preflightResult = filter.preflight(dataGraph, args);
    const std::vector<Error>& errors = preflightResult.outputActions.errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].code == complex::FilterParameter::Constants::k_Validate_Does_Not_Exist);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  Float32Array* angles = UnitTest::CreateTestDataArray<float>(dataGraph, Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

  // Create default Parameters for the filter.
  {
    args.insertOrAssign(ConvertOrientations::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(8));
    args.insertOrAssign(ConvertOrientations::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(1));
    auto preflightResult = filter.preflight(dataGraph, args);
    const std::vector<Error>& errors = preflightResult.outputActions.errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].code == complex::FilterParameter::Constants::k_Validate_OutOfRange_Error);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  {
    args.insertOrAssign(ConvertOrientations::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(1));
    args.insertOrAssign(ConvertOrientations::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(8));
    auto preflightResult = filter.preflight(dataGraph, args);
    auto& errors = preflightResult.outputActions.errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].code == complex::FilterParameter::Constants::k_Validate_OutOfRange_Error);
    REQUIRE(!preflightResult.outputActions.valid());
  }
}

TEST_CASE("OrientationAnalysis::ConvertOrientations: Instantiation and Parameter Check", "[OrientationAnalysis][ConvertOrientations]")
{

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ConvertOrientations filter;
  DataStructure dataGraph;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataGraph, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataGraph, Constants::k_EbsdScanData, topLevelGroup->getId());

  std::vector<size_t> tupleShape = {10};
  std::vector<size_t> componentShape = {3};

  Float32Array* angles = UnitTest::CreateTestDataArray<float>(dataGraph, Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());
  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      (*angles)[t * componentShape[0] + c] = static_cast<float>(t * c);
    }
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(ConvertOrientations::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ConvertOrientations::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(ConvertOrientations::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));
  args.insertOrAssign(ConvertOrientations::k_OutputOrientationArrayName_Key, std::make_any<std::string>(Constants::k_AxisAngles));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}

/**
 * @brief TEST_CASE This test case will execute all the combinations of the ConvertOrientations filter. This test only
 * tests the execution of the filter and not the final output.
 */
TEST_CASE("OrientationAnalysis::ConvertOrientations: Valid filter execution")
{

  std::vector<std::string> inRep = {"eu", "om", "qu", "ax", "ro", "ho", "cu"};
  std::vector<std::string> outRep = {"eu", "om", "qu", "ax", "ro", "ho", "cu"};
  std::vector<std::string> names = {"Euler", "OrientationMatrix", "Quaternion", "AxisAngle", "Rodrigues", "Homochoric", "Cubochoric"};

  std::vector<size_t> strides = {3, 9, 4, 4, 4, 3, 3};
  // clang-format off
  const std::vector<std::vector<float>> k_InitValues = { {4.76687F, 1.39683F, 2.46356F}, // Euler
                                                {0.0660025F, 0.783565F, 0.617794F, -0.168761F, 0.618991F, -0.767053F, -0.983445F, -0.0536321F, 0.17309F}, // OM
                                                {0.261688F, 0.587345F, -0.34932F, 0.681558F}, // QU
                                                {0.357612F, 0.802643F, -0.477366F, 1.64181F },// AX
                                                {0.357612F, 0.802643F, -0.477366F, 1.07366F}, // RO
                                                {0.280631F, 0.629864F, -0.374607F}, // HO
                                                {0.359479F, 0.632495F, -0.458758F} // CU;
    };
  // clang-format on

  for(size_t i = 0; i < 7; i++)
  {
    std::vector<size_t> tupleShape = {1};
    std::vector<size_t> componentShape = {strides[i]};
    std::vector<float> initValues = k_InitValues[i];

    for(size_t o = 0; o < 7; o++)
    {
      if(inRep[i] == outRep[o])
      {
        continue;
      }
      std::vector<float> outputValues = k_InitValues[o];

      // Instantiate the filter, a DataStructure object and an Arguments Object
      ConvertOrientations filter;
      DataStructure dataGraph;
      Arguments args;

      DataGroup* topLevelGroup = DataGroup::Create(dataGraph, Constants::k_SmallIN100);
      DataGroup* scanData = DataGroup::Create(dataGraph, Constants::k_EbsdScanData, topLevelGroup->getId());

      Float32Array* angles = UnitTest::CreateTestDataArray<float>(dataGraph, Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

      for(size_t t = 0; t < tupleShape[0]; t++)
      {
        for(size_t c = 0; c < componentShape[0]; c++)
        {
          (*angles)[t * componentShape[0] + c] = initValues[c];
        }
      }

      // Create default Parameters for the filter.
      args.insertOrAssign(ConvertOrientations::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(i));
      args.insertOrAssign(ConvertOrientations::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(o));
      args.insertOrAssign(ConvertOrientations::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));
      args.insertOrAssign(ConvertOrientations::k_OutputOrientationArrayName_Key, std::make_any<std::string>(Constants::k_AxisAngles));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataGraph, args);
      REQUIRE(preflightResult.outputActions.valid());

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataGraph, args);
      REQUIRE(executeResult.result.valid());

      Float32Array& output = dataGraph.getDataRefAs<Float32Array>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_AxisAngles}));
      for(size_t t = 0; t < tupleShape[0]; t++)
      {
        for(size_t c = 0; c < strides[o]; c++)
        {
          // std::cout << outputValues[c] << "F, ";
          float absDif = std::fabs(output[t * strides[o] + c] - outputValues[c]);
          REQUIRE(absDif < 0.0001);
        }
      }
    }
  }
}
