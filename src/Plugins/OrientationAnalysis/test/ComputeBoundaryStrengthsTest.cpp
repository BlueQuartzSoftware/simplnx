#include <array>
#include <bit>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <utility>

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/ComputeBoundaryStrengthsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
DataPath faceDataPath = DataPath({Constants::k_TriangleDataContainerName, Constants::k_FaceData});
DataPath faceLabelsPath = faceDataPath.createChildPath(Constants::k_FaceLabels);
DataPath avgQuatsPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_AvgQuats});
DataPath featurePhasesPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Phases});
DataPath crystalStructuresPath = DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures});

const std::string k_f1s = "F1List_COMPUTED";
const std::string k_f1spts = "F1sptList_COMPUTED";
const std::string k_f7s = "F7List_COMPUTED";
const std::string k_mPrimes = "mPrimeList_COMPUTED";

const std::array<std::string, 4> k_BoundaryMetricOutputNames = {"mPrime", "F1", "F1spt", "F7"};
const std::array<uint32, 3> k_SymmetryCrystalStructures = {ebsdlib::CrystalStructure::UnknownCrystalStructure, ebsdlib::CrystalStructure::Hexagonal_High, ebsdlib::CrystalStructure::Cubic_High};
using BoundaryMetricsType = std::array<std::array<float32, 2>, 4>;

void CreateBoundaryStrengthInputs(DataStructure& dataStructure, int32 phase, const std::array<uint32, 3>& crystalStructures, usize numFaces = 1, const std::array<int32, 2>& faceLabels = {1, 2})
{
  const DataPath labelsPath({"FaceLabels"});
  auto labelsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, labelsPath, {numFaces}, {2});
  REQUIRE(labelsStore != nullptr);
  auto* labelsArrayPtr = Int32Array::Create(dataStructure, "FaceLabels", labelsStore);
  REQUIRE(labelsArrayPtr != nullptr);
  for(usize faceIdx = 0; faceIdx < numFaces; faceIdx++)
  {
    (*labelsArrayPtr)[2 * faceIdx] = faceLabels[0];
    (*labelsArrayPtr)[2 * faceIdx + 1] = faceLabels[1];
  }

  auto quatsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, DataPath({"AvgQuats"}), {3}, {4});
  REQUIRE(quatsStore != nullptr);
  auto* quatsArrayPtr = Float32Array::Create(dataStructure, "AvgQuats", quatsStore);
  REQUIRE(quatsArrayPtr != nullptr);
  quatsArrayPtr->fill(0.0F);
  (*quatsArrayPtr)[7] = 1.0F;
  (*quatsArrayPtr)[11] = 1.0F;

  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"FeaturePhases"}), {3}, {1});
  REQUIRE(phasesStore != nullptr);
  auto* phasesArrayPtr = Int32Array::Create(dataStructure, "FeaturePhases", phasesStore);
  REQUIRE(phasesArrayPtr != nullptr);
  phasesArrayPtr->fill(phase);
  (*phasesArrayPtr)[0] = 0;

  auto structuresStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, DataPath({"CrystalStructures"}), {3}, {1});
  REQUIRE(structuresStore != nullptr);
  auto* structuresArrayPtr = UInt32Array::Create(dataStructure, "CrystalStructures", structuresStore);
  REQUIRE(structuresArrayPtr != nullptr);
  for(usize ensembleIdx = 0; ensembleIdx < crystalStructures.size(); ensembleIdx++)
  {
    (*structuresArrayPtr)[ensembleIdx] = crystalStructures[ensembleIdx];
  }
}

void RequireBoundaryStrengthStore(const DataStructure& dataStructure, const DataPath& path, bool requireOoc)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(path));
  const auto& arrayRef = dataStructure.getDataRefAs<IDataArray>(path);
  CAPTURE(path.toString(), arrayRef.getIDataStoreRef().getDataFormat());
  if(requireOoc)
  {
    REQUIRE(arrayRef.getIDataStoreRef().getDataFormat() == "HDF5-OOC");
    REQUIRE(arrayRef.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  }
  else
  {
    REQUIRE(arrayRef.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  }
  std::cout << fmt::format("Boundary strength store path={} format={} expected={}\n", path.toString(), arrayRef.getIDataStoreRef().getDataFormat(), requireOoc ? "out-of-core" : "in-memory");
}

Result<> ExecuteBoundaryStrengths(DataStructure& dataStructure, bool requireOoc)
{
  for(const auto& name : {"FaceLabels", "AvgQuats", "FeaturePhases", "CrystalStructures"})
  {
    RequireBoundaryStrengthStore(dataStructure, DataPath({name}), requireOoc);
  }
  ComputeBoundaryStrengthsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_Loading_Key, std::make_any<VectorFloat64Parameter::ValueType>(std::vector<float64>{0.0, 0.0, 1.0}));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath({"FaceLabels"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"AvgQuats"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"FeaturePhases"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"CrystalStructures"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshmPrimesArrayName_Key, std::make_any<std::string>(k_BoundaryMetricOutputNames[0]));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sArrayName_Key, std::make_any<std::string>(k_BoundaryMetricOutputNames[1]));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sptsArrayName_Key, std::make_any<std::string>(k_BoundaryMetricOutputNames[2]));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF7sArrayName_Key, std::make_any<std::string>(k_BoundaryMetricOutputNames[3]));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  for(const auto& name : k_BoundaryMetricOutputNames)
  {
    RequireBoundaryStrengthStore(dataStructure, DataPath({name}), requireOoc);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
  return std::move(executeResult.result);
}

