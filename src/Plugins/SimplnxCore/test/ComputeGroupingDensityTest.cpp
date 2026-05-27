#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NeighborListSelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ComputeGroupingDensityFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// =============================================================================
// Exemplar archive paths (compute_grouping_densities_v2.tar.gz)
// =============================================================================
const std::string k_TestDataDirName = "compute_grouping_densities_v2";
const fs::path k_TestDataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_TestDataDirName;
const fs::path k_InputFile = k_TestDataDir / "data" / "compute_grouping_density_inputs.dream3d";
const fs::path k_ExemplarFile = k_TestDataDir / "output_simplnx" / "simplnx_compute_grouping_density_ab.dream3d";

// =============================================================================
// DataPath constants for the v2 input file
// =============================================================================
const std::string k_DataContainerName = "DataContainer";
const std::string k_FeatureAMName = "FeatureData";
const std::string k_ParentAMName = "ParentData";

const auto k_VolumesPath = DataPath({k_DataContainerName, k_FeatureAMName, "Volumes"});
const auto k_ParentIdsPath = DataPath({k_DataContainerName, k_FeatureAMName, "ParentIds"});
const auto k_ContiguousNLPath = DataPath({k_DataContainerName, k_FeatureAMName, "ContiguousNeighborList"});
const auto k_NonContiguousNLPath = DataPath({k_DataContainerName, k_FeatureAMName, "NonContiguousNeighborList"});
const auto k_ParentVolumesPath = DataPath({k_DataContainerName, k_ParentAMName, "ParentVolumes"});

// Output array names (placed by the filter into the same AMs as the inputs)
const std::string k_ComputedGroupingDensitiesName = "ComputedGroupingDensities";
const std::string k_ComputedCheckedFeaturesName = "ComputedCheckedFeatures";
const auto k_ComputedGroupingDensitiesPath = DataPath({k_DataContainerName, k_ParentAMName, k_ComputedGroupingDensitiesName});
const auto k_ComputedCheckedFeaturesPath = DataPath({k_DataContainerName, k_FeatureAMName, k_ComputedCheckedFeaturesName});

// Constants used by the inline preflight-error and edge-case tests.
const std::string k_ImageGeomName = "ImageGeom";
constexpr usize k_NumFeatures = 6;
constexpr usize k_NumParents = 3;
} // namespace

// =============================================================================
// Exemplar-based test: exercises all 4 (UseNonContiguousNeighbors, FindCheckedFeatures)
// configurations against pre-validated outputs in compute_grouping_densities_v2.tar.gz.
//
// The v2 exemplar archive was hand-reviewed and signed off by the filter author,
// and the SIMPLNX outputs in it were independently confirmed bit-identical to
// the legacy DREAM3D 6.5.172 `FindGroupingDensity` filter (the pre-SIMPLNX port
// source) — see src/Plugins/SimplnxCore/vv/ComputeGroupingDensityFilter.md and
// src/Plugins/SimplnxCore/vv/deviations/ComputeGroupingDensityFilter.md.
//
// Driving the test from the same exemplar archive used for the V&V comparison
// gives a single source of truth: any future change to either the algorithm or
// the exemplar surfaces here.
// =============================================================================

