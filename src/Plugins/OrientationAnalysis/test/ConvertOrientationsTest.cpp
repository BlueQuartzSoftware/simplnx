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
 * When you start working on this unit test remove "[ConvertOrientationsFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */
#include "OrientationAnalysis/Filters/Algorithms/ConvertOrientations.hpp"
#include "OrientationAnalysis/Filters/ConvertOrientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <EbsdLib/Core/EbsdDataArray.hpp>
#include <EbsdLib/OrientationMath/OrientationConverter.hpp>

#include <array>
#include <catch2/catch.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
// Component counts per representation, indexed by ebsdlib::orientations::Type
// (Euler, OrientationMatrix, Quaternion, AxisAngle, Rodrigues, Homochoric, Cubochoric, Stereographic).
constexpr std::array<size_t, 8> k_Comps = {3, 9, 4, 4, 4, 3, 3, 3};
const std::vector<std::string> k_RepNames = {"Euler", "OrientationMatrix", "Quaternion", "AxisAngle", "Rodrigues", "Homochoric", "Cubochoric", "Stereographic"};

// V&V dispatch landmarks: 3 distinct general orientations, each expressed in all 8 representations.
// Generated directly from the EbsdLib 3.0.0 float Orientation classes (the reference implementation
// the filter links), independent of the filter's parallel-convertor plumbing — see
// src/Plugins/OrientationAnalysis/vv/ConvertOrientationsFilter.md. These are NOT a math oracle (the
// transform equations are verified by EbsdLib's own Orientation*Test.cpp suite); they are landmarks
// that prove the filter's (inputType,outputType) switch routes to the correct conversion and strides
// components correctly. Seed 0 == EbsdLib OrientationConverterTest published orientation
// (302.84, 51.282, 37.969 deg): its quaternion matches that test's exemplar, and its stereographic
// matches the closed form st = (qx,qy,qz)/(1+qw). Layouts: eu[phi1,Phi,phi2] rad; om row-major 3x3;
// qu[x,y,z,w]; ax[x,y,z,angle rad]; ro[x,y,z,tan(angle/2)]; ho[x,y,z]; cu[x,y,z]; st[x,y,z].
const std::vector<std::vector<std::vector<float>>> k_Ref = {
    // seed 0 — (302.84, 51.282, 37.969) deg
    {{5.28555489F, 0.895039737F, 0.662684023F},
     {0.750837624F, -0.453670323F, 0.480027199F, 0.0806576908F, 0.784318447F, 0.615092576F, -0.655543447F, -0.423116744F, 0.625487804F},
     {-0.291989446F, 0.31937167F, 0.150276214F, 0.888909996F},
     {-0.637417495F, 0.697193384F, 0.328055322F, 0.951672375F},
     {-0.637417495F, 0.697193384F, 0.328055322F, 0.515329897F},
     {-0.298757643F, 0.326774597F, 0.153759554F},
     {-0.358878195F, 0.377770394F, 0.199860498F},
     {-0.154580921F, 0.169077232F, 0.0795571059F}},
    // seed 1 — (45, 30, 60) deg
    {{0.785398185F, 0.52359879F, 1.04719758F},
     {-0.176776767F, 0.883883476F, 0.433012724F, -0.918558598F, -0.306186289F, 0.249999985F, 0.353553385F, -0.353553385F, 0.866025388F},
     {-0.25660482F, 0.0337826647F, -0.766320527F, 0.588018298F},
     {-0.317247421F, 0.0417664163F, -0.947422624F, 1.88437927F},
     {-0.317247421F, 0.0417664163F, -0.947422624F, 1.37554824F},
     {-0.281666279F, 0.0370820723F, -0.841163695F},
     {-0.301626205F, 0.0443257056F, -0.715598881F},
     {-0.161588073F, 0.0212734733F, -0.482564032F}},
    // seed 2 — (123.4, 88.7, 271.2) deg
    {{2.15373635F, 1.54810703F, 4.73333311F},
     {0.00740783196F, 0.0299700946F, -0.999523342F, -0.550756693F, 0.834403157F, 0.0209372081F, 0.834632933F, 0.550339103F, 0.0226873513F},
     {0.193853885F, -0.671622694F, -0.212647885F, 0.682733178F},
     {0.265310556F, -0.919190168F, -0.291032225F, 1.63859916F},
     {0.265310556F, -0.919190168F, -0.291032225F, 1.07020998F},
     {0.207828149F, -0.720037639F, -0.227976933F},
     {0.261202067F, -0.63136822F, -0.281791151F},
     {0.115201794F, -0.399126083F, -0.126370534F}}};
} // namespace

