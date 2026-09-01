#include "OrientationAnalysis/Filters/Algorithms/ComputeKernelAvgMisorientationsScanline.hpp"
#include "OrientationAnalysis/Filters/ComputeKernelAvgMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
/**
 * @class CacheMemoryBudgetSentinel
 * @brief Restores the shared cache budget after a bounded-cache test.
 *
 * The sentinel clears cached state before it changes or restores the budget.
 */
class CacheMemoryBudgetSentinel
{
public:
  explicit CacheMemoryBudgetSentinel(uint64 budget)
  : m_Manager(CacheMemoryBudgetManager::instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(budget);
  }

  ~CacheMemoryBudgetSentinel()
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }

  CacheMemoryBudgetSentinel(const CacheMemoryBudgetSentinel&) = delete;
  CacheMemoryBudgetSentinel(CacheMemoryBudgetSentinel&&) noexcept = delete;
  CacheMemoryBudgetSentinel& operator=(const CacheMemoryBudgetSentinel&) = delete;
  CacheMemoryBudgetSentinel& operator=(CacheMemoryBudgetSentinel&&) noexcept = delete;

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget = 0;
};

namespace AnalyticalFixtures
{
const std::string k_GeomName = "ImageGeometry";
const DataPath k_ImageGeomPath = DataPath({k_GeomName});
const DataPath k_CellDataPath = k_ImageGeomPath.createChildPath("CellData");
const DataPath k_EnsembleDataPath = k_ImageGeomPath.createChildPath("EnsembleData");

const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_CellPhasesName = "Phases";
const std::string k_QuatsName = "Quats";
const std::string k_CrystalStructuresName = "CrystalStructures";

const std::string k_KAMOutName = "KernelAverageMisorientationsOut";

std::array<float32, 4> QuatFromPhi1Deg(float32 phi1Deg)
{
  const float32 halfAngleRad = (phi1Deg * 0.5f) * 3.14159265358979323846f / 180.0f;
  return {0.0f, 0.0f, std::sin(halfAngleRad), std::cos(halfAngleRad)};
}

/**
 * @struct FixtureData
 * @brief Holds the arrays for one analytical KAM fixture.
 */
struct FixtureData
{
  DataStructure ds;
  ImageGeom* geom = nullptr;
  AttributeMatrix* cellAM = nullptr;
  AttributeMatrix* ensembleAM = nullptr;
  Int32Array* featureIds = nullptr;
  Int32Array* cellPhases = nullptr;
  Float32Array* quats = nullptr;
  UInt32Array* crystalStructures = nullptr;
  usize totalCells = 0;
};

// Build valid image, cell, and ensemble arrays. Cells start in feature and
// phase one with identity quaternions. Ensemble index one is Cubic_High.
FixtureData CreateScaffold(usize nX, usize nY, usize nZ, usize numCrystalStructures = 2)
{
  FixtureData td;
  td.totalCells = nX * nY * nZ;

  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({nX, nY, nZ});

  td.cellAM = AttributeMatrix::Create(td.ds, "CellData", ShapeType{nZ, nY, nX}, td.geom->getId());
  td.ensembleAM = AttributeMatrix::Create(td.ds, "EnsembleData", ShapeType{numCrystalStructures}, td.geom->getId());

  td.featureIds = CreateTestDataArray<int32>(td.ds, k_FeatureIdsName, {nZ, nY, nX}, {1}, td.cellAM->getId());
  td.cellPhases = CreateTestDataArray<int32>(td.ds, k_CellPhasesName, {nZ, nY, nX}, {1}, td.cellAM->getId());
  td.quats = CreateTestDataArray<float32>(td.ds, k_QuatsName, {nZ, nY, nX}, {4}, td.cellAM->getId());
  td.crystalStructures = CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {numCrystalStructures}, {1}, td.ensembleAM->getId());

  for(usize i = 0; i < td.totalCells; ++i)
  {
    (*td.featureIds)[i] = 1;
    (*td.cellPhases)[i] = 1;
    (*td.quats)[i * 4 + 0] = 0.0f;
    (*td.quats)[i * 4 + 1] = 0.0f;
    (*td.quats)[i * 4 + 2] = 0.0f;
    (*td.quats)[i * 4 + 3] = 1.0f;
  }
  (*td.crystalStructures)[0] = 999u;
  if(numCrystalStructures > 1)
  {
    (*td.crystalStructures)[1] = 1u;
  }
  return td;
}

void SetCellQuat(FixtureData& td, usize cellIdx, const std::array<float32, 4>& q)
{
  (*td.quats)[cellIdx * 4 + 0] = q[0];
  (*td.quats)[cellIdx * 4 + 1] = q[1];
  (*td.quats)[cellIdx * 4 + 2] = q[2];
  (*td.quats)[cellIdx * 4 + 3] = q[3];
}