TEST_CASE("SimplnxCore::ComputeGroupingDensityFilter: Exemplar A/B — all 4 configurations", "[SimplnxCore][ComputeGroupingDensityFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_grouping_densities_v2.tar.gz", k_TestDataDirName);

  // Generate all 4 (UseNonContig, FindCheckedFeatures) combinations.
  // The suffix matches the exemplar array naming inside the v2 archive.
  auto config = GENERATE(table<bool, bool, std::string>({
      {false, false, "NC0_CF0"},
      {false, true, "NC0_CF1"},
      {true, false, "NC1_CF0"},
      {true, true, "NC1_CF1"},
  }));
  const bool useNonContiguous = std::get<0>(config);
  const bool findCheckedFeatures = std::get<1>(config);
  const std::string suffix = std::get<2>(config);

  DYNAMIC_SECTION("Config " << suffix << "  UseNonContig=" << useNonContiguous << "  FindChecked=" << findCheckedFeatures)
  {
    // Fresh DataStructure per configuration so output paths don't collide
    DataStructure dataStructure = UnitTest::LoadDataStructure(k_InputFile);

    ComputeGroupingDensityFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeGroupingDensityFilter::k_FeatureVolumesArrayPath_Key, std::make_any<DataPath>(k_VolumesPath));
    args.insertOrAssign(ComputeGroupingDensityFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(k_ContiguousNLPath));
    args.insertOrAssign(ComputeGroupingDensityFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(useNonContiguous));
    args.insertOrAssign(ComputeGroupingDensityFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(useNonContiguous ? k_NonContiguousNLPath : DataPath{}));
    args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentIdsPath_Key, std::make_any<DataPath>(k_ParentIdsPath));
    args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentVolumesPath_Key, std::make_any<DataPath>(k_ParentVolumesPath));
    args.insertOrAssign(ComputeGroupingDensityFilter::k_FindCheckedFeatures_Key, std::make_any<bool>(findCheckedFeatures));
    args.insertOrAssign(ComputeGroupingDensityFilter::k_CheckedFeaturesName_Key, std::make_any<std::string>(k_ComputedCheckedFeaturesName));
    args.insertOrAssign(ComputeGroupingDensityFilter::k_GroupingDensitiesName_Key, std::make_any<std::string>(k_ComputedGroupingDensitiesName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Load the exemplar — it contains all 4 pre-computed output configurations
    // as separately-named arrays (e.g., "GroupingDensities_NC0_CF1")
    DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);

    // GroupingDensities is always produced
    const DataPath exemplarDensitiesPath = DataPath({k_DataContainerName, k_ParentAMName, "GroupingDensities_" + suffix});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_ComputedGroupingDensitiesPath));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Float32Array>(exemplarDensitiesPath));
    const auto& computedDensities = dataStructure.getDataRefAs<Float32Array>(k_ComputedGroupingDensitiesPath);
    const auto& exemplarDensities = exemplarDS.getDataRefAs<Float32Array>(exemplarDensitiesPath);
    UnitTest::CompareDataArrays<float32>(exemplarDensities, computedDensities);

    // CheckedFeatures is produced only when FindCheckedFeatures==true
    if(findCheckedFeatures)
    {
      const DataPath exemplarCheckedFeaturesPath = DataPath({k_DataContainerName, k_FeatureAMName, "CheckedFeatures_" + suffix});
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_ComputedCheckedFeaturesPath));
      REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32Array>(exemplarCheckedFeaturesPath));
      const auto& computedCheckedFeatures = dataStructure.getDataRefAs<Int32Array>(k_ComputedCheckedFeaturesPath);
      const auto& exemplarCheckedFeatures = exemplarDS.getDataRefAs<Int32Array>(exemplarCheckedFeaturesPath);
      UnitTest::CompareDataArrays<int32>(exemplarCheckedFeatures, computedCheckedFeatures);
    }

    // Class 4 (Invariant) oracle assertions — properties that any valid output must satisfy.
    // Independent of the exemplar bit-comparison above; these would catch a future
    // regression even if someone "fixed" the exemplar incorrectly to match buggy code.
    const auto& invDensities = dataStructure.getDataRefAs<Float32Array>(k_ComputedGroupingDensitiesPath);
    REQUIRE(invDensities[0] == 0.0f); // placeholder parent never touched
    for(usize i = 1; i < invDensities.getNumberOfTuples(); ++i)
    {
      // density is either positive (parent had at least one assigned feature)
      // or exactly the -1.0f sentinel (parent had no assigned features).
      REQUIRE((invDensities[i] > 0.0f || invDensities[i] == -1.0f));
      // totalCheckVolume always includes the parent's own features,
      // so it is never smaller than ParentVolumes[i] -> density <= 1.0.
      REQUIRE(invDensities[i] <= 1.0f);
    }
    if(findCheckedFeatures)
    {
      const auto& invChecked = dataStructure.getDataRefAs<Int32Array>(k_ComputedCheckedFeaturesPath);
      REQUIRE(invChecked[0] == 0); // placeholder feature never claimed
      const int32 maxParentId = static_cast<int32>(invDensities.getNumberOfTuples()) - 1;
      for(usize i = 1; i < invChecked.getNumberOfTuples(); ++i)
      {
        REQUIRE((invChecked[i] >= 0 && invChecked[i] <= maxParentId));
      }
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

// =============================================================================
// Edge case: a parent with no features assigned -> totalCheckVolume == 0
// triggers the -1.0f sentinel write at ComputeGroupingDensity.cpp line 114.
// Not exercised by the v2 exemplar (every parent has features).
// =============================================================================

TEST_CASE("SimplnxCore::ComputeGroupingDensityFilter: Empty-parent edge case (-1.0f sentinel)", "[SimplnxCore][ComputeGroupingDensityFilter]")
{
  UnitTest::LoadPlugins();

  // 3 features (indices 1, 2 carry data; 0 is the SIMPL placeholder).
  // 3 parents (index 1 has features assigned; index 2 has NONE).
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setDimensions({1, 1, 1});

  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {3}, imageGeom->getId());
  auto* parentAM = AttributeMatrix::Create(dataStructure, "ParentData", {3}, imageGeom->getId());

  // Feature volumes: [0, 5, 10]
  auto* featureVolumes = UnitTest::CreateTestDataArray<float32>(dataStructure, "Volumes", {3}, {1}, featureAM->getId());
  auto& featureVolumesRef = featureVolumes->getDataStoreRef();
  featureVolumesRef[0] = 0.0f;
  featureVolumesRef[1] = 5.0f;
  featureVolumesRef[2] = 10.0f;

  // All non-placeholder features map to parent 1; parent 2 has no features.
  auto* parentIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "ParentIds", {3}, {1}, featureAM->getId());
  auto& parentIdsRef = parentIds->getDataStoreRef();
  parentIdsRef[0] = 0;
  parentIdsRef[1] = 1;
  parentIdsRef[2] = 1;

  // Trivial contiguous neighbor list — empty for every feature.
  auto* contiguousNL = NeighborList<int32>::Create(dataStructure, "ContigNL", ShapeType{3}, featureAM->getId());
  contiguousNL->setList(0, std::make_shared<std::vector<int32>>(std::vector<int32>{}));
  contiguousNL->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{}));
  contiguousNL->setList(2, std::make_shared<std::vector<int32>>(std::vector<int32>{}));

  // Parent volumes: parent 1 sums to 15 (5+10); parent 2 is non-zero but
  // irrelevant — totalCheckVolume==0 path triggers regardless of ParentVolumes[2].
  auto* parentVolumes = UnitTest::CreateTestDataArray<float32>(dataStructure, "ParentVolumes", {3}, {1}, parentAM->getId());
  auto& parentVolumesRef = parentVolumes->getDataStoreRef();
  parentVolumesRef[0] = 0.0f;
  parentVolumesRef[1] = 15.0f;
  parentVolumesRef[2] = 7.0f;

  const DataPath volumesPath = DataPath({k_ImageGeomName, "FeatureData", "Volumes"});
  const DataPath parentIdsPath = DataPath({k_ImageGeomName, "FeatureData", "ParentIds"});
  const DataPath contigNLPath = DataPath({k_ImageGeomName, "FeatureData", "ContigNL"});
  const DataPath parentVolumesPath = DataPath({k_ImageGeomName, "ParentData", "ParentVolumes"});
  const DataPath outputDensitiesPath = DataPath({k_ImageGeomName, "ParentData", "GroupingDensities"});

  ComputeGroupingDensityFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FeatureVolumesArrayPath_Key, std::make_any<DataPath>(volumesPath));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(contigNLPath));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentIdsPath_Key, std::make_any<DataPath>(parentIdsPath));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentVolumesPath_Key, std::make_any<DataPath>(parentVolumesPath));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FindCheckedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_CheckedFeaturesName_Key, std::make_any<std::string>("CheckedFeatures"));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_GroupingDensitiesName_Key, std::make_any<std::string>("GroupingDensities"));

  auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(outputDensitiesPath));
  const auto& densities = dataStructure.getDataRefAs<Float32Array>(outputDensitiesPath);

  // Parent 1: totalCheckVolume = 5 + 10 = 15; density = 15/15 = 1.0
  // Parent 2: NO features assigned -> totalCheckVolume == 0 -> -1.0f sentinel
  REQUIRE(densities[1] == Approx(1.0f).epsilon(0.0001f));
  REQUIRE(densities[2] == -1.0f);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// =============================================================================