BoundaryMetricsType ReadBoundaryMetrics(const DataStructure& dataStructure, const std::string& fixtureName)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"FaceLabels"})));
  const auto& labelsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({"FaceLabels"}));
  BoundaryMetricsType metrics{};
  for(usize metricIdx = 0; metricIdx < k_BoundaryMetricOutputNames.size(); metricIdx++)
  {
    const DataPath path({k_BoundaryMetricOutputNames[metricIdx]});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(path));
    const auto& arrayRef = dataStructure.getDataRefAs<Float32Array>(path);
    REQUIRE(arrayRef.getNumberOfComponents() == 2);
    REQUIRE(arrayRef.getTupleShape() == labelsRef.getTupleShape());
    REQUIRE(arrayRef.getNumberOfTuples() >= 1);
    for(usize directionIdx = 0; directionIdx < 2; directionIdx++)
    {
      metrics[metricIdx][directionIdx] = arrayRef[directionIdx];
    }
    for(usize faceIdx = 0; faceIdx < arrayRef.getNumberOfTuples(); faceIdx++)
    {
      // Exact float bits let baseline and fixed logs supply a preservation check on the same build configuration.
      std::cout << fmt::format("Boundary strength metrics fixture={} store={} array={} face={} bits={:08x},{:08x}\n", fixtureName, arrayRef.getDataStoreRef().getDataFormat(),
                               k_BoundaryMetricOutputNames[metricIdx], faceIdx, std::bit_cast<uint32>(arrayRef[2 * faceIdx]), std::bit_cast<uint32>(arrayRef[2 * faceIdx + 1]));
      for(usize directionIdx = 0; faceIdx > 0 && directionIdx < 2; directionIdx++)
      {
        CHECK(std::bit_cast<uint32>(arrayRef[2 * faceIdx + directionIdx]) == std::bit_cast<uint32>(metrics[metricIdx][directionIdx]));
      }
    }
  }
  return metrics;
}

void CheckSymmetryWarning(const Result<>& result, bool expectWarning)
{
  if(expectWarning)
  {
    REQUIRE(result.warnings().size() == 1);
    CHECK(result.warnings()[0].code == -94739);
    CHECK(result.warnings()[0].message ==
          "Compute Boundary Strengths supports Cubic m-3m. A same-phase boundary uses another crystal structure. Check the Crystal Structures array 'CrystalStructures'.");
  }
  else
  {
    CHECK(result.warnings().empty());
  }
}

void CheckZeroBoundaryMetrics(const BoundaryMetricsType& metrics)
{
  for(const auto& directions : metrics)
  {
    for(const auto value : directions)
    {
      CHECK(value == 0.0F);
    }
  }
}

void CheckSymmetryWarningFixture(int32 phase, uint32 phaseZeroStructure, bool expectWarning, bool requireOoc, usize numFaces = 1)
{
  auto structures = k_SymmetryCrystalStructures;
  structures[0] = phaseZeroStructure;
  DataStructure dataStructure;
  CreateBoundaryStrengthInputs(dataStructure, phase, structures, numFaces);
  const auto result = ExecuteBoundaryStrengths(dataStructure, requireOoc);
  const auto metrics = ReadBoundaryMetrics(dataStructure, fmt::format("phase-{}-tuple0-{}-faces-{}", phase, phaseZeroStructure, numFaces));
  if(phase == 2)
  {
    CHECK(metrics[0][0] == Approx(1.0F).margin(1.0E-6F));
    CHECK(metrics[0][1] == Approx(1.0F).margin(1.0E-6F));
  }
  if(phase == 0)
  {
    CheckZeroBoundaryMetrics(metrics);
  }
  CheckSymmetryWarning(result, expectWarning);
}