// This section of code exists solely to generate a source code in case another
// orientation representation is created. Leave this code here.
void _make_code()
{
  std::vector<std::string> inRep = {"eu", "om", "qu", "ax", "ro", "ho", "cu", "st"};
  std::vector<std::string> outRep = {"eu", "om", "qu", "ax", "ro", "ho", "cu", "st"};
  std::vector<std::string> names = {"Euler", "OrientationMatrix", "Quaternion", "AxisAngle", "Rodrigues", "Homochoric", "Cubochoric", "Stereographic"};

  std::vector<int> strides = {3, 9, 4, 4, 4, 3, 3, 3};

  for(size_t i = 0; i < 8; i++)
  {
    for(size_t o = 0; o < 8; o++)
    {
      if(inRep[i] == outRep[o])
      {
        continue;
      }

      std::cout << "else if( inputType == ebsdlib::orientations::Type::" << names[i] << " && outputType == ebsdlib::orientations::Type::" << names[o] << ")\n"
                << "{\n";
      std::cout << "  messageHandler.sendInfoMessage(\"Converting " << names[i] << " to " << names[o] << "\");\n";

      if(inRep[i] == "qu")
      {
        std::cout << "  FromQuaternionFunctionType " << inRep[i] << "2" << outRep[o] << " = OrientationTransformation::" << inRep[i] << "2" << outRep[o] << "<QuaternionType, OutputType>;\n";
        //   std::cout << "  ValidateInputDataFunctionType " << inRep[i] << "Check = " << names[i] << "Check<float>();" << std::endl;
        std::cout << "  parallelAlgorithm.execute(::FromQuaternion<float, FromQuaternionFunctionType, ValidateInputDataFunctionType, " << strides[i] << ", " << strides[o]
                  << ">(inputDataArray, outputDataArray, " << inRep[i] << "2" << outRep[o] << "," << inRep[i] << "Check, QuaternionType::Order::VectorScalar));\n";
      }
      else if(outRep[o] == "qu")
      {
        std::cout << "  ToQuaternionFunctionType " << inRep[i] << "2" << outRep[o] << " = OrientationTransformation::" << inRep[i] << "2" << outRep[o] << "<InputType, QuaternionType>;\n";
        //    std::cout << "  ValidateInputDataFunctionType " << inRep[i] << "Check = " << names[i] << "Check<float>();" << std::endl;
        std::cout << "  parallelAlgorithm.execute(::ToQuaternion<float, ToQuaternionFunctionType, ValidateInputDataFunctionType, " << strides[i] << ", " << strides[o]
                  << ">(inputDataArray, outputDataArray, " << inRep[i] << "2" << outRep[o] << ", " << inRep[i] << "Check, QuaternionType::Order::VectorScalar));\n";
      }
      else
      {
        std::cout << "  ConversionFunctionType " << inRep[i] << "2" << outRep[o] << " = OrientationTransformation::" << inRep[i] << "2" << outRep[o] << "<InputType, OutputType>;\n";
        //   std::cout << "  ValidateInputDataFunctionType " << inRep[i] << "Check = " << names[i] << "Check<float>();" << std::endl;
        std::cout << "  parallelAlgorithm.execute(::ConvertOrientation<float, ConversionFunctionType, ValidateInputDataFunctionType, " << strides[i] << ", " << strides[o]
                  << ">(inputDataArray, outputDataArray, " << inRep[i] << "2" << outRep[o] << ", " << inRep[i] << "Check));\n";
        //        std::cout << "  dataAlg.execute(::ConvertOrientationImpl<float, OrientationType, " << strides[i] << ", OrientationType, " << strides[o]
        //                  << ", std::function<OutputType(InputType)>>(inputDataArray, outputDataArray, " << inRep[i] << "2" << outRep[o] << "));\n";
      }
      std::cout << "}\n";
    };
  }
}