Arguments BuildArgs(const std::vector<int32>& kernelRadius, bool useFeatureIds = true)
{
  Arguments args;
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_KernelSize_Key, std::make_any<VectorInt32Parameter::ValueType>(kernelRadius));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_UseFeatureIds_Key, std::make_any<bool>(useFeatureIds));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_FeatureIdsName)));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_CellPhasesName)));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_QuatsName)));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_EnsembleDataPath.createChildPath(k_CrystalStructuresName)));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_KernelAverageMisorientationsArrayName_Key, std::make_any<std::string>(k_KAMOutName));
  return args;
}

const Float32Array& GetOutputKAM(const DataStructure& ds)
{
  return ds.getDataRefAs<Float32Array>(k_CellDataPath.createChildPath(k_KAMOutName));
}
} // namespace AnalyticalFixtures

AnalyticalFixtures::FixtureData CreateOocFixture(usize nX, usize nY, usize nZ)
{
  AnalyticalFixtures::FixtureData td;
  td.totalCells = nX * nY * nZ;
  const ShapeType tupleShape = {nZ, nY, nX};

  td.geom = ImageGeom::Create(td.ds, AnalyticalFixtures::k_GeomName);
  td.geom->setSpacing({1.0F, 1.0F, 1.0F});
  td.geom->setOrigin({0.0F, 0.0F, 0.0F});
  td.geom->setDimensions({nX, nY, nZ});

  td.cellAM = AttributeMatrix::Create(td.ds, "CellData", tupleShape, td.geom->getId());
  td.geom->setCellData(*td.cellAM);
  td.ensembleAM = AttributeMatrix::Create(td.ds, "EnsembleData", {2}, td.geom->getId());

  auto featureIdsStore =
      DataStoreUtilities::CreateDataStore<int32>(td.ds, AnalyticalFixtures::k_CellDataPath.createChildPath(AnalyticalFixtures::k_FeatureIdsName), tupleShape, {1}, IDataAction::Mode::Execute);
  td.featureIds = Int32Array::Create(td.ds, AnalyticalFixtures::k_FeatureIdsName, featureIdsStore, td.cellAM->getId());

  auto phasesStore =
      DataStoreUtilities::CreateDataStore<int32>(td.ds, AnalyticalFixtures::k_CellDataPath.createChildPath(AnalyticalFixtures::k_CellPhasesName), tupleShape, {1}, IDataAction::Mode::Execute);
  td.cellPhases = Int32Array::Create(td.ds, AnalyticalFixtures::k_CellPhasesName, phasesStore, td.cellAM->getId());

  auto quatsStore =
      DataStoreUtilities::CreateDataStore<float32>(td.ds, AnalyticalFixtures::k_CellDataPath.createChildPath(AnalyticalFixtures::k_QuatsName), tupleShape, {4}, IDataAction::Mode::Execute);
  td.quats = Float32Array::Create(td.ds, AnalyticalFixtures::k_QuatsName, quatsStore, td.cellAM->getId());

  auto crystalStructuresStore =
      DataStoreUtilities::CreateDataStore<uint32>(td.ds, AnalyticalFixtures::k_EnsembleDataPath.createChildPath(AnalyticalFixtures::k_CrystalStructuresName), {2}, {1}, IDataAction::Mode::Execute);
  td.crystalStructures = UInt32Array::Create(td.ds, AnalyticalFixtures::k_CrystalStructuresName, crystalStructuresStore, td.ensembleAM->getId());

  REQUIRE(td.featureIds != nullptr);
  REQUIRE(td.cellPhases != nullptr);
  REQUIRE(td.quats != nullptr);
  REQUIRE(td.crystalStructures != nullptr);
  return td;
}
} // namespace

