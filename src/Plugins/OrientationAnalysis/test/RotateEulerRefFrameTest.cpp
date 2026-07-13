#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"

#include <array>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using namespace nx::core;

namespace AnalyticalFixtures
{
// Class 1 (Analytical) oracle fixtures for RotateEulerRefFrameFilter.
//
// The filter rotates the sample reference frame by angle w (degrees, right-hand rule)
// about sample-frame axis n. Derivation (independent of EbsdLib/SIMPLNX/legacy code):
//   - Bunge passive convention: v_crystal = g . v_sample
//   - Rotating the reference frame by +w about unit axis n gives new-frame coordinates
//     u_new = P(w) . u_old with P(w) the passive rotation matrix, so
//     g' = g . P(w)^T = g . R_active(n, w)   (R_active = Rodrigues rotation matrix)
//   - Euler <-> matrix conversions per Rowenhorst et al 2015, Modelling Simul. Mater.
//     Sci. Eng. 23 083501, Eq. A.5 (eu2om) and Eq. A.9 (om2eu), with outputs
//     canonicalized to phi1, phi2 in [0, 2pi), Phi in [0, pi].
//
// Closed forms used below (all verified by the independent numpy script archived in
// vv/provenance — see the V&V report):
//   - Z-axis rotation: phi1' = phi1 - w (mod 2pi), Phi and phi2 unchanged.
//   - Identity orientation + axis-angle (n, w): g' = R_active(n, w), om2eu by hand.
struct FixtureData
{
  std::string name;
  std::array<float32, 3> eulerIn;   // radians
  std::array<float32, 4> axisAngle; // i, j, k, w (degrees)
  std::array<float64, 3> expected;  // radians, canonical ranges
};

const std::vector<FixtureData> k_Fixtures = {
    // F1: g = I, rotate frame +90 about z. phi1' = 0 - pi/2 mod 2pi = 3pi/2.
    {"F1 identity + z90", {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F, 90.0F}, {4.712388980384709, 0.0, 0.0}},
    // F2: general orientation, z-axis: phi1' = 1.0 - pi/6; Phi, phi2 unchanged.
    {"F2 general + z30", {1.0F, 0.5F, 0.3F}, {0.0F, 0.0F, 1.0F, 30.0F}, {0.476401224401701, 0.5, 0.3}},
    // F3: wrap-around: phi1' = 0.1 - pi/2 + 2pi.
    {"F3 wraparound + z90", {0.1F, 0.5F, 0.3F}, {0.0F, 0.0F, 1.0F, 90.0F}, {4.812388980384732, 0.5, 0.3}},
    // F4: g = I, +90 about x: g' = R_active(x, 90) => om2eu = (pi, pi/2, pi) by hand.
    {"F4 identity + x90", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F, 90.0F}, {3.141592653589793, 1.570796326794897, 3.141592653589793}},
    // F5: axis (2,0,0) must normalize to (1,0,0); expected identical to F4.
    {"F5 unnormalized axis x90", {0.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F, 90.0F}, {3.141592653589793, 1.570796326794897, 3.141592653589793}},
    // F6: w = 0 is a no-op for already-canonical inputs.
    {"F6 zero angle no-op", {5.0F, 1.0F, 2.0F}, {0.0F, 0.0F, 1.0F, 0.0F}, {5.0, 1.0, 2.0}},
    // F7: g = I, +120 about (1,1,1): R_active maps x->y->z->x; om2eu = (pi, pi/2, pi/2) by hand.
    {"F7 identity + 111 120", {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F, 120.0F}, {3.141592653589793, 1.570796326794896, 1.570796326794897}},
    // F8: general orientation, y-axis 45 deg. No closed form; value from the numpy oracle script.
    {"F8 general + y45", {0.8F, 1.1F, 0.4F}, {0.0F, 1.0F, 0.0F, 45.0F}, {0.208427151998385, 0.687549596714496, 1.288701818256954}},
};

// Creates a DataStructure holding a cell AttributeMatrix with one float32 3-component
// Euler angles array of numTuples tuples, returning the array pointer.
inline Float32Array* CreateEulerArray(DataStructure& dataStructure, usize numTuples)
{
  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{numTuples});
  return UnitTest::CreateTestDataArray<float32>(dataStructure, "EulerAngles", {numTuples}, {3}, cellData->getId());
}