TEST_CASE("OrientationAnalysis::ConvertOrientations: Invalid preflight", "[OrientationAnalysis][ConvertOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  //------------------------------
  // This code is commented out because it generates a bunch of code. See the comment
  // at the top of the _make_code function. This should stay in here in case it is
  // needed later on. I don't want to rewrite the code.
  //_make_code();
  //----------------------------

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ConvertOrientationsFilter filter;
  DataStructure dataStructure;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, nx::core::Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, nx::core::Constants::k_EbsdScanData, topLevelGroup->getId());

  std::vector<size_t> tupleShape = {10};
  std::vector<size_t> componentShape = {3};

  args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(ConvertOrientationsFilter::k_InputOrientationArrayPath_Key,
                      std::make_any<DataPath>(DataPath({nx::core::Constants::k_SmallIN100, nx::core::Constants::k_EbsdScanData, nx::core::Constants::k_EulerAngles})));
  args.insertOrAssign(ConvertOrientationsFilter::k_OutputOrientationArrayName_Key, std::make_any<std::string>(nx::core::Constants::k_AxisAngles));
  // Create default Parameters for the filter.
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    const std::vector<Error>& errors = preflightResult.outputActions.errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].code == nx::core::FilterParameter::Constants::k_Validate_Does_Not_Exist);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  Float32Array* angles = UnitTest::CreateTestDataArray<float>(dataStructure, nx::core::Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

  // Create default Parameters for the filter.
  {
    args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(8));
    args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(1));
    auto preflightResult = filter.preflight(dataStructure, args);
    const std::vector<Error>& errors = preflightResult.outputActions.errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].code == nx::core::FilterParameter::Constants::k_Validate_OutOfRange_Error);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  {
    args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(1));
    args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(8));
    auto preflightResult = filter.preflight(dataStructure, args);
    auto& errors = preflightResult.outputActions.errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].code == nx::core::FilterParameter::Constants::k_Validate_OutOfRange_Error);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  // Input component count does not match the selected input representation type: the 3-component
  // array above is declared as a Quaternion (which requires 4 components) -> -67004.
  {
    args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(2));  // Quaternion (expects 4 components)
    args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(0)); // Euler
    auto preflightResult = filter.preflight(dataStructure, args);
    const std::vector<Error>& errors = preflightResult.outputActions.errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].code == convert_orientations_constants::k_InputComponentCountError);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Verifies the filter's value-add (NOT the EbsdLib transform math, which EbsdLib's own
 * Orientation*Test.cpp suite verifies): that the (inputType, outputType) switch dispatches to the
 * correct conversion and that components are read/written with the correct per-tuple stride.
 *
 * For every (in, out) representation pair (full 8x8 minus the same-type diagonal, including
 * Stereographic) the filter must transform R[t][in] into R[t][out] within tolerance, where R is a
 * set of 3 distinct general orientations each expressed in all 8 representations (k_Ref, generated
 * from EbsdLib 3.0.0 — see vv/ConvertOrientationsFilter.md). Wired to the wrong conversion the
 * output would be a detectably different number; the 3 distinct tuples additionally pin the striding
 * (identical tuples would not catch an offset bug). Class 3 (Rowenhorst 2015) dispatch landmarks
 * plus Class 4 (round-trip consistency) — tolerance 1.0e-4 (tightened during V&V; the observed float32 dispatch error is well under this).
 */