// Analytical and invariant fixtures replace the archived KAM output because it
// is a circular oracle. The shared archive remains for other filter tests. See
// vv/provenance/ComputeKernelAvgMisorientationsFilter.md.

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientations: Working Set Plan", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  const DataPath geomPath({"ImageGeometry"});

  SECTION("normal kernel selects rolling window")
  {
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{200, 200, 200}, {1, 1, 1}, 1024ULL * 1024ULL * 1024ULL, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value().UseRollingWindow);
    REQUIRE(result.value().WindowSlices == 3);
    REQUIRE(result.value().RollingBytes == 3040000);
    REQUIRE(result.value().RollingBytes <= result.value().CapBytes);
  }

  SECTION("live cache use reduces the algorithm cap")
  {
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{1, 1, 1}, {0, 0, 0}, 1024, 768, geomPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value().CapBytes == 128);
  }

  SECTION("large window selects bounded blocks")
  {
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{64, 64, 5}, {0, 0, 4}, 1024ULL * 1024ULL, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE_FALSE(result.value().UseRollingWindow);
    REQUIRE(result.value().CapBytes == 262144);
    REQUIRE(result.value().RollingBytes == 507904);
    // Keep blocks aligned to complete X rows and provide eight fully
    // associative slots for the five active Z-row blocks. This prevents the
    // bounded fallback from evicting every input block for each focal tuple.
    REQUIRE(result.value().BlockTuples == 1280);
    REQUIRE(result.value().CacheSlots == 8);
  }

  SECTION("slice tuple overflow reports complete context")
  {
    constexpr uint64 k_Budget = 1024ULL * 1024ULL * 1024ULL;
    constexpr usize k_MaxSize = std::numeric_limits<usize>::max();
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{k_MaxSize, 2, 1}, {1, 1, 1}, k_Budget, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == -67200);
    const std::string expectedMessage =
        fmt::format("Compute Kernel Average Misorientations cannot size its working set for Image Geometry 'ImageGeometry' with dimensions (Z=1, Y=2, X={}) and kernel radius (1, 1, 1) "
                    "under cache budget 1073741824 bytes because the slice tuple count overflows.",
                    k_MaxSize);
    CHECK(result.errors()[0].message == expectedMessage);
  }
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientations: Working Set Input Validation", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  const DataPath geomPath({"ImageGeometry"});
  constexpr int32 k_InputError = -67202;

  SECTION("oversized kernel is rejected")
  {
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{4, 5, 6}, {1, 2, 3, 4}, 4096, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == k_InputError);
    CHECK(result.errors()[0].message ==
          "Compute Kernel Average Misorientations cannot size its working set for Image Geometry 'ImageGeometry' with dimensions (Z=6, Y=5, X=4) under cache budget 4096 bytes because kernel "
          "radius must contain exactly 3 values (X, Y, Z), but 4 values were provided: [1, 2, 3, 4].");
  }

  SECTION("negative kernel radius is rejected as invalid input")
  {
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{4, 5, 6}, {1, -2, 3}, 4096, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == k_InputError);
    CHECK(result.errors()[0].message ==
          "Compute Kernel Average Misorientations cannot size its working set for Image Geometry 'ImageGeometry' with dimensions (Z=6, Y=5, X=4) and kernel radius (1, -2, 3) under cache "
          "budget 4096 bytes because all kernel radii must be nonnegative.");
  }

  SECTION("INT32_MAX radii remain valid")
  {
    constexpr int32 k_MaxRadius = std::numeric_limits<int32>::max();
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{1, 1, 1}, {k_MaxRadius, k_MaxRadius, k_MaxRadius}, 1024, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value().CapBytes == 256);
    REQUIRE(result.value().SliceTuples == 1);
    REQUIRE(result.value().WindowSlices == 1);
    REQUIRE(result.value().RollingBytes == 28);
    REQUIRE(result.value().UseRollingWindow);
  }

  SECTION("short kernel is rejected before indexing")
  {
    const VectorInt32Parameter::ValueType kernelSize = {1, 2};
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{4, 5, 6}, kernelSize, 4096, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == k_InputError);
    CHECK(result.errors()[0].message ==
          "Compute Kernel Average Misorientations cannot size its working set for Image Geometry 'ImageGeometry' with dimensions (Z=6, Y=5, X=4) under cache budget 4096 bytes because kernel "
          "radius must contain exactly 3 values (X, Y, Z), but 2 values were provided: [1, 2].");
  }
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientations: Working Set Budget Boundaries", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  const DataPath geomPath({"ImageGeometry"});

  SECTION("zero budget uses irreducible fallback")
  {
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{1, 1, 1}, {0, 0, 0}, 0, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value().CapBytes == 0);
    REQUIRE(result.value().RollingBytes == 28);
    REQUIRE_FALSE(result.value().UseRollingWindow);
    REQUIRE(result.value().BlockTuples == 1);
    REQUIRE(result.value().CacheSlots == 1);
  }

  SECTION("cache use at or above budget leaves no policy cap")
  {
    auto fullyUsedResult = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{1, 1, 1}, {0, 0, 0}, 1024, 1024, geomPath);
    SIMPLNX_RESULT_REQUIRE_VALID(fullyUsedResult);
    REQUIRE(fullyUsedResult.value().CapBytes == 0);
    REQUIRE_FALSE(fullyUsedResult.value().UseRollingWindow);
    REQUIRE(fullyUsedResult.value().BlockTuples == 1);
    REQUIRE(fullyUsedResult.value().CacheSlots == 1);

    auto overusedResult = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{1, 1, 1}, {0, 0, 0}, 1024, 2048, geomPath);
    SIMPLNX_RESULT_REQUIRE_VALID(overusedResult);
    REQUIRE(overusedResult.value().CapBytes == 0);
    REQUIRE_FALSE(overusedResult.value().UseRollingWindow);
    REQUIRE(overusedResult.value().BlockTuples == 1);
    REQUIRE(overusedResult.value().CacheSlots == 1);
  }

  SECTION("cap below one tuple keeps fixed executor overhead")
  {
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{1, 1, 1}, {0, 0, 0}, 100, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value().CapBytes == 25);
    REQUIRE(result.value().RollingBytes == 28);
    REQUIRE_FALSE(result.value().UseRollingWindow);
    REQUIRE(result.value().BlockTuples == 1);
    REQUIRE(result.value().CacheSlots == 1);
  }

  SECTION("zero dimensions have an empty rolling working set")
  {
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{0, 0, 0}, {0, 0, 0}, 0, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value().CapBytes == 0);
    REQUIRE(result.value().SliceTuples == 0);
    REQUIRE(result.value().WindowSlices == 0);
    REQUIRE(result.value().RollingBytes == 0);
    REQUIRE(result.value().UseRollingWindow);
    REQUIRE(result.value().BlockTuples == 1);
    REQUIRE(result.value().CacheSlots == 1);
  }
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientations: Working Set Overflow Boundaries", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  const DataPath geomPath({"ImageGeometry"});
  constexpr int32 k_OverflowError = -67200;
  constexpr uint64 k_Budget = 4096;
  constexpr usize k_MaxSize = std::numeric_limits<usize>::max();
  constexpr uint64 k_MaxBytes = std::numeric_limits<uint64>::max();
  constexpr uint64 k_InputBytesPerTuple = 24;
  constexpr uint64 k_FocalAndOutputBytesPerTuple = 4;
  constexpr uint64 k_TotalRollingBytesPerTuple = k_InputBytesPerTuple + k_FocalAndOutputBytesPerTuple;

  constexpr bool k_CanRepresentInputByteOverflow = k_MaxSize > k_MaxBytes / k_InputBytesPerTuple;
  constexpr bool k_CanRepresentFocalByteOverflow = k_MaxSize > k_MaxBytes / k_FocalAndOutputBytesPerTuple;
  constexpr bool k_CanRepresentTotalRollingOverflow = k_MaxSize > k_MaxBytes / k_TotalRollingBytesPerTuple;
  constexpr uint64 k_MaxDimensionRollingBytes = !k_CanRepresentTotalRollingOverflow ? static_cast<uint64>(k_MaxSize) * k_TotalRollingBytesPerTuple : 0;
  constexpr uint64 k_MaxDimensionFocalBytes = !k_CanRepresentFocalByteOverflow ? static_cast<uint64>(k_MaxSize) * k_FocalAndOutputBytesPerTuple : 0;

  const auto expectedOverflowMessage = [](const SizeVec3& dimensions, const char* quantity) {
    return fmt::format(
        "Compute Kernel Average Misorientations cannot size its working set for Image Geometry 'ImageGeometry' with dimensions (Z={}, Y={}, X={}) and kernel radius (0, 0, 0) under cache budget "
        "4096 bytes because {} overflows.",
        dimensions[2], dimensions[1], dimensions[0], quantity);
  };

  SECTION("total tuple count overflow")
  {
    const SizeVec3 dimensions = {2, k_MaxSize / 2, 2};
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, {0, 0, 0}, k_Budget, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == k_OverflowError);
    CHECK(result.errors()[0].message == expectedOverflowMessage(dimensions, "the total tuple count"));
  }

  SECTION("total quaternion value count overflow")
  {
    const SizeVec3 dimensions = {1, 1, k_MaxSize};
    auto result = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, {0, 0, 0}, k_Budget, 0, geomPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == k_OverflowError);
    const std::string overflowedQuantity = fmt::format("the total quaternion value count ({} tuples * 4 components)", k_MaxSize);
    CHECK(result.errors()[0].message == expectedOverflowMessage(dimensions, overflowedQuantity.c_str()));
  }

  SECTION("rolling input byte count overflow")
  {
    if constexpr(k_CanRepresentInputByteOverflow)
    {
      const SizeVec3 dimensions = {static_cast<usize>(k_MaxBytes / k_InputBytesPerTuple + 1), 1, 1};
      auto result = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, {0, 0, 0}, k_Budget, 0, geomPath);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      REQUIRE(result.errors().size() == 1);
      REQUIRE(result.errors()[0].code == k_OverflowError);
      CHECK(result.errors()[0].message == expectedOverflowMessage(dimensions, "the rolling-window input byte count"));
    }
    else if constexpr(!k_CanRepresentTotalRollingOverflow)
    {
      const SizeVec3 dimensions = {k_MaxSize, 1, 1};
      auto result = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, {0, 0, 0}, k_Budget, 0, geomPath);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      REQUIRE(result.value().CapBytes == 1024);
      REQUIRE(result.value().SliceTuples == k_MaxSize);
      REQUIRE(result.value().WindowSlices == 1);
      REQUIRE(result.value().RollingBytes == k_MaxDimensionRollingBytes);
      REQUIRE(result.value().UseRollingWindow == (k_MaxDimensionRollingBytes <= result.value().CapBytes));
      REQUIRE(result.value().BlockTuples >= 1);
      REQUIRE(result.value().CacheSlots >= 1);
    }
    else
    {
      const SizeVec3 dimensions = {k_MaxSize, 1, 1};
      auto result = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, {0, 0, 0}, k_Budget, 0, geomPath);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      REQUIRE(result.errors().size() == 1);
      REQUIRE(result.errors()[0].code == k_OverflowError);
      CHECK(result.errors()[0].message == expectedOverflowMessage(dimensions, "the total rolling byte count"));
    }
  }

  SECTION("focal and output byte count overflow")
  {
    if constexpr(k_CanRepresentFocalByteOverflow)
    {
      const SizeVec3 dimensions = {static_cast<usize>(k_MaxBytes / k_FocalAndOutputBytesPerTuple + 1), 1, 0};
      auto result = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, {0, 0, 0}, k_Budget, 0, geomPath);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      REQUIRE(result.errors().size() == 1);
      REQUIRE(result.errors()[0].code == k_OverflowError);
      CHECK(result.errors()[0].message == expectedOverflowMessage(dimensions, "the focal/output byte count"));
    }
    else
    {
      const SizeVec3 dimensions = {k_MaxSize, 1, 0};
      auto result = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, {0, 0, 0}, k_Budget, 0, geomPath);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      REQUIRE(result.value().CapBytes == 1024);
      REQUIRE(result.value().SliceTuples == k_MaxSize);
      REQUIRE(result.value().WindowSlices == 0);
      REQUIRE(result.value().RollingBytes == k_MaxDimensionFocalBytes);
      REQUIRE(result.value().UseRollingWindow == (k_MaxDimensionFocalBytes <= result.value().CapBytes));
      REQUIRE(result.value().BlockTuples >= 1);
      REQUIRE(result.value().CacheSlots >= 1);
    }
  }

  SECTION("total rolling byte count overflow")
  {
    if constexpr(k_CanRepresentTotalRollingOverflow)
    {
      const SizeVec3 dimensions = {static_cast<usize>(k_MaxBytes / k_TotalRollingBytesPerTuple + 1), 1, 1};
      auto result = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, {0, 0, 0}, k_Budget, 0, geomPath);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      REQUIRE(result.errors().size() == 1);
      REQUIRE(result.errors()[0].code == k_OverflowError);
      CHECK(result.errors()[0].message == expectedOverflowMessage(dimensions, "the total rolling byte count"));
    }
    else
    {
      const SizeVec3 dimensions = {k_MaxSize, 1, 1};
      auto result = CreateComputeKernelAvgMisorientationsWorkingSet(dimensions, {0, 0, 0}, k_Budget, 0, geomPath);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      REQUIRE(result.value().CapBytes == 1024);
      REQUIRE(result.value().SliceTuples == k_MaxSize);
      REQUIRE(result.value().WindowSlices == 1);
      REQUIRE(result.value().RollingBytes == k_MaxDimensionRollingBytes);
      REQUIRE(result.value().UseRollingWindow == (k_MaxDimensionRollingBytes <= result.value().CapBytes));
      REQUIRE(result.value().BlockTuples >= 1);
      REQUIRE(result.value().CacheSlots >= 1);
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Scanline Handles Maximum Kernel Radius", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter][Scanline]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(1, 1, 1);
  constexpr int32 k_MaxRadius = std::numeric_limits<int32>::max();

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({k_MaxRadius, k_MaxRadius, k_MaxRadius});

  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const Float32Array* kam = nullptr;
  REQUIRE_NOTHROW(kam = &AnalyticalFixtures::GetOutputKAM(td.ds));
  REQUIRE(kam != nullptr);
  REQUIRE(kam->getNumberOfTuples() == 1);
  CHECK((*kam)[0] == 0.0f);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Scanline Handles Empty Image Geometry", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter][Scanline]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const std::array<SizeVec3, 3> emptyDimensions = {SizeVec3{0, 1, 1}, SizeVec3{1, 0, 1}, SizeVec3{1, 1, 0}};
  for(const SizeVec3& dimensions : emptyDimensions)
  {
    DYNAMIC_SECTION("dimensions = " << dimensions[0] << " x " << dimensions[1] << " x " << dimensions[2])
    {
      AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(dimensions[0], dimensions[1], dimensions[2]);

      ComputeKernelAvgMisorientationsFilter filter;
      Arguments args = AnalyticalFixtures::BuildArgs({1, 1, 1});

      auto preflightResult = filter.preflight(td.ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = scope.executeFilter(filter, td.ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      const Float32Array* kam = nullptr;
      REQUIRE_NOTHROW(kam = &AnalyticalFixtures::GetOutputKAM(td.ds));
      REQUIRE(kam != nullptr);
      CHECK(kam->getNumberOfTuples() == 0);
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeKernelAvgMisorientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeKernelAvgMisorientationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeKernelAvgMisorientationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Pipeline loading verifies IntVec3FilterParameterConverter.
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeKernelAvgMisorientationsFilter::k_KernelAverageMisorientationsArrayName_Key) == "TestName");
    }
  }
}