// Preflight error tests — one TEST_CASE per error code in preflightImpl().
// =============================================================================

TEST_CASE("SimplnxCore::ComputeGroupingDensityFilter: Preflight Error - Feature tuple count mismatch (-15671)", "[SimplnxCore][ComputeGroupingDensityFilter]")
{
  UnitTest::LoadPlugins();

  // ParentIds in a different AM with mismatched tuple count
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setDimensions({1, 1, 1});

  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {k_NumFeatures}, imageGeom->getId());
  auto* parentAM = AttributeMatrix::Create(dataStructure, "ParentData", {k_NumParents}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Volumes", {k_NumFeatures}, {1}, featureAM->getId());
  NeighborList<int32>::Create(dataStructure, "ContigNL", ShapeType{k_NumFeatures}, featureAM->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "ParentVolumes", {k_NumParents}, {1}, parentAM->getId());

  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchAM", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "ParentIds", {10}, {1}, mismatchAM->getId());

  ComputeGroupingDensityFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FeatureVolumesArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "FeatureData", "Volumes"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "FeatureData", "ContigNL"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentIdsPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "MismatchAM", "ParentIds"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentVolumesPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "ParentData", "ParentVolumes"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FindCheckedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_CheckedFeaturesName_Key, std::make_any<std::string>("CheckedFeatures"));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_GroupingDensitiesName_Key, std::make_any<std::string>("GroupingDensities"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("SimplnxCore::ComputeGroupingDensityFilter: Preflight Error - NonContiguousNL tuple count mismatch (-15672)", "[SimplnxCore][ComputeGroupingDensityFilter]")
{
  UnitTest::LoadPlugins();

  // Feature-level arrays are all 6 tuples; NonContiguousNL is 4 tuples in a different AM
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setDimensions({1, 1, 1});

  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {k_NumFeatures}, imageGeom->getId());
  auto* parentAM = AttributeMatrix::Create(dataStructure, "ParentData", {k_NumParents}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Volumes", {k_NumFeatures}, {1}, featureAM->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "ParentIds", {k_NumFeatures}, {1}, featureAM->getId());
  NeighborList<int32>::Create(dataStructure, "ContigNL", ShapeType{k_NumFeatures}, featureAM->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "ParentVolumes", {k_NumParents}, {1}, parentAM->getId());

  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchAM", {4}, imageGeom->getId());
  NeighborList<int32>::Create(dataStructure, "NonContigNL", ShapeType{4}, mismatchAM->getId());

  ComputeGroupingDensityFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FeatureVolumesArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "FeatureData", "Volumes"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "FeatureData", "ContigNL"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "MismatchAM", "NonContigNL"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentIdsPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "FeatureData", "ParentIds"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentVolumesPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "ParentData", "ParentVolumes"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FindCheckedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_CheckedFeaturesName_Key, std::make_any<std::string>("CheckedFeatures"));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_GroupingDensitiesName_Key, std::make_any<std::string>("GroupingDensities"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("SimplnxCore::ComputeGroupingDensityFilter: Preflight Error - Volumes not in AttributeMatrix (-15673)", "[SimplnxCore][ComputeGroupingDensityFilter]")
{
  UnitTest::LoadPlugins();

  // Volumes placed directly under ImageGeom (no AttributeMatrix parent)
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setDimensions({1, 1, 1});

  UnitTest::CreateTestDataArray<float32>(dataStructure, "Volumes", {k_NumFeatures}, {1}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "ParentIds", {k_NumFeatures}, {1}, imageGeom->getId());
  NeighborList<int32>::Create(dataStructure, "ContigNL", ShapeType{k_NumFeatures}, imageGeom->getId());

  auto* parentAM = AttributeMatrix::Create(dataStructure, "ParentData", {k_NumParents}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "ParentVolumes", {k_NumParents}, {1}, parentAM->getId());

  ComputeGroupingDensityFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FeatureVolumesArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "Volumes"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "ContigNL"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentIdsPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "ParentIds"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentVolumesPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "ParentData", "ParentVolumes"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FindCheckedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_CheckedFeaturesName_Key, std::make_any<std::string>("CheckedFeatures"));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_GroupingDensitiesName_Key, std::make_any<std::string>("GroupingDensities"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

TEST_CASE("SimplnxCore::ComputeGroupingDensityFilter: Preflight Error - Parent Volumes not in AttributeMatrix (-15670)", "[SimplnxCore][ComputeGroupingDensityFilter]")
{
  UnitTest::LoadPlugins();

  // ParentVolumes placed directly under ImageGeom (no AM parent)
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setDimensions({1, 1, 1});

  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {k_NumFeatures}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Volumes", {k_NumFeatures}, {1}, featureAM->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "ParentIds", {k_NumFeatures}, {1}, featureAM->getId());
  NeighborList<int32>::Create(dataStructure, "ContigNL", ShapeType{k_NumFeatures}, featureAM->getId());

  UnitTest::CreateTestDataArray<float32>(dataStructure, "ParentVolumes", {k_NumParents}, {1}, imageGeom->getId());

  ComputeGroupingDensityFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FeatureVolumesArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "FeatureData", "Volumes"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "FeatureData", "ContigNL"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentIdsPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "FeatureData", "ParentIds"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_ParentVolumesPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeomName, "ParentVolumes"})));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_FindCheckedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_CheckedFeaturesName_Key, std::make_any<std::string>("CheckedFeatures"));
  args.insertOrAssign(ComputeGroupingDensityFilter::k_GroupingDensitiesName_Key, std::make_any<std::string>("GroupingDensities"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}

// =============================================================================
// SIMPL JSON backwards-compatibility — verifies FromSIMPLJson() correctly
// translates the SIMPL 6.5 filter parameter keys to the simplnx Arguments.
// =============================================================================

TEST_CASE("SimplnxCore::ComputeGroupingDensityFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeGroupingDensityFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeGroupingDensityFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeGroupingDensityFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(ComputeGroupingDensityFilter::k_CheckedFeaturesName_Key) == "CheckedFeatures");
      CHECK(args.value<DataPath>(ComputeGroupingDensityFilter::k_ContiguousNeighborListArrayPath_Key) == DataPath({"DataContainer", "FeatureData", "ContiguousNeighborList"}));
      CHECK(args.value<bool>(ComputeGroupingDensityFilter::k_FindCheckedFeatures_Key) == true);
      CHECK(args.value<DataPath>(ComputeGroupingDensityFilter::k_NonContiguousNeighborListArrayPath_Key) == DataPath({"DataContainer", "FeatureData", "NonContiguousNeighborList"}));
      CHECK(args.value<std::string>(ComputeGroupingDensityFilter::k_GroupingDensitiesName_Key) == "GroupingDensities");
      CHECK(args.value<DataPath>(ComputeGroupingDensityFilter::k_ParentIdsPath_Key) == DataPath({"DataContainer", "FeatureData", "ParentIds"}));
      CHECK(args.value<DataPath>(ComputeGroupingDensityFilter::k_ParentVolumesPath_Key) == DataPath({"DataContainer", "FeatureData", "ParentVolumes"}));
      CHECK(args.value<bool>(ComputeGroupingDensityFilter::k_UseNonContiguousNeighbors_Key) == true);
      CHECK(args.value<DataPath>(ComputeGroupingDensityFilter::k_FeatureVolumesArrayPath_Key) == DataPath({"DataContainer", "FeatureData", "Volumes"}));
    }
  }
}