TEST_CASE("OrientationAnalysis::ConvertOrientations: Dispatch and striding (8x8 matrix)", "[ConvertOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  const size_t numTuples = k_Ref.size(); // 3 distinct orientations

  for(size_t in = 0; in < 8; in++)
  {
    for(size_t out = 0; out < 8; out++)
    {
      if(in == out)
      {
        continue; // same-type is rejected at preflight (see "Equal Representations")
      }

      DYNAMIC_SECTION(k_RepNames[in] << " -> " << k_RepNames[out])
      {
        ConvertOrientationsFilter filter;
        DataStructure dataStructure;
        Arguments args;

        DataGroup* topLevelGroup = DataGroup::Create(dataStructure, nx::core::Constants::k_SmallIN100);
        DataGroup* scanData = DataGroup::Create(dataStructure, nx::core::Constants::k_EbsdScanData, topLevelGroup->getId());

        std::vector<size_t> tupleShape = {numTuples};
        std::vector<size_t> componentShape = {k_Comps[in]};
        Float32Array* input = UnitTest::CreateTestDataArray<float>(dataStructure, nx::core::Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

        for(size_t t = 0; t < numTuples; t++)
        {
          for(size_t c = 0; c < k_Comps[in]; c++)
          {
            (*input)[t * k_Comps[in] + c] = k_Ref[t][in][c];
          }
        }

        args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(in));
        args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(out));
        args.insertOrAssign(ConvertOrientationsFilter::k_InputOrientationArrayPath_Key,
                            std::make_any<DataPath>(DataPath({nx::core::Constants::k_SmallIN100, nx::core::Constants::k_EbsdScanData, nx::core::Constants::k_EulerAngles})));
        args.insertOrAssign(ConvertOrientationsFilter::k_OutputOrientationArrayName_Key, std::make_any<std::string>(nx::core::Constants::k_AxisAngles));

        auto preflightResult = filter.preflight(dataStructure, args);
        SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

        auto executeResult = filter.execute(dataStructure, args);
        SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

        Float32Array& output = dataStructure.getDataRefAs<Float32Array>(DataPath({nx::core::Constants::k_SmallIN100, nx::core::Constants::k_EbsdScanData, nx::core::Constants::k_AxisAngles}));

        // Striding: output array must have the output type's component count over all tuples.
        REQUIRE(output.getNumberOfComponents() == k_Comps[out]);
        REQUIRE(output.getNumberOfTuples() == numTuples);

        // Dispatch landmark: each tuple's output matches the expected representation of that orientation.
        for(size_t t = 0; t < numTuples; t++)
        {
          for(size_t c = 0; c < k_Comps[out]; c++)
          {
            INFO("tuple " << t << " component " << c);
            float absDif = std::fabs(output[t * k_Comps[out] + c] - k_Ref[t][out][c]);
            REQUIRE(absDif < 1.0e-4F);
          }
        }

        UnitTest::CheckArraysInheritTupleDims(dataStructure);
      }
    }
  }
}

/**
 * @brief Class 1 (Analytical) check for the Stereographic representation, which has no legacy
 * equivalent. The stereographic projection of a unit quaternion [x,y,z,w] is the closed form
 * st = (x, y, z) / (1 + w). Expected values are computed in-test from that formula (no EbsdLib call),
 * so this independently pins the Quaternion -> Stereographic conversion.
 */
