#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/ComputeMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"
#include "simplnx/Common/Constants.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include <filesystem>
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;
using namespace nx::core;

namespace compute_misorientations_test
{
const DataPath k_CrystalStructuresDataPath({"CrystalStructures"});
const std::string k_OutputArrayName = "Output";

constexpr size_t k_Size = 4;
constexpr float32 k_Increment = 90.0f / k_Size;

std::vector<usize> k_TupleShape = {k_Size * k_Size * k_Size};

} // namespace compute_misorientations_test

/**
 * This whole section is being left in here in case we hae to regenerate the test files
 * again. After generation the developer will need to hand validate the output from
 * these functions are correct. The hand verification was performed by MAJ in March 2025
 */
#ifdef GENERATE_TEST_DATA
Result<> CreateDataStructure(DataStructure& dataStructure, uint32 xtal)
{
  UnitTest::LoadPlugins();
  std::vector<usize> compShape = {3};
  DataPath k_EulersDataPath = DataPath::FromString(fmt::format("{}/Eulers", xtal)).value();

  DataGroup* dgPtr = DataGroup::Create(dataStructure, fmt::format("{}", xtal));
  REQUIRE(dgPtr != nullptr);

  Result<> result = CreateArray<float32>(dataStructure, tupleShape, compShape, k_EulersDataPath, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  DataPath k_Eulers2DataPath = DataPath::FromString(fmt::format("{}/Eulers2", xtal)).value();
  result = CreateArray<float32>(dataStructure, tupleShape, compShape, k_Eulers2DataPath, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  compShape = {1};
  DataPath phasePath = DataPath::FromString(fmt::format("{}/Phases", xtal)).value();
  result = CreateArray<int32>(dataStructure, tupleShape, compShape, phasePath, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  auto& eulersRef = dataStructure.getDataRefAs<Float32Array>(k_EulersDataPath);
  auto& eulers2Ref = dataStructure.getDataRefAs<Float32Array>(k_Eulers2DataPath);
  auto& phasesRef = dataStructure.getDataRefAs<Int32Array>(phasePath);
  size_t tupleIdx = 0;

  for(size_t phi1 = 0; phi1 < size; phi1++)
  {
    for(size_t Phi = 0; Phi < size; Phi++)
    {
      for(size_t phi2 = 0; phi2 < size; phi2++)
      {
        eulersRef[tupleIdx * 3 + 0] = (phi1 * increment) * Constants::k_PiOver180F;
        eulersRef[tupleIdx * 3 + 1] = (Phi * increment) * Constants::k_PiOver180F;
        eulersRef[tupleIdx * 3 + 2] = (phi2 * increment) * Constants::k_PiOver180F;

        eulers2Ref[tupleIdx * 3 + 0] = (phi1 * increment * 0.5) * Constants::k_PiOver180F;
        eulers2Ref[tupleIdx * 3 + 1] = (Phi * increment * 2.0f) * Constants::k_PiOver180F;
        eulers2Ref[tupleIdx * 3 + 2] = (phi2 * increment) * Constants::k_PiOver180F;

        phasesRef[tupleIdx] = static_cast<int32>(xtal + 1);
        tupleIdx++;
      }
    }
  }
  return {};
}

void GenerateReferenceOrientationTestData()
{
  DataStructure dataStructure;

  Result<> result = CreateArray<uint32>(dataStructure, {12ULL}, {1ULL}, k_CrystalStructuresDataPath, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  auto& xtalRef = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresDataPath);
  xtalRef[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  for(uint32 i = 1; i < 12; i++)
  {
    xtalRef[i] = i - 1;
  }

  for(uint32 xtal = 0; xtal < 11; xtal++)
  {
    result = CreateDataStructure(dataStructure, xtal);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    DataPath k_EulersDataPath = DataPath::FromString(fmt::format("{}/Eulers", xtal)).value();
    DataPath k_Eulers2DataPath = DataPath::FromString(fmt::format("{}/Eulers2", xtal)).value();
    DataPath k_PhasesDataPath = DataPath::FromString(fmt::format("{}/Phases", xtal)).value();

    ComputeMisorientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeMisorientationsFilter::k_ComputationType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(ComputeMisorientationsFilter::k_InputOrientationArrayPath1_Key, std::make_any<DataPath>(k_EulersDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_ReferenceOrientation_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0f, 0.0f, 1.0f, 0.0f}));
    args.insertOrAssign(ComputeMisorientationsFilter::k_OutputMisorientationArrayName_Key, std::make_any<std::string>(k_OutputArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/compute_misorientations/ComputeMisorientationsFilter_Ref.dream3d", unit_test::k_TestFilesDir)));
}

void GenerateTestDataInputArrays()
{
  DataStructure dataStructure;

  Result<> result = CreateArray<uint32>(dataStructure, {12ULL}, {1ULL}, k_CrystalStructuresDataPath, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  auto& xtalRef = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresDataPath);
  xtalRef[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  for(uint32 i = 1; i < 12; i++)
  {
    xtalRef[i] = i - 1;
  }

  for(uint32 xtal = 0; xtal < 11; xtal++)
  {
    result = CreateDataStructure(dataStructure, xtal);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    DataPath k_EulersDataPath = DataPath::FromString(fmt::format("{}/Eulers", xtal)).value();
    DataPath k_Eulers2DataPath = DataPath::FromString(fmt::format("{}/Eulers2", xtal)).value();
    DataPath k_PhasesDataPath = DataPath::FromString(fmt::format("{}/Phases", xtal)).value();

    ComputeMisorientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeMisorientationsFilter::k_ComputationType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
    args.insertOrAssign(ComputeMisorientationsFilter::k_InputOrientationArrayPath1_Key, std::make_any<DataPath>(k_EulersDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_InputOrientationArrayPath2_Key, std::make_any<DataPath>(k_Eulers2DataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_ReferenceOrientation_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0f, 0.0f, 1.0f, 0.0f}));
    args.insertOrAssign(ComputeMisorientationsFilter::k_OutputMisorientationArrayName_Key, std::make_any<std::string>(k_OutputArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/compute_misorientations/ComputeMisorientationsFilter_Arrays.dream3d", unit_test::k_TestFilesDir)));
}

TEST_CASE("OrientationAnalysis::ComputeMisorientationsFilter:MakeTestData", "[OrientationAnalysis][ComputeMisorientations]")
{
  GenerateTestDataInputArrays();
  GenerateReferenceOrientationTestData();

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
#endif

TEST_CASE("OrientationAnalysis::ComputeMisorientationsFilter:Reference Orientation", "[OrientationAnalysis][ComputeMisorientations]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "compute_misorientations.tar.gz", "compute_misorientations");

  auto baseDataFilePath = fs::path(fmt::format("{}/compute_misorientations/ComputeMisorientationsFilter_Ref.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  for(uint32 xtal = 0; xtal < 11; xtal++)
  {
    DataPath k_EulersDataPath = DataPath::FromString(fmt::format("{}/Eulers", xtal)).value();
    DataPath k_Eulers2DataPath = DataPath::FromString(fmt::format("{}/Eulers2", xtal)).value();
    DataPath k_PhasesDataPath = DataPath::FromString(fmt::format("{}/Phases", xtal)).value();

    ComputeMisorientationsFilter filter;
    Arguments args;
    std::string computedArrayName = "Computed Values";
    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeMisorientationsFilter::k_ComputationType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(ComputeMisorientationsFilter::k_InputOrientationArrayPath1_Key, std::make_any<DataPath>(k_EulersDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_InputOrientationArrayPath2_Key, std::make_any<DataPath>(k_Eulers2DataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(compute_misorientations_test::k_CrystalStructuresDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_ReferenceOrientation_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0f, 0.0f, 1.0f, 0.0f}));
    args.insertOrAssign(ComputeMisorientationsFilter::k_OutputMisorientationArrayName_Key, std::make_any<std::string>(computedArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());

    UnitTest::CompareArrays<float32>(dataStructure, k_EulersDataPath.replaceName(computedArrayName), k_EulersDataPath.replaceName(compute_misorientations_test::k_OutputArrayName));
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/compute_misorientation_reference_orientation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeMisorientationsFilter:InputArrays", "[Reconstruction][ComputeMisorientationsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "compute_misorientations.tar.gz", "compute_misorientations");

  auto baseDataFilePath = fs::path(fmt::format("{}/compute_misorientations/ComputeMisorientationsFilter_Arrays.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  for(uint32 xtal = 0; xtal < 11; xtal++)
  {
    INFO(fmt::format("Crystal Class:'{}'", xtal));

    DataPath k_EulersDataPath = DataPath::FromString(fmt::format("{}/Eulers", xtal)).value();
    DataPath k_Eulers2DataPath = DataPath::FromString(fmt::format("{}/Eulers2", xtal)).value();
    DataPath k_PhasesDataPath = DataPath::FromString(fmt::format("{}/Phases", xtal)).value();

    ComputeMisorientationsFilter filter;
    Arguments args;

    std::string computedArrayName = "Computed Values";
    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeMisorientationsFilter::k_ComputationType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
    args.insertOrAssign(ComputeMisorientationsFilter::k_InputOrientationArrayPath1_Key, std::make_any<DataPath>(k_EulersDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_InputOrientationArrayPath2_Key, std::make_any<DataPath>(k_Eulers2DataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(compute_misorientations_test::k_CrystalStructuresDataPath));
    args.insertOrAssign(ComputeMisorientationsFilter::k_ReferenceOrientation_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0f, 0.0f, 1.0f, 0.0f}));
    args.insertOrAssign(ComputeMisorientationsFilter::k_OutputMisorientationArrayName_Key, std::make_any<std::string>(computedArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());

    UnitTest::CompareArrays<float32>(dataStructure, k_EulersDataPath.replaceName(computedArrayName), k_EulersDataPath.replaceName(compute_misorientations_test::k_OutputArrayName));
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/compute_misorientation_arrays.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