void CheckBoundaryMetricPreservation(bool requireOoc)
{
  DataStructure cubicPhaseTwo;
  CreateBoundaryStrengthInputs(cubicPhaseTwo, 2, k_SymmetryCrystalStructures);
  const auto phaseTwoResult = ExecuteBoundaryStrengths(cubicPhaseTwo, requireOoc);
  const auto phaseTwoMetrics = ReadBoundaryMetrics(cubicPhaseTwo, "cubic-phase-2");

  auto relabeledStructures = k_SymmetryCrystalStructures;
  std::swap(relabeledStructures[1], relabeledStructures[2]);
  DataStructure cubicPhaseOne;
  CreateBoundaryStrengthInputs(cubicPhaseOne, 1, relabeledStructures);
  const auto phaseOneResult = ExecuteBoundaryStrengths(cubicPhaseOne, requireOoc);
  const auto phaseOneMetrics = ReadBoundaryMetrics(cubicPhaseOne, "cubic-phase-1-relabeled");
  for(usize metricIdx = 0; metricIdx < phaseTwoMetrics.size(); metricIdx++)
  {
    for(usize directionIdx = 0; directionIdx < 2; directionIdx++)
    {
      CHECK(std::bit_cast<uint32>(phaseTwoMetrics[metricIdx][directionIdx]) == std::bit_cast<uint32>(phaseOneMetrics[metricIdx][directionIdx]));
    }
  }
  CHECK(phaseTwoMetrics[0][0] == Approx(1.0F).margin(1.0E-6F));
  CHECK(phaseTwoMetrics[0][1] == Approx(1.0F).margin(1.0E-6F));

  DataStructure exteriorFace;
  CreateBoundaryStrengthInputs(exteriorFace, 1, k_SymmetryCrystalStructures, 1, {1, -1});
  const auto exteriorResult = ExecuteBoundaryStrengths(exteriorFace, requireOoc);
  CheckZeroBoundaryMetrics(ReadBoundaryMetrics(exteriorFace, "exterior-noncubic-phase-1"));
  CheckSymmetryWarning(exteriorResult, false);
  SIMPLNX_RESULT_REQUIRE_VALID(phaseTwoResult);
  SIMPLNX_RESULT_REQUIRE_VALID(phaseOneResult);
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "feature_boundary_neighbor_slip_transmission_1.tar.gz",
                                                              "feature_boundary_neighbor_slip_transmission");

  DataStructure dataStructure =
      UnitTest::LoadDataStructure(fs::path(fmt::format("{}/feature_boundary_neighbor_slip_transmission_1/6_6_feature_boundary_neighbor_slip_transmission.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeBoundaryStrengthsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_Loading_Key, std::make_any<VectorFloat64Parameter::ValueType>(std::vector<float64>{0, 0, 1}));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabelsPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sArrayName_Key, std::make_any<std::string>(k_f1s));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sptsArrayName_Key, std::make_any<std::string>(k_f1spts));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF7sArrayName_Key, std::make_any<std::string>(k_f7s));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshmPrimesArrayName_Key, std::make_any<std::string>(k_mPrimes));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // #ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_ComputeBoundaryStrengthsFilter.dream3d", unit_test::k_BinaryTestOutputDir)));
  // #endif

  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F1s"), faceDataPath.createChildPath(k_f1s));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F1spts"), faceDataPath.createChildPath(k_f1spts));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F7s"), faceDataPath.createChildPath(k_f7s));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("mPrimes"), faceDataPath.createChildPath(k_mPrimes));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeBoundaryStrengthsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeBoundaryStrengthsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeBoundaryStrengthsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (DoubleVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sptsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF7sArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeBoundaryStrengthsFilter::k_SurfaceMeshmPrimesArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: Phase and Laue Index Bounds", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  UnitTest::LoadPlugins();

  const int32 invalidPhaseIdx = GENERATE(-1, 2);
  CAPTURE(invalidPhaseIdx);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  DataStructure dataStructure;
  auto faceLabelsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"FaceLabels"}), {1}, {2});
  auto* faceLabelsArrayPtr = Int32Array::Create(dataStructure, "FaceLabels", faceLabelsStore);
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(faceLabelsArrayPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  (*faceLabelsArrayPtr)[0] = 1;
  (*faceLabelsArrayPtr)[1] = 2;

  auto avgQuatsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, DataPath({"AvgQuats"}), {3}, {4});
  auto* avgQuatsArrayPtr = Float32Array::Create(dataStructure, "AvgQuats", avgQuatsStore);
  avgQuatsArrayPtr->fill(0.0F);
  (*avgQuatsArrayPtr)[7] = 1.0F;
  (*avgQuatsArrayPtr)[11] = 1.0F;

  auto featurePhasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"FeaturePhases"}), {3}, {1});
  auto* featurePhasesArrayPtr = Int32Array::Create(dataStructure, "FeaturePhases", featurePhasesStore);
  featurePhasesArrayPtr->fill(0);
  (*featurePhasesArrayPtr)[1] = 1;
  (*featurePhasesArrayPtr)[2] = 1;

  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, DataPath({"CrystalStructures"}), {2}, {1});
  auto* crystalStructuresArrayPtr = UInt32Array::Create(dataStructure, "CrystalStructures", crystalStructuresStore);
  (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Cubic_High;

  ComputeBoundaryStrengthsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_Loading_Key, std::make_any<VectorFloat64Parameter::ValueType>(std::vector<float64>{0.0, 0.0, 1.0}));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath({"FaceLabels"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"AvgQuats"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"FeaturePhases"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"CrystalStructures"})));

  SECTION("Participating Phase returns an error")
  {
    (*featurePhasesArrayPtr)[2] = invalidPhaseIdx;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -94740);
  }

  SECTION("Participating Laue index returns an error")
  {
    (*crystalStructuresArrayPtr)[1] = 999U;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -94741);
  }

  SECTION("External Face is ignored")
  {
    (*faceLabelsArrayPtr)[1] = -1;
    (*featurePhasesArrayPtr)[1] = invalidPhaseIdx;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: cubic phase 2 warning", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  // PreferencesSentinel requires an application, but this direct filter test does not need plugin loading.
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  CheckSymmetryWarningFixture(2, ebsdlib::CrystalStructure::UnknownCrystalStructure, false, false);
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: noncubic phase 1 warning", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  const usize numFaces = GENERATE(1, 2);
  CAPTURE(numFaces);
  CheckSymmetryWarningFixture(1, ebsdlib::CrystalStructure::UnknownCrystalStructure, true, false, numFaces);
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: unknown phase 0 warning", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  CheckSymmetryWarningFixture(0, ebsdlib::CrystalStructure::UnknownCrystalStructure, true, false);
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: cubic phase 0 warning", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  CheckSymmetryWarningFixture(0, ebsdlib::CrystalStructure::Cubic_High, false, false);
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: numerical preservation", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  CheckBoundaryMetricPreservation(false);
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: real store warning and numerical preservation", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter][.OocStoreContract]")
{
  REQUIRE(Application::GetOrCreateInstance()->getIOManager("HDF5-OOC") != nullptr);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  SECTION("Cubic phase 2")
  {
    CheckSymmetryWarningFixture(2, ebsdlib::CrystalStructure::UnknownCrystalStructure, false, true);
  }
  SECTION("Noncubic phase 1")
  {
    CheckSymmetryWarningFixture(1, ebsdlib::CrystalStructure::UnknownCrystalStructure, true, true);
  }
  SECTION("Two unsupported faces produce one warning")
  {
    CheckSymmetryWarningFixture(1, ebsdlib::CrystalStructure::UnknownCrystalStructure, true, true, 2);
  }
  SECTION("Unknown phase 0")
  {
    CheckSymmetryWarningFixture(0, ebsdlib::CrystalStructure::UnknownCrystalStructure, true, true);
  }
  SECTION("Cubic phase 0")
  {
    CheckSymmetryWarningFixture(0, ebsdlib::CrystalStructure::Cubic_High, false, true);
  }
  SECTION("Numerical preservation")
  {
    CheckBoundaryMetricPreservation(true);
  }
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: Invalid Loading Direction", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  const std::vector<float64> loading = GENERATE(std::vector<float64>{0.0, 0.0, 0.0}, std::vector<float64>{std::numeric_limits<float64>::quiet_NaN(), 0.0, 1.0},
                                                std::vector<float64>{0.0, std::numeric_limits<float64>::infinity(), 1.0});
  CAPTURE(loading);

  DataStructure dataStructure;
  auto* faceLabels = UnitTest::CreateTestDataArray<int32>(dataStructure, "FaceLabels", {1}, {2});
  (*faceLabels)[0] = 1;
  (*faceLabels)[1] = 2;
  auto* avgQuats = UnitTest::CreateTestDataArray<float32>(dataStructure, "AvgQuats", {3}, {4});
  avgQuats->fill(0.0F);
  (*avgQuats)[7] = 1.0F;
  (*avgQuats)[11] = 1.0F;
  auto* featurePhases = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeaturePhases", {3}, {1});
  featurePhases->fill(1);
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1});
  (*crystalStructures)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructures)[1] = ebsdlib::CrystalStructure::Cubic_High;

  ComputeBoundaryStrengthsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_Loading_Key, std::make_any<VectorFloat64Parameter::ValueType>(loading));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath({"FaceLabels"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"AvgQuats"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"FeaturePhases"})));
  args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"CrystalStructures"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -94744);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -94744);
}

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: Default Loading Direction", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  // DREAM3D 6.5 FindBoundaryStrengths used [1, 1, 1] as the default loading direction.
  const auto loading = ComputeBoundaryStrengthsFilter().getDefaultArguments().value<VectorFloat64Parameter::ValueType>(ComputeBoundaryStrengthsFilter::k_Loading_Key);
  REQUIRE(loading == std::vector<float64>{1.0, 1.0, 1.0});
}