TEST_CASE("OrientationAnalysis::ConvertOrientations: Stereographic closed form (Class 1)", "[ConvertOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  const size_t numTuples = k_Ref.size();
  constexpr size_t k_Qu = 2;
  constexpr size_t k_St = 7;

  ConvertOrientationsFilter filter;
  DataStructure dataStructure;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, nx::core::Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, nx::core::Constants::k_EbsdScanData, topLevelGroup->getId());

  std::vector<size_t> tupleShape = {numTuples};
  std::vector<size_t> componentShape = {k_Comps[k_Qu]};
  Float32Array* input = UnitTest::CreateTestDataArray<float>(dataStructure, nx::core::Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

  for(size_t t = 0; t < numTuples; t++)
  {
    for(size_t c = 0; c < k_Comps[k_Qu]; c++)
    {
      (*input)[t * k_Comps[k_Qu] + c] = k_Ref[t][k_Qu][c];
    }
  }

  args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(k_Qu));
  args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(k_St));
  args.insertOrAssign(ConvertOrientationsFilter::k_InputOrientationArrayPath_Key,
                      std::make_any<DataPath>(DataPath({nx::core::Constants::k_SmallIN100, nx::core::Constants::k_EbsdScanData, nx::core::Constants::k_EulerAngles})));
  args.insertOrAssign(ConvertOrientationsFilter::k_OutputOrientationArrayName_Key, std::make_any<std::string>(nx::core::Constants::k_AxisAngles));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  Float32Array& output = dataStructure.getDataRefAs<Float32Array>(DataPath({nx::core::Constants::k_SmallIN100, nx::core::Constants::k_EbsdScanData, nx::core::Constants::k_AxisAngles}));

  for(size_t t = 0; t < numTuples; t++)
  {
    const std::vector<float>& qu = k_Ref[t][k_Qu]; // [x, y, z, w]
    const float denom = 1.0F + qu[3];
    const float expected[3] = {qu[0] / denom, qu[1] / denom, qu[2] / denom};
    for(size_t c = 0; c < 3; c++)
    {
      INFO("tuple " << t << " component " << c);
      REQUIRE(std::fabs(output[t * 3 + c] - expected[c]) < 1.0e-5F);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ConvertOrientations: Equal Representations", "[ConvertOrientationsFilter]")
{
  typedef ebsdlib::OrientationConverter<EbsdDataArray<float32>, float32> OCType;
  const std::vector<ebsdlib::orientations::Type> ocTypes = OCType::GetOrientationTypes();

  ebsdlib::orientations::Type ocType = GENERATE_COPY(from_range(OCType::GetOrientationTypes()));

  ConvertOrientationsFilter filter;
  DataStructure dataStructure;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, nx::core::Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, nx::core::Constants::k_EbsdScanData, topLevelGroup->getId());

  std::vector<usize> tupleShape = {12};
  std::vector<usize> componentShape = {4};
  Float32Array* angles = UnitTest::CreateTestDataArray<float>(dataStructure, nx::core::Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      (*angles)[t * componentShape[0] + c] = 0;
    }
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ocType)));
  args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ocType)));
  args.insertOrAssign(ConvertOrientationsFilter::k_InputOrientationArrayPath_Key,
                      std::make_any<DataPath>(DataPath({nx::core::Constants::k_SmallIN100, nx::core::Constants::k_EbsdScanData, nx::core::Constants::k_EulerAngles})));
  args.insertOrAssign(ConvertOrientationsFilter::k_OutputOrientationArrayName_Key, std::make_any<std::string>(nx::core::Constants::k_AxisAngles));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == convert_orientations_constants::k_MatchingTypesError);
}

TEST_CASE("OrientationAnalysis::ConvertOrientationsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ConvertOrientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ConvertOrientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ConvertOrientationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ConvertOrientationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(ConvertOrientationsFilter::k_InputType_Key) == 0);
      CHECK(args.value<ChoicesParameter::ValueType>(ConvertOrientationsFilter::k_OutputType_Key) == 0);
      CHECK(args.value<DataPath>(ConvertOrientationsFilter::k_InputOrientationArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ConvertOrientationsFilter::k_OutputOrientationArrayName_Key) == "TestName");
    }
  }
}