// Runs RotateEulerRefFrameFilter in place on the given euler array.
inline void RunRotateFilter(DataStructure& dataStructure, const DataPath& eulerPath, const std::array<float32, 4>& axisAngle)
{
  RotateEulerRefFrameFilter filter;
  Arguments args;
  args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxisAngle_Key,
                      std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{axisAngle[0], axisAngle[1], axisAngle[2], axisAngle[3]}));
  args.insertOrAssign(RotateEulerRefFrameFilter::k_EulerAnglesArrayPath_Key, std::make_any<DataPath>(eulerPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace AnalyticalFixtures

TEST_CASE("OrientationAnalysis::RotateEulerRefFrame", "[OrientationAnalysis][RotateEulerRefFrameFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ASCIIData.tar.gz", "ASCIIData");

  // Instantiate an "Application" instance to load plugins
  UnitTest::LoadPlugins();

  const uint64 k_NumComponents = 3;
  const static DynamicTableInfo::TableDataType k_NumTuples = {{static_cast<double>(480000)}};
  const nx::core::NumericType k_NumericType = nx::core::NumericType::float32;

  // Constant strings and DataPaths to be used later
  const DataPath k_EulerAnglesDataPath({Constants::k_EulerAngles});

  const std::string k_EulersRotated("EulersRotated");
  const DataPath k_EulersRotatedDataPath({k_EulersRotated});

  std::string inputFile = fmt::format("{}/ASCIIData/EulerAngles.csv", unit_test::k_TestFilesDir.view());
  std::string comparisonDataFile = fmt::format("{}/ASCIIData/EulersRotated.csv", unit_test::k_TestFilesDir.view());

  // Make sure we can load the "Import Text Filter" filter from the plugin
  auto* filterList = Application::Instance()->getFilterList();
  // Make sure we can instantiate the Import Text Filter

  DataStructure dataStructure;

  // Run the "Import Text" Filter to import the data for the EulerAngles and EulersRotated
  {
    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(k_NumericType));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(k_NumTuples));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(k_NumComponents));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_EulerAnglesDataPath));

    auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(comparisonDataFile)));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(k_NumericType));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(k_NumTuples));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(k_NumComponents));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_EulersRotatedDataPath));

    auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Run the RotateEulerRefFrameFilter
  {
    RotateEulerRefFrameFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxisAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0F, 1.0F, 1.0F, 30.0F}));
    args.insertOrAssign(RotateEulerRefFrameFilter::k_EulerAnglesArrayPath_Key, std::make_any<DataPath>(k_EulerAnglesDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the 2 data sets
  {
    const auto& eulerAngles = dataStructure.getDataRefAs<Float32Array>(k_EulerAnglesDataPath);
    const auto& eulersRotated = dataStructure.getDataRefAs<Float32Array>(k_EulersRotatedDataPath);

    size_t numElements = eulerAngles.getSize();
    for(size_t i = 0; i < numElements; i++)
    {
      float absDif = std::fabs(eulerAngles[i] - eulersRotated[i]);
      bool sameValue = (absDif < 0.0001);
      if(!sameValue)
      {
        REQUIRE(absDif < 0.0001);
      }
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::RotateEulerRefFrameFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][RotateEulerRefFrameFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RotateEulerRefFrameFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RotateEulerRefFrameFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RotateEulerRefFrameFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (FloatVec3p1FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(RotateEulerRefFrameFilter::k_EulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

TEST_CASE("OrientationAnalysis::RotateEulerRefFrameFilter: Class 1 Analytical Fixtures", "[OrientationAnalysis][RotateEulerRefFrameFilter]")
{
  UnitTest::LoadPlugins();

  // Single-pass double-precision math on float32 storage: float32 rounding of the decimal
  // inputs perturbs the output by O(1e-7); 1e-5 rad is a comfortable-but-tight bound.
  constexpr float64 k_Tolerance = 1.0e-5;

  const DataPath k_EulerPath({"CellData", "EulerAngles"});

  for(const auto& fixture : AnalyticalFixtures::k_Fixtures)
  {
    DYNAMIC_SECTION(fixture.name)
    {
      DataStructure dataStructure;
      Float32Array* eulerArray = AnalyticalFixtures::CreateEulerArray(dataStructure, 1);
      for(usize c = 0; c < 3; c++)
      {
        (*eulerArray)[c] = fixture.eulerIn[c];
      }

      AnalyticalFixtures::RunRotateFilter(dataStructure, k_EulerPath, fixture.axisAngle);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_EulerPath));
      const auto& resultRef = dataStructure.getDataRefAs<Float32Array>(k_EulerPath);
      for(usize c = 0; c < 3; c++)
      {
        const float64 absDif = std::fabs(static_cast<float64>(resultRef[c]) - fixture.expected[c]);
        INFO(fmt::format("{}: component {}: actual {} expected {}", fixture.name, c, resultRef[c], fixture.expected[c]));
        REQUIRE(absDif < k_Tolerance);
      }

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("OrientationAnalysis::RotateEulerRefFrameFilter: Zero-Length Axis Fails Preflight", "[OrientationAnalysis][RotateEulerRefFrameFilter]")
{
  UnitTest::LoadPlugins();

  const DataPath k_EulerPath({"CellData", "EulerAngles"});

  DataStructure dataStructure;
  AnalyticalFixtures::CreateEulerArray(dataStructure, 1);

  RotateEulerRefFrameFilter filter;
  Arguments args;
  args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxisAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.0F, 0.0F, 0.0F, 90.0F}));
  args.insertOrAssign(RotateEulerRefFrameFilter::k_EulerAnglesArrayPath_Key, std::make_any<DataPath>(k_EulerPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("OrientationAnalysis::RotateEulerRefFrameFilter: Class 4 Invariants", "[OrientationAnalysis][RotateEulerRefFrameFilter]")
{
  UnitTest::LoadPlugins();

  const DataPath k_EulerPath({"CellData", "EulerAngles"});

  // A batch of canonical-range orientations (phi1, phi2 in [0, 2pi), Phi in [0, pi]).
  const std::vector<std::array<float32, 3>> k_BatchEulers = {
      {0.0F, 0.0F, 0.0F}, {1.0F, 0.5F, 0.3F}, {0.7F, 0.9F, 4.2F}, {2.0F, 2.5F, 1.0F}, {5.9F, 0.1F, 3.3F}, {3.1F, 1.6F, 6.0F},
  };

  constexpr float32 k_TwoPi = 6.2831855F;
  constexpr float32 k_Pi = 3.1415927F;

  SECTION("Output ranges: phi1, phi2 in [0, 2pi], Phi in [0, pi]")
  {
    DataStructure dataStructure;
    Float32Array* eulerArray = AnalyticalFixtures::CreateEulerArray(dataStructure, k_BatchEulers.size());
    for(usize t = 0; t < k_BatchEulers.size(); t++)
    {
      for(usize c = 0; c < 3; c++)
      {
        (*eulerArray)[t * 3 + c] = k_BatchEulers[t][c];
      }
    }

    AnalyticalFixtures::RunRotateFilter(dataStructure, k_EulerPath, {0.3F, -0.5F, 0.81F, 37.0F});

    const auto& resultRef = dataStructure.getDataRefAs<Float32Array>(k_EulerPath);
    for(usize t = 0; t < k_BatchEulers.size(); t++)
    {
      INFO(fmt::format("tuple {}", t));
      REQUIRE(resultRef[t * 3 + 0] >= 0.0F);
      REQUIRE(resultRef[t * 3 + 0] <= k_TwoPi);
      REQUIRE(resultRef[t * 3 + 1] >= 0.0F);
      REQUIRE(resultRef[t * 3 + 1] <= k_Pi);
      REQUIRE(resultRef[t * 3 + 2] >= 0.0F);
      REQUIRE(resultRef[t * 3 + 2] <= k_TwoPi);
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Round-trip: rotate by (n, w) then (-n, w) recovers the input")
  {
    DataStructure dataStructure;
    Float32Array* eulerArray = AnalyticalFixtures::CreateEulerArray(dataStructure, k_BatchEulers.size());
    for(usize t = 0; t < k_BatchEulers.size(); t++)
    {
      for(usize c = 0; c < 3; c++)
      {
        (*eulerArray)[t * 3 + c] = k_BatchEulers[t][c];
      }
    }

    AnalyticalFixtures::RunRotateFilter(dataStructure, k_EulerPath, {0.3F, -0.5F, 0.81F, 37.0F});
    AnalyticalFixtures::RunRotateFilter(dataStructure, k_EulerPath, {-0.3F, 0.5F, -0.81F, 37.0F});

    const auto& resultRef = dataStructure.getDataRefAs<Float32Array>(k_EulerPath);
    constexpr float32 k_Tolerance = 1.0e-4F; // two float32 store/load passes
    for(usize i = 0; i < k_BatchEulers.size() * 3; i++)
    {
      const float32 absDif = std::fabs(resultRef[i] - k_BatchEulers[i / 3][i % 3]);
      INFO(fmt::format("flat index {}", i));
      REQUIRE(absDif < k_Tolerance);
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Composability: 45 deg twice equals 90 deg once")
  {
    DataStructure twiceDataStructure;
    Float32Array* twiceArray = AnalyticalFixtures::CreateEulerArray(twiceDataStructure, k_BatchEulers.size());
    DataStructure onceDataStructure;
    Float32Array* onceArray = AnalyticalFixtures::CreateEulerArray(onceDataStructure, k_BatchEulers.size());
    for(usize t = 0; t < k_BatchEulers.size(); t++)
    {
      for(usize c = 0; c < 3; c++)
      {
        (*twiceArray)[t * 3 + c] = k_BatchEulers[t][c];
        (*onceArray)[t * 3 + c] = k_BatchEulers[t][c];
      }
    }

    AnalyticalFixtures::RunRotateFilter(twiceDataStructure, k_EulerPath, {0.0F, 0.0F, 1.0F, 45.0F});
    AnalyticalFixtures::RunRotateFilter(twiceDataStructure, k_EulerPath, {0.0F, 0.0F, 1.0F, 45.0F});
    AnalyticalFixtures::RunRotateFilter(onceDataStructure, k_EulerPath, {0.0F, 0.0F, 1.0F, 90.0F});

    const auto& twiceRef = twiceDataStructure.getDataRefAs<Float32Array>(k_EulerPath);
    const auto& onceRef = onceDataStructure.getDataRefAs<Float32Array>(k_EulerPath);
    constexpr float32 k_Tolerance = 1.0e-4F;
    for(usize i = 0; i < k_BatchEulers.size() * 3; i++)
    {
      const float32 absDif = std::fabs(twiceRef[i] - onceRef[i]);
      INFO(fmt::format("flat index {}", i));
      REQUIRE(absDif < k_Tolerance);
    }

    UnitTest::CheckArraysInheritTupleDims(twiceDataStructure);
    UnitTest::CheckArraysInheritTupleDims(onceDataStructure);
  }
}