// Pure phi1 Bunge rotations give closed-form cubic misorientations. Differences
// at or below 45 degrees equal the symmetry-reduced angle in degrees.

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Uniform 2D Single Feature", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Identity orientations make every valid 2D kernel average zero. This checks
  // feature matching and boundary clamping.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(3, 3, 1);

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 1, 0});

  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  for(usize i = 0; i < td.totalCells; ++i)
  {
    REQUIRE(kam[i] == Approx(0.0f).margin(1e-4f));
  }

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - 1D x-axis Gradient", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // A 5-degree x gradient gives {2.5, 10/3, 10/3, 10/3, 2.5}. This checks
  // one-dimensional averaging and boundary clamping.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(5, 1, 1);
  const std::array<float32, 5> phi1Deg = {0.0f, 5.0f, 10.0f, 15.0f, 20.0f};
  for(usize i = 0; i < 5; ++i)
  {
    AnalyticalFixtures::SetCellQuat(td, i, AnalyticalFixtures::QuatFromPhi1Deg(phi1Deg[i]));
  }

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0});
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 5> expected = {2.5f, 10.0f / 3.0f, 10.0f / 3.0f, 10.0f / 3.0f, 2.5f};
  for(usize i = 0; i < 5; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - 1D z-axis Gradient (3D path)", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // A 10-degree z gradient gives {5, 20/3, 5}. This checks the outer z loop
  // and z-boundary clamping.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(1, 1, 3);
  const std::array<float32, 3> phi1Deg = {0.0f, 10.0f, 20.0f};
  for(usize i = 0; i < 3; ++i)
  {
    AnalyticalFixtures::SetCellQuat(td, i, AnalyticalFixtures::QuatFromPhi1Deg(phi1Deg[i]));
  }

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({0, 0, 1});
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 3> expected = {5.0f, 20.0f / 3.0f, 5.0f};
  for(usize i = 0; i < 3; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Multi-Feature Multi-Voxel + Background", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Mixed feature IDs verify same-feature accumulation, background zero, and
  // isolated-feature handling.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(6, 1, 1);
  (*td.featureIds)[0] = 1;
  (*td.featureIds)[1] = 1;
  (*td.featureIds)[2] = 2;
  (*td.featureIds)[3] = 2;
  (*td.featureIds)[4] = 0;
  (*td.featureIds)[5] = 1;
  (*td.cellPhases)[0] = 1;
  (*td.cellPhases)[1] = 1;
  (*td.cellPhases)[2] = 1;
  (*td.cellPhases)[3] = 1;
  (*td.cellPhases)[4] = 0;
  (*td.cellPhases)[5] = 1;
  AnalyticalFixtures::SetCellQuat(td, 0, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  AnalyticalFixtures::SetCellQuat(td, 1, AnalyticalFixtures::QuatFromPhi1Deg(10.0f));
  AnalyticalFixtures::SetCellQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  AnalyticalFixtures::SetCellQuat(td, 3, AnalyticalFixtures::QuatFromPhi1Deg(20.0f));
  AnalyticalFixtures::SetCellQuat(td, 5, AnalyticalFixtures::QuatFromPhi1Deg(30.0f));

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0});
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 6> expected = {5.0f, 5.0f, 10.0f, 10.0f, 0.0f, 0.0f};
  for(usize i = 0; i < 6; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }
  // The background short-circuit must write exact zero.
  REQUIRE(kam[4] == 0.0f);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Per-Voxel Mode (use_feature_ids = false)", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  // Per-voxel mode admits positive same-phase neighbors across feature
  // boundaries. Feature zero remains excluded.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(6, 1, 1);
  (*td.featureIds)[0] = 1;
  (*td.featureIds)[1] = 1;
  (*td.featureIds)[2] = 2;
  (*td.featureIds)[3] = 2;
  (*td.featureIds)[4] = 0;
  (*td.featureIds)[5] = 1;
  (*td.cellPhases)[0] = 1;
  (*td.cellPhases)[1] = 1;
  (*td.cellPhases)[2] = 1;
  (*td.cellPhases)[3] = 1;
  (*td.cellPhases)[4] = 0;
  (*td.cellPhases)[5] = 1;
  AnalyticalFixtures::SetCellQuat(td, 0, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  AnalyticalFixtures::SetCellQuat(td, 1, AnalyticalFixtures::QuatFromPhi1Deg(10.0f));
  AnalyticalFixtures::SetCellQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  AnalyticalFixtures::SetCellQuat(td, 3, AnalyticalFixtures::QuatFromPhi1Deg(20.0f));
  AnalyticalFixtures::SetCellQuat(td, 5, AnalyticalFixtures::QuatFromPhi1Deg(30.0f));

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0}, false);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 6> expected = {5.0f, 20.0f / 3.0f, 10.0f, 10.0f, 0.0f, 0.0f};
  for(usize i = 0; i < 6; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }
  REQUIRE(kam[4] == 0.0f);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Per-Voxel Mode Two-Phase Gates", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  // Two cubic phases verify the per-voxel phase and feature-ID gates.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(5, 1, 1, 3);
  (*td.crystalStructures)[2] = 1u;
  const std::array<int32, 5> featureIds = {1, 2, 3, 4, 0};
  const std::array<int32, 5> phases = {1, 1, 2, 1, 1};
  const std::array<float32, 5> phi1Deg = {0.0f, 10.0f, 20.0f, 30.0f, 40.0f};
  for(usize i = 0; i < 5; ++i)
  {
    (*td.featureIds)[i] = featureIds[i];
    (*td.cellPhases)[i] = phases[i];
    AnalyticalFixtures::SetCellQuat(td, i, AnalyticalFixtures::QuatFromPhi1Deg(phi1Deg[i]));
  }

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0}, false);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 5> expected = {5.0f, 5.0f, 0.0f, 0.0f, 0.0f};
  for(usize i = 0; i < 5; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }
  // An invalid focal cell must receive exact zero.
  REQUIRE(kam[4] == 0.0f);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 4 - Mode Equivalence on Single Feature", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  // A single-feature, single-phase gradient gives both modes the same neighbor
  // set. Compare the full three-dimensional outputs bit-for-bit.
  auto buildFixture = []() {
    AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(3, 3, 3);
    for(usize z = 0; z < 3; ++z)
    {
      for(usize y = 0; y < 3; ++y)
      {
        for(usize x = 0; x < 3; ++x)
        {
          const usize idx = (z * 9) + (y * 3) + x;
          const auto phi1 = static_cast<float32>(2 * x + 3 * y + 4 * z);
          AnalyticalFixtures::SetCellQuat(td, idx, AnalyticalFixtures::QuatFromPhi1Deg(phi1));
        }
      }
    }
    return td;
  };

  AnalyticalFixtures::FixtureData tdPerGrain = buildFixture();
  AnalyticalFixtures::FixtureData tdPerVoxel = buildFixture();

  ComputeKernelAvgMisorientationsFilter filter;
  auto runFilter = [&filter, &scope](AnalyticalFixtures::FixtureData& td, bool useFeatureIds) {
    Arguments args = AnalyticalFixtures::BuildArgs({1, 1, 1}, useFeatureIds);
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };
  runFilter(tdPerGrain, true);
  runFilter(tdPerVoxel, false);

  const auto& kamPerGrain = AnalyticalFixtures::GetOutputKAM(tdPerGrain.ds);
  const auto& kamPerVoxel = AnalyticalFixtures::GetOutputKAM(tdPerVoxel.ds);
  bool anyNonzero = false;
  for(usize i = 0; i < tdPerGrain.totalCells; ++i)
  {
    REQUIRE(kamPerGrain[i] == kamPerVoxel[i]);
    if(kamPerGrain[i] > 1e-4f)
    {
      anyNonzero = true;
    }
  }
  // Require nonzero output so an all-zero result cannot pass the equality check.
  REQUIRE(anyNonzero);

  UnitTest::CheckArraysInheritTupleDims(tdPerGrain.ds);
  UnitTest::CheckArraysInheritTupleDims(tdPerVoxel.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 4 - Invariants", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Class 4 checks zero for uniform and background cells, nonnegative output,
  // the 62.8-degree cubic bound, and nonzero output for a gradient.
  constexpr float32 k_CubicMaxAngleDeg = 62.8f;

  SECTION("(i) Uniform-orientation single-feature => KAM == 0")
  {
    AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(3, 3, 3);
    ComputeKernelAvgMisorientationsFilter filter;
    Arguments args = AnalyticalFixtures::BuildArgs({1, 1, 1});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
    for(usize i = 0; i < td.totalCells; ++i)
    {
      REQUIRE(kam[i] == Approx(0.0f).margin(1e-4f));
    }
  }

  SECTION("(ii) Background cell => KAM == 0 exactly")
  {
    // The middle cell is background.
    AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(3, 1, 1);
    (*td.featureIds)[1] = 0;
    (*td.cellPhases)[1] = 0;
    AnalyticalFixtures::SetCellQuat(td, 0, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
    AnalyticalFixtures::SetCellQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(10.0f));
    ComputeKernelAvgMisorientationsFilter filter;
    Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
    REQUIRE(kam[1] == 0.0f);
  }

  SECTION("(iii, iv, v) Range and non-triviality invariants on x-axis gradient")
  {
    AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(5, 1, 1);
    const std::array<float32, 5> phi1Deg = {0.0f, 5.0f, 10.0f, 15.0f, 20.0f};
    for(usize i = 0; i < 5; ++i)
    {
      AnalyticalFixtures::SetCellQuat(td, i, AnalyticalFixtures::QuatFromPhi1Deg(phi1Deg[i]));
    }
    ComputeKernelAvgMisorientationsFilter filter;
    Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
    for(usize i = 0; i < td.totalCells; ++i)
    {
      REQUIRE(kam[i] >= 0.0f);
      REQUIRE(kam[i] <= k_CubicMaxAngleDeg);
    }
    // Require nonzero output from the gradient fixture.
    bool anyNonzero = false;
    for(usize i = 0; i < td.totalCells; ++i)
    {
      if(kam[i] > 1e-4f)
      {
        anyNonzero = true;
      }
    }
    REQUIRE(anyNonzero);
  }
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Bounded Cache Full-Depth Kernel", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  // The cache budget forces the bounded block-cache traversal for a full-depth
  // kernel.
  constexpr usize k_X = 64;
  constexpr usize k_Y = 64;
  constexpr usize k_Z = 5;
  constexpr usize k_SliceTuples = k_X * k_Y;
  constexpr uint64 k_CacheBudget = 1024ULL * 1024ULL;
  constexpr int64 k_BytesPerQuatSlice = static_cast<int64>(k_SliceTuples * 4 * sizeof(float32));

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const CacheMemoryBudgetSentinel budget(k_CacheBudget);

  auto td = CreateOocFixture(k_X, k_Y, k_Z);
  scope.requireExpectedStore(*td.featureIds);
  scope.requireExpectedStore(*td.cellPhases);
  scope.requireExpectedStore(*td.quats);

  const std::vector<int32> ids(k_SliceTuples, 1);
  const std::vector<int32> phases(k_SliceTuples, 1);
  std::vector<float32> quats(k_SliceTuples * 4);
  const std::array<float32, k_Z> phi1Deg = {0.0F, 5.0F, 10.0F, 15.0F, 20.0F};

  for(usize z = 0; z < k_Z; z++)
  {
    const auto q = AnalyticalFixtures::QuatFromPhi1Deg(phi1Deg[z]);
    for(usize tuple = 0; tuple < k_SliceTuples; tuple++)
    {
      std::copy(q.begin(), q.end(), quats.begin() + tuple * 4);
    }

    const usize tupleOffset = z * k_SliceTuples;
    SIMPLNX_RESULT_REQUIRE_VALID(td.featureIds->getDataStoreRef().copyFromBuffer(tupleOffset, nonstd::span<const int32>(ids.data(), ids.size())));
    SIMPLNX_RESULT_REQUIRE_VALID(td.cellPhases->getDataStoreRef().copyFromBuffer(tupleOffset, nonstd::span<const int32>(phases.data(), phases.size())));
    SIMPLNX_RESULT_REQUIRE_VALID(td.quats->getDataStoreRef().copyFromBuffer(tupleOffset * 4, nonstd::span<const float32>(quats.data(), quats.size())));
  }

  const std::array<uint32, 2> structures = {ebsdlib::CrystalStructure::UnknownCrystalStructure, ebsdlib::CrystalStructure::Cubic_High};
  SIMPLNX_RESULT_REQUIRE_VALID(td.crystalStructures->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(structures.data(), structures.size())));

  auto planResult =
      CreateComputeKernelAvgMisorientationsWorkingSet(SizeVec3{k_X, k_Y, k_Z}, {0, 0, 4}, k_CacheBudget, CacheMemoryBudgetManager::instance().usedBytes(), AnalyticalFixtures::k_ImageGeomPath);
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);
  REQUIRE_FALSE(planResult.value().UseRollingWindow);

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({0, 0, 4});
  auto result = scope.executeFilter(filter, td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  const std::array<float32, 5> expected = {10.0F, 7.0F, 6.0F, 7.0F, 10.0F};
  const auto& output = AnalyticalFixtures::GetOutputKAM(td.ds);
  scope.requireExpectedStore(output);
  std::vector<float32> outputSlice(k_SliceTuples);
  for(usize z = 0; z < k_Z; z++)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(output.getDataStoreRef().copyIntoBuffer(z * k_SliceTuples, nonstd::span<float32>(outputSlice.data(), outputSlice.size())));
    for(const float32 value : outputSlice)
    {
      REQUIRE(value == Approx(expected[z]).margin(1.0e-3F));
    }
  }
}
