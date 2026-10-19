#include "SimplnxCore/Filters/Algorithms/ComputeFeatureCentroids.hpp"
#include "SimplnxCore/Filters/ComputeFeatureCentroidsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

// =============================================================================
// V&V Class 1 (Analytical) + Class 4 (Invariant) oracle support — added 2026-07-07.
//
// The centroid of a feature is the arithmetic mean of the voxel-center coordinates
// (voxel-center = origin + (index + 0.5) * spacing) of every cell belonging to that
// feature. This is a closed-form Class 1 oracle: expected values are hand-derivable on
// a toy grid, independently of any DREAM3D implementation. These inline fixtures replace
// the retired consistency-with-self exemplar test (Centroids NX vs a sibling Centroids
// array in 6_6_stats_test_v2.dream3d), which was a circular oracle.
//
// Reference: src/Plugins/SimplnxCore/vv/ComputeFeatureCentroidsFilter.md
// =============================================================================

namespace CentroidToy
{
const std::string k_GeomName = "Image";
const std::string k_CellAMName = "CellData";
const std::string k_FeatureAMName = "FeatureData";
const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_CentroidsName = "Centroids";

struct Scaffold
{
  DataStructure ds;
  DataPath geomPath;
  DataPath featureIdsPath;
  DataPath featureAMPath;
  DataPath centroidsPath;
};

class CentroidsFailOnLaterReadStore : public DataStore<int32>
{
public:
  CentroidsFailOnLaterReadStore(const ShapeType& tupleShape, int32 errorCode)
  : DataStore<int32>(tupleShape, ShapeType{1}, int32{0})
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyIntoBuffer(usize offset, nonstd::span<int32> buffer) const override
  {
    if(++m_ReadCount == 2)
    {
      return MakeErrorResult(m_ErrorCode, "Injected ComputeFeatureCentroids later-page read failure");
    }
    return DataStore<int32>::copyIntoBuffer(offset, buffer);
  }

private:
  int32 m_ErrorCode;
  mutable usize m_ReadCount = 0;
};

class CentroidsObservedReadStore : public DataStore<int32>
{
public:
  CentroidsObservedReadStore(const ShapeType& tupleShape, usize failOnRead = 0, std::atomic_bool* cancelAfterRead = nullptr, usize cancelOnRead = 0, usize warnOnRead = 0)
  : DataStore<int32>(tupleShape, ShapeType{1}, int32{0})
  , m_FailOnRead(failOnRead)
  , m_CancelAfterRead(cancelAfterRead)
  , m_CancelOnRead(cancelOnRead)
  , m_WarnOnRead(warnOnRead)
  {
  }

  Result<> copyIntoBuffer(usize offset, nonstd::span<int32> buffer) const override
  {
    m_ReadCount++;
    m_MaxWorkingReserved = std::max(m_MaxWorkingReserved, CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes());
    if(m_ReadCount == m_FailOnRead)
    {
      return MakeErrorResult(-91912, "Injected periodic centroid read failure");
    }
    auto result = DataStore<int32>::copyIntoBuffer(offset, buffer);
    if(m_ReadCount == m_WarnOnRead)
    {
      result.warnings().push_back(Warning{-91913, "Injected periodic centroid read warning"});
    }
    if(m_CancelAfterRead != nullptr && m_ReadCount == m_CancelOnRead)
    {
      *m_CancelAfterRead = true;
    }
    return result;
  }

  usize readCount() const
  {
    return m_ReadCount;
  }

  uint64 maxWorkingReserved() const
  {
    return m_MaxWorkingReserved;
  }

private:
  usize m_FailOnRead = 0;
  std::atomic_bool* m_CancelAfterRead = nullptr;
  usize m_CancelOnRead = 0;
  usize m_WarnOnRead = 0;
  mutable usize m_ReadCount = 0;
  mutable uint64 m_MaxWorkingReserved = 0;
};

class CentroidsBudgetSentinel
{
public:
  explicit CentroidsBudgetSentinel(uint64 budgetBytes)
  : m_Manager(CacheMemoryBudgetManager::instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  , m_PreviousReserved(m_Manager.reservedWorkingMemoryBytes())
  {
    m_Manager.setBudgetBytes(budgetBytes);
  }

  ~CentroidsBudgetSentinel()
  {
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }

  void requireNoReservationLeak() const
  {
    REQUIRE(m_Manager.reservedWorkingMemoryBytes() == m_PreviousReserved);
  }

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget = 0;
  uint64 m_PreviousReserved = 0;
};

// Build an ImageGeom + Cell Data AM (with FeatureIds) + empty Feature Data AM sized to numFeatures.
// featureIds are supplied in row-major (z, y, x) order.
inline Scaffold Build(usize dimX, usize dimY, usize dimZ, std::array<float32, 3> spacing, std::array<float32, 3> origin, usize numFeatures, const std::vector<int32>& featureIdValues)
{
  Scaffold s;
  auto* imageGeom = ImageGeom::Create(s.ds, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({spacing[0], spacing[1], spacing[2]});
  imageGeom->setOrigin({origin[0], origin[1], origin[2]});

  const ShapeType cellTupleShape{dimZ, dimY, dimX};
  auto* cellAM = AttributeMatrix::Create(s.ds, k_CellAMName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto* featureIds = CreateTestDataArray<int32>(s.ds, k_FeatureIdsName, cellTupleShape, {1}, cellAM->getId());
  REQUIRE(featureIdValues.size() == dimX * dimY * dimZ);
  for(usize i = 0; i < featureIdValues.size(); ++i)
  {
    (*featureIds)[i] = featureIdValues[i];
  }

  AttributeMatrix::Create(s.ds, k_FeatureAMName, ShapeType{numFeatures}, imageGeom->getId());

  s.geomPath = DataPath({k_GeomName});
  s.featureIdsPath = s.geomPath.createChildPath(k_CellAMName).createChildPath(k_FeatureIdsName);
  s.featureAMPath = s.geomPath.createChildPath(k_FeatureAMName);
  s.centroidsPath = s.featureAMPath.createChildPath(k_CentroidsName);
  return s;
}

inline Arguments MakeArguments(const Scaffold& s, bool isPeriodic)
{
  Arguments args;
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(s.geomPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(s.featureIdsPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(s.featureAMPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_IsPeriodic_Key, std::make_any<bool>(isPeriodic));
  return args;
}

inline std::shared_ptr<CentroidsObservedReadStore> ObserveIds(Scaffold& s, const std::vector<int32>& values, usize failOnRead = 0, std::atomic_bool* cancelAfterRead = nullptr, usize cancelOnRead = 0,
                                                              usize warnOnRead = 0)
{
  auto observed = std::make_shared<CentroidsObservedReadStore>(ShapeType{values.size()}, failOnRead, cancelAfterRead, cancelOnRead, warnOnRead);
  for(usize index = 0; index < values.size(); index++)
  {
    (*observed)[index] = values[index];
  }
  REQUIRE_NOTHROW(s.ds.getDataRefAs<Int32Array>(s.featureIdsPath));
  auto& array = s.ds.getDataRefAs<Int32Array>(s.featureIdsPath);
  auto result = array.setDataStore(observed);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  return observed;
}

constexpr float32 k_UnpublishedSentinel = -123.25F;

inline void PrepareSentinelOutput(Scaffold& s, usize featureCount)
{
  REQUIRE_NOTHROW(s.ds.getDataRefAs<AttributeMatrix>(s.featureAMPath));
  const auto& featureAM = s.ds.getDataRefAs<AttributeMatrix>(s.featureAMPath);
  auto* output = CreateTestDataArray<float32>(s.ds, k_CentroidsName, ShapeType{featureCount}, ShapeType{3}, featureAM.getId());
  REQUIRE(output != nullptr);
  for(usize index = 0; index < output->getSize(); index++)
  {
    (*output)[index] = k_UnpublishedSentinel;
  }
}

inline Result<> RunDirect(Scaffold& s, const std::atomic_bool& cancel)
{
  ComputeFeatureCentroidsInputValues inputs;
  inputs.FeatureIdsArrayPath = s.featureIdsPath;
  inputs.CentroidsArrayPath = s.centroidsPath;
  inputs.ImageGeometryPath = s.geomPath;
  inputs.FeatureAttributeMatrixPath = s.featureAMPath;
  inputs.IsPeriodic = true;
  const IFilter::MessageHandler handler{};
  return ComputeFeatureCentroids(s.ds, handler, cancel, &inputs)();
}

inline void RequireUnpublished(const Scaffold& s)
{
  REQUIRE_NOTHROW(s.ds.getDataRefAs<Float32Array>(s.centroidsPath));
  const auto& output = s.ds.getDataRefAs<Float32Array>(s.centroidsPath);
  for(usize index = 0; index < output.getSize(); index++)
  {
    REQUIRE(output[index] == k_UnpublishedSentinel);
  }
}

inline std::vector<int32> PopulationIds(const std::vector<usize>& populations, usize rows)
{
  std::vector<int32> ids(populations.size() * rows, 2);
  for(usize y = 0; y < rows; y++)
  {
    for(usize x = 0; x < populations.size(); x++)
    {
      if(y < populations[x])
      {
        ids[y * populations.size() + x] = 1;
      }
    }
  }
  return ids;
}

// Independent test oracle: sort occupied integer coordinates to find the seam,
// then average every original voxel coordinate so repeated cells retain weight.
inline std::vector<float32> SortedCoordinateOracle(const std::array<usize, 3>& dims, const std::vector<int32>& ids, usize features, const std::array<float32, 3>& origin,
                                                   const std::array<float32, 3>& spacing, bool periodic)
{
  std::vector<float32> expected(features * 3, 0.0F);
  for(usize feature = 0; feature < features; feature++)
  {
    for(usize axis = 0; axis < 3; axis++)
    {
      std::vector<usize> weightedCoordinates;
      for(usize index = 0; index < ids.size(); index++)
      {
        if(ids[index] != static_cast<int32>(feature))
        {
          continue;
        }
        const std::array<usize, 3> q = {index % dims[0], (index / dims[0]) % dims[1], index / (dims[0] * dims[1])};
        weightedCoordinates.push_back(q[axis]);
      }
      if(weightedCoordinates.empty())
      {
        continue;
      }

      std::vector<usize> occupied = weightedCoordinates;
      std::sort(occupied.begin(), occupied.end());
      occupied.erase(std::unique(occupied.begin(), occupied.end()), occupied.end());
      bool adjust = false;
      usize cut = 0;
      if(periodic && dims[axis] > 1 && occupied.front() == 0 && occupied.back() == dims[axis] - 1)
      {
        usize largestGap = 0;
        bool tied = false;
        for(usize index = 0; index < occupied.size(); index++)
        {
          const usize next = index + 1 == occupied.size() ? occupied.front() + dims[axis] : occupied[index + 1];
          const usize gap = next - occupied[index];
          if(gap > largestGap)
          {
            largestGap = gap;
            cut = occupied[index];
            tied = false;
          }
          else if(gap == largestGap)
          {
            tied = true;
          }
        }
        adjust = largestGap > 1 && !tied;
      }

      double sum = 0.0;
      for(usize q : weightedCoordinates)
      {
        sum += static_cast<double>(q) + 0.5 + (adjust && q <= cut ? static_cast<double>(dims[axis]) : 0.0);
      }
      double indexMean = sum / static_cast<double>(weightedCoordinates.size());
      if(adjust)
      {
        indexMean = std::fmod(indexMean, static_cast<double>(dims[axis]));
      }
      expected[feature * 3 + axis] = static_cast<float32>(static_cast<double>(origin[axis]) + static_cast<double>(spacing[axis]) * indexMean);
    }
  }
  return expected;
}

// Run the filter and return the flat [numFeatures * 3] Centroids values.
inline std::vector<float32> Run(Scaffold& s, bool isPeriodic)
{
  ComputeFeatureCentroidsFilter filter;
  Arguments args = MakeArguments(s, isPeriodic);

  auto preflightResult = filter.preflight(s.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(s.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE_NOTHROW(s.ds.getDataRefAs<Float32Array>(s.centroidsPath));
  const auto& centroids = s.ds.getDataRefAs<Float32Array>(s.centroidsPath);
  std::vector<float32> out(centroids.getSize());
  for(usize i = 0; i < centroids.getSize(); ++i)
  {
    out[i] = centroids[i];
  }
  return out;
}

inline void RequireCentroid(const std::vector<float32>& c, usize featureId, float32 x, float32 y, float32 z, float32 margin = 1.0e-4f)
{
  REQUIRE(c[featureId * 3 + 0] == Approx(x).margin(margin));
  REQUIRE(c[featureId * 3 + 1] == Approx(y).margin(margin));
  REQUIRE(c[featureId * 3 + 2] == Approx(z).margin(margin));
}
} // namespace CentroidToy

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: real HDF5 65536-block tail oracle", "[SimplnxCore][ComputeFeatureCentroidsFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const bool periodic = GENERATE(false, true);
  CAPTURE(periodic);
  const PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  constexpr usize k_BlockTuples = 65536;
  std::vector<int32> featureIds(k_BlockTuples + 1, 0);
  featureIds[k_BlockTuples - 1] = 1;
  featureIds[k_BlockTuples] = 1;
  auto fixture = CentroidToy::Build(k_BlockTuples + 1, 1, 1, {2.0F, 3.0F, 4.0F}, {10.0F, -2.0F, 7.0F}, 3, featureIds);
  REQUIRE_NOTHROW(fixture.ds.getDataRefAs<Int32Array>(fixture.featureIdsPath));
  auto& idsArray = fixture.ds.getDataRefAs<Int32Array>(fixture.featureIdsPath);
  auto store = DataStoreUtilities::ConvertDataStore<int32>(idsArray.getDataStoreRef(), "HDF5-OOC");
  REQUIRE(store != nullptr);
  auto replaceResult = idsArray.setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  REQUIRE(idsArray.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  const auto centroids = CentroidToy::Run(fixture, periodic);
  REQUIRE_NOTHROW(fixture.ds.getDataRefAs<Float32Array>(fixture.centroidsPath));
  REQUIRE(fixture.ds.getDataRefAs<Float32Array>(fixture.centroidsPath).getDataStoreRef().getDataFormat() == "HDF5-OOC");
  // The two feature-one centers are x=131081 and x=131083. Neither feature wraps.
  const std::vector<float32> expected = {65545.0F, -0.5F, 9.0F, 131082.0F, -0.5F, 9.0F, 0.0F, 0.0F, 0.0F};
  REQUIRE(centroids == expected);
  UnitTest::CheckArraysInheritTupleDims(fixture.ds);
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: real HDF5 wrapped weighted oracle", "[SimplnxCore][ComputeFeatureCentroidsFilter][.OocStoreContract]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  const std::vector<int32> ids = {1, 2, 2, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2};
  auto fixture = Build(5, 3, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
  REQUIRE_NOTHROW(fixture.ds.getDataRefAs<Int32Array>(fixture.featureIdsPath));
  auto& idsArray = fixture.ds.getDataRefAs<Int32Array>(fixture.featureIdsPath);
  auto store = DataStoreUtilities::ConvertDataStore<int32>(idsArray.getDataStoreRef(), "HDF5-OOC");
  REQUIRE(store != nullptr);
  auto replaceResult = idsArray.setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  REQUIRE(idsArray.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  const auto actual = Run(fixture, true);
  REQUIRE_NOTHROW(fixture.ds.getDataRefAs<Float32Array>(fixture.centroidsPath));
  REQUIRE(fixture.ds.getDataRefAs<Float32Array>(fixture.centroidsPath).getDataStoreRef().getDataFormat() == "HDF5-OOC");
  RequireCentroid(actual, 1, 4.7F, 0.9F, 0.5F);
  UnitTest::CheckArraysInheritTupleDims(fixture.ds);

  if(std::getenv("SIMPLNX_CENTROID_TIMING") != nullptr)
  {
    constexpr uint64 k_MiB = 1024ULL * 1024ULL;
    constexpr usize k_DimX = 32768;
    constexpr usize k_DimY = 16;
    constexpr uint64 k_UsefulBytes = k_DimY * (k_DimX / 8) + sizeof(uint64);
    std::vector<int32> timingIds(k_DimX * k_DimY, static_cast<int32>(k_DimY + 1));
    std::vector<float32> expected((k_DimY + 2) * 3, 0.0F);
    for(usize y = 0; y < k_DimY; y++)
    {
      timingIds[y * k_DimX] = static_cast<int32>(y + 1);
      timingIds[y * k_DimX + k_DimX - 1] = static_cast<int32>(y + 1);
      expected[(y + 1) * 3 + 1] = static_cast<float32>(y) + 0.5F;
      expected[(y + 1) * 3 + 2] = 0.5F;
    }
    expected[(k_DimY + 1) * 3] = static_cast<float32>(k_DimX) / 2.0F;
    expected[(k_DimY + 1) * 3 + 1] = static_cast<float32>(k_DimY) / 2.0F;
    expected[(k_DimY + 1) * 3 + 2] = 0.5F;

    CentroidsBudgetSentinel budget(256 * k_MiB);
    auto& manager = CacheMemoryBudgetManager::instance();
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    for(const uint64 headroom : std::array<uint64, 5>{uint64{8} * k_MiB, uint64{32} * k_MiB, uint64{64} * k_MiB, uint64{64} * 1024ULL, uint64{8} * 1024ULL})
    {
      // Populate in memory so setup does not perform one HDF5 write per voxel.
      auto timingFixture = [&]() {
        const PreferencesSentinel buildResident(DataStorageMode::ForceInCore, 0);
        return Build(k_DimX, k_DimY, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, k_DimY + 2, timingIds);
      }();
      auto& timingIdsArray = timingFixture.ds.getDataRefAs<Int32Array>(timingFixture.featureIdsPath);
      REQUIRE(timingIdsArray.getDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
      auto hdf5Store = DataStoreUtilities::ConvertDataStore<int32>(timingIdsArray.getDataStoreRef(), "HDF5-OOC");
      REQUIRE(hdf5Store != nullptr);
      auto replaceTimingStoreResult = timingIdsArray.setDataStore(hdf5Store);
      SIMPLNX_RESULT_REQUIRE_VALID(replaceTimingStoreResult);
      REQUIRE(timingIdsArray.getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
      REQUIRE(timingIdsArray.getDataStoreRef().getDataFormat() == "HDF5-OOC");

      const uint64 pressureBytes = 64 * k_MiB - headroom;
      auto pressureReservation = manager.reserveWorkingMemory(pressureBytes);
      REQUIRE(pressureReservation.sizeBytes() == pressureBytes);
      std::vector<uint8> pressureBuffer(static_cast<usize>(pressureBytes), uint8{0});
      const auto started = std::chrono::steady_clock::now();
      const auto timedActual = Run(timingFixture, true);
      const auto elapsedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
      REQUIRE(timingFixture.ds.getDataRefAs<Float32Array>(timingFixture.centroidsPath).getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
      REQUIRE(timingFixture.ds.getDataRefAs<Float32Array>(timingFixture.centroidsPath).getDataStoreRef().getDataFormat() == "HDF5-OOC");
      REQUIRE(timedActual.size() == expected.size());
      for(usize index = 0; index < expected.size(); index++)
      {
        CAPTURE(headroom, index);
        REQUIRE(timedActual[index] == Approx(expected[index]).margin(2.0e-4F));
      }
      REQUIRE(manager.reservedWorkingMemoryBytes() == pressureBytes);
      const usize plannedOccupancyScans = headroom >= k_UsefulBytes ? 1 : (headroom >= 64 * 1024ULL ? 2 : 9);
      std::cout << "CENTROID_HDF5_TIMING headroom=" << headroom << " useful=" << k_UsefulBytes << " planned_occupancy_scans=" << plannedOccupancyScans << " elapsed_ms=" << elapsedMs << '\n';
    }
    budget.requireNoReservationLeak();
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: Class 1 - Analytical Centroids", "[SimplnxCore][ComputeFeatureCentroidsFilter]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();

  SECTION("Fixture A - single multi-cell feature + empty background")
  {
    // 3x1x1, spacing 1, origin 0; FeatureIds [1,1,1]; 2 features (0 empty, 1 present)
    auto s = Build(3, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 2, {1, 1, 1});
    auto c = Run(s, false);
    RequireCentroid(c, 0, 0.0f, 0.0f, 0.0f); // count==0 -> stays (0,0,0)
    RequireCentroid(c, 1, 1.5f, 0.5f, 0.5f); // x=(0.5+1.5+2.5)/3
  }

  SECTION("Fixture B - multi-feature 2D (averaging, single-cell, empty id)")
  {
    // 4x2x1; row y0 [1,1,2,2], row y1 [1,3,2,2]; 4 features
    auto s = Build(4, 2, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 4, {1, 1, 2, 2, 1, 3, 2, 2});
    auto c = Run(s, false);
    RequireCentroid(c, 0, 0.0f, 0.0f, 0.0f);               // empty
    RequireCentroid(c, 1, 5.0f / 6.0f, 5.0f / 6.0f, 0.5f); // (0.8333.., 0.8333.., 0.5)
    RequireCentroid(c, 2, 3.0f, 1.0f, 0.5f);
    RequireCentroid(c, 3, 1.5f, 1.5f, 0.5f); // single cell (1,1)
  }

  SECTION("Fixture C - 3D volume (z-stride correctness)")
  {
    // 2x2x2; z0 plane all fid1, z1 plane all fid2; 3 features
    auto s = Build(2, 2, 2, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 3, {1, 1, 1, 1, 2, 2, 2, 2});
    auto c = Run(s, false);
    RequireCentroid(c, 0, 0.0f, 0.0f, 0.0f);
    RequireCentroid(c, 1, 1.0f, 1.0f, 0.5f); // z index 0
    RequireCentroid(c, 2, 1.0f, 1.0f, 1.5f); // z index 1
  }

  UnitTest::CheckArraysInheritTupleDims(Build(3, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 2, {1, 1, 1}).ds);
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: Class 1/4 - Periodic Boundary", "[SimplnxCore][ComputeFeatureCentroidsFilter][Periodic]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();

  SECTION("weighted asymmetric boundary uses largest-gap unwrapped arithmetic")
  {
    // In a 5x3x1 image, feature 1 occupies x centers 0.5,0.5,3.5,4.5,4.5.
    // The unique largest gap is between 0.5 and 3.5; wrapping x=0 forward gives 4.7.
    const std::vector<int32> ids = {1, 2, 2, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2};
    auto ordinaryFixture = Build(5, 3, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto ordinary = Run(ordinaryFixture, false);
    RequireCentroid(ordinary, 1, 2.7F, 0.9F, 0.5F);

    auto periodicFixture = Build(5, 3, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto periodic = Run(periodicFixture, true);
    RequireCentroid(periodic, 1, 4.7F, 0.9F, 0.5F);
    RequireCentroid(periodic, 0, 0.0F, 0.0F, 0.0F);
    UnitTest::CheckArraysInheritTupleDims(periodicFixture.ds);
  }

  SECTION("the asymmetric boundary correction works on each axis")
  {
    for(usize targetAxis = 0; targetAxis < 3; targetAxis++)
    {
      const usize secondaryAxis = (targetAxis + 1) % 3;
      std::array<usize, 3> dims = {1, 1, 1};
      dims[targetAxis] = 5;
      dims[secondaryAxis] = 3;
      std::vector<int32> ids(dims[0] * dims[1] * dims[2], 2);
      for(usize z = 0; z < dims[2]; z++)
      {
        for(usize y = 0; y < dims[1]; y++)
        {
          for(usize x = 0; x < dims[0]; x++)
          {
            const std::array<usize, 3> q = {x, y, z};
            const bool belongs = (q[targetAxis] == 0 || q[targetAxis] == 4) ? q[secondaryAxis] < 2 : q[targetAxis] == 3 && q[secondaryAxis] == 0;
            if(belongs)
            {
              ids[(z * dims[1] + y) * dims[0] + x] = 1;
            }
          }
        }
      }
      auto fixture = Build(dims[0], dims[1], dims[2], {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
      auto actual = Run(fixture, true);
      std::array<float32, 3> expected = {0.5F, 0.5F, 0.5F};
      expected[targetAxis] = 4.7F;
      expected[secondaryAxis] = 0.9F;
      RequireCentroid(actual, 1, expected[0], expected[1], expected[2]);
    }
  }

  SECTION("origin and signed nonunit spacing map the wrapped index mean")
  {
    const std::vector<int32> ids = {1, 2, 2, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2};
    auto fixture = Build(5, 3, 1, {-2.0F, 3.0F, 0.0F}, {10.0F, -4.0F, 7.0F}, 3, ids);
    auto actual = Run(fixture, true);
    RequireCentroid(actual, 1, 0.6F, -1.3F, 7.0F);
  }

  SECTION("circular-anchor recentering is different from largest-gap arithmetic")
  {
    const auto ids = PopulationIds({1, 0, 1, 0, 3, 0, 0, 1}, 3);
    auto fixture = Build(8, 3, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto actual = Run(fixture, true);
    REQUIRE(actual[3] == Approx(8.0F / 3.0F).margin(1.0e-4F));
  }

  SECTION("complete axis occupancy retains the weighted ordinary mean")
  {
    const auto ids = PopulationIds({3, 1, 1, 1, 1}, 3);
    auto fixture = Build(5, 3, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto actual = Run(fixture, true);
    REQUIRE(actual[3] == Approx(13.5F / 7.0F).margin(1.0e-4F));
  }

  SECTION("tied largest gaps retain the weighted ordinary mean")
  {
    const auto ids = PopulationIds({2, 0, 0, 1, 1, 0, 0, 1}, 2);
    auto fixture = Build(8, 2, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto actual = Run(fixture, true);
    REQUIRE(actual[3] == Approx(3.3F).margin(1.0e-4F));
  }

  SECTION("a feature can wrap on all three axes")
  {
    std::vector<int32> ids(27, 2);
    ids[0] = 1;
    ids[26] = 1;
    auto fixture = Build(3, 3, 3, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto actual = Run(fixture, true);
    RequireCentroid(actual, 1, 0.0F, 0.0F, 0.0F);
  }

  SECTION("populated feature zero is corrected while an unused feature stays zero")
  {
    auto fixture = Build(5, 1, 1, {1.0F, 1.0F, 1.0F}, {10.0F, 0.0F, 0.0F}, 3, {0, 1, 1, 1, 0});
    const auto actual = Run(fixture, true);
    RequireCentroid(actual, 0, 10.0F, 0.5F, 0.5F);
    RequireCentroid(actual, 2, 0.0F, 0.0F, 0.0F);
  }

  SECTION("Fixture D - periodic wrap, unit spacing (asymmetric)")
  {
    // 4x1x1; FeatureIds [1,2,1,1]; feature 1 spans x=0 and x=3 (full extent), feature 2 (x=1) does not.
    // feature 1 cells x=0,2,3 -> centers 0.5,2.5,3.5 -> non-periodic mean 6.5/3 = 2.16667
    auto sNP = Build(4, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 3, {1, 2, 1, 1});
    auto nonPeriodic = Run(sNP, false);
    RequireCentroid(nonPeriodic, 1, 6.5f / 3.0f, 0.5f, 0.5f);
    RequireCentroid(nonPeriodic, 2, 1.5f, 0.5f, 0.5f); // single cell x=1

    auto sP = Build(4, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 3, {1, 2, 1, 1});
    auto periodic = Run(sP, true);
    // Class 1 minimum-image oracle: domain L=4 (seam at x=0). The cell at center 0.5 wraps to 4.5, so the
    // contiguous cluster is {2.5, 3.5, 4.5}; mean = 10.5/3 = 3.5, mapped back into [0,4) -> 3.5.
    // (The old constant-offset model gave 6.5/3 + 1.5 = 3.6667, which is wrong.)
    RequireCentroid(periodic, 1, 3.5f, 0.5f, 0.5f);
    // Class 4 invariant: the non-spanning feature 2 must be unchanged by the periodic pass.
    RequireCentroid(periodic, 2, 1.5f, 0.5f, 0.5f);
  }

  SECTION("Fixture E - periodic wrap, NON-unit spacing (asymmetric, interior result)")
  {
    // 4x1x1, spacing (2,1,1), origin (10,0,0); FeatureIds [1,1,2,1]
    // feature 1 cells idx 0,1,3 -> x-centers 11.0, 13.0, 17.0; non-periodic centroid.x = 41/3 = 13.6667
    auto sNP = Build(4, 1, 1, {2.0f, 1.0f, 1.0f}, {10.0f, 0.0f, 0.0f}, 3, {1, 1, 2, 1});
    auto nonPeriodic = Run(sNP, false);
    RequireCentroid(nonPeriodic, 1, 41.0f / 3.0f, 0.5f, 0.5f);

    auto sP = Build(4, 1, 1, {2.0f, 1.0f, 1.0f}, {10.0f, 0.0f, 0.0f}, 3, {1, 1, 2, 1});
    auto periodic = Run(sP, true);
    // Class 1 minimum-image oracle: domain L=8 (seam at x=10 == x=18). Positions relative to origin are
    // {1, 3, 7}; the largest empty gap is 3->7, so 1 and 3 wrap to 9 and 11. Cluster {7, 9, 11}; mean = 9,
    // mapped back into [10,18) -> x = 11.0. (The old constant-offset model gave 13.6667 + 3.0 = 16.6667.)
    RequireCentroid(periodic, 1, 11.0f, 0.5f, 0.5f);
  }

  SECTION("Fixture F - domain-filling feature (arithmetic-mean fallback)")
  {
    // 4x1x1, unit spacing; a single feature fills every cell (centers 0.5,1.5,2.5,3.5). It spans the full
    // extent, so the periodic branch fires, but the unit vectors cancel (zero resultant): the circular mean
    // is undefined, so the arithmetic mean 2.0 is kept. Class 4 fallback path.
    auto sP = Build(4, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 2, {1, 1, 1, 1});
    auto periodic = Run(sP, true);
    RequireCentroid(periodic, 1, 2.0f, 0.5f, 0.5f);
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: sorted-coordinate periodic oracle", "[SimplnxCore][ComputeFeatureCentroidsFilter][Periodic]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();
  std::mt19937 generator(19077);
  std::uniform_int_distribution<int32> labels(0, 3);
  const std::array<float32, 3> origin = {10.0F, -3.0F, 7.0F};
  const std::array<float32, 3> spacing = {-2.0F, 1.25F, 0.5F};
  for(usize trial = 0; trial < 48; trial++)
  {
    const std::array<usize, 3> dims = {2 + trial % 5, 2 + (trial / 5) % 4, 2 + (trial / 11) % 3};
    std::vector<int32> ids(dims[0] * dims[1] * dims[2]);
    for(auto& id : ids)
    {
      id = labels(generator);
    }
    INFO("trial " << trial);
    auto fixture = Build(dims[0], dims[1], dims[2], spacing, origin, 5, ids);
    const auto actual = Run(fixture, true);
    const auto expected = SortedCoordinateOracle(dims, ids, 5, origin, spacing, true);
    REQUIRE(actual.size() == expected.size());
    for(usize index = 0; index < expected.size(); index++)
    {
      REQUIRE(actual[index] == Approx(expected[index]).margin(2.0e-4F));
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: bounded periodic scans and failures", "[SimplnxCore][ComputeFeatureCentroidsFilter][Periodic]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();
  const PreferencesSentinel resident(DataStorageMode::ForceInCore, 0);

  SECTION("no wrapped pair avoids both occupancy and correction scans")
  {
    CentroidsBudgetSentinel budget(0);
    const std::vector<int32> ids = {2, 1, 1, 1, 3};
    auto fixture = Build(5, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 4, ids);
    auto observed = ObserveIds(fixture, ids);
    const auto actual = Run(fixture, true);
    RequireCentroid(actual, 1, 2.5F, 0.5F, 0.5F);
    REQUIRE(observed->readCount() == 2); // validator + original algorithm scan
    budget.requireNoReservationLeak();
  }

  SECTION("one wrapped pair has one occupancy scan and one weighted scan")
  {
    const std::vector<int32> ids = {1, 2, 2, 2, 1};
    auto fixture = Build(5, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto observed = ObserveIds(fixture, ids);
    const auto actual = Run(fixture, true);
    RequireCentroid(actual, 1, 0.0F, 0.5F, 0.5F);
    REQUIRE(observed->readCount() == 4); // validator + original + occupancy + weighted
  }

  SECTION("tied or full-axis gaps omit the weighted correction scan")
  {
    const std::vector<int32> ids = {1, 1, 1, 1, 1};
    auto fixture = Build(5, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 2, ids);
    auto observed = ObserveIds(fixture, ids);
    const auto actual = Run(fixture, true);
    RequireCentroid(actual, 1, 2.5F, 0.5F, 0.5F);
    REQUIRE(observed->readCount() == 3);
  }

  SECTION("two wrapped features batch into one or two occupancy scans")
  {
    const std::vector<int32> ids = {1, 3, 3, 3, 3, 3, 3, 1, 2, 4, 4, 4, 4, 4, 4, 2};
    std::vector<float32> roomy;
    {
      CentroidsBudgetSentinel budget(64); // one quarter gives 16 occupancy bytes
      auto fixture = Build(8, 2, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 5, ids);
      auto observed = ObserveIds(fixture, ids);
      roomy = Run(fixture, true);
      REQUIRE(observed->readCount() == 4);
      budget.requireNoReservationLeak();
    }
    {
      CentroidsBudgetSentinel budget(32); // one quarter gives one 8-byte word
      auto fixture = Build(8, 2, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 5, ids);
      auto observed = ObserveIds(fixture, ids);
      const auto limited = Run(fixture, true);
      REQUIRE(limited == roomy);
      REQUIRE(observed->readCount() == 5);
      budget.requireNoReservationLeak();
    }
  }

  SECTION("minimum grant carries a unique gap across empty windows")
  {
    CentroidsBudgetSentinel budget(32);
    std::vector<int32> ids(260, 2);
    ids[0] = 1;
    ids[1] = 1;
    ids[258] = 1;
    ids[259] = 1;
    auto fixture = Build(260, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto observed = ObserveIds(fixture, ids);
    const auto actual = Run(fixture, true);
    RequireCentroid(actual, 1, 0.0F, 0.5F, 0.5F);
    REQUIRE(observed->readCount() == 8); // validator + original + five windows + weighted
    budget.requireNoReservationLeak();
  }

  SECTION("word 63/64 boundary and partial last word preserve the unique cut")
  {
    CentroidsBudgetSentinel budget(32);
    std::vector<int32> ids(130, 2);
    for(usize q : {usize{0}, usize{63}, usize{64}, usize{129}})
    {
      ids[q] = 1;
    }
    auto fixture = Build(130, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto observed = ObserveIds(fixture, ids);
    const auto actual = Run(fixture, true);
    RequireCentroid(actual, 1, 32.0F, 0.5F, 0.5F);
    REQUIRE(observed->readCount() == 6); // validator + original + three windows + weighted
    budget.requireNoReservationLeak();
  }

  SECTION("equal maxima across windows retain the ordinary mean")
  {
    CentroidsBudgetSentinel budget(32);
    std::vector<int32> ids(129, 2);
    for(usize q : {usize{0}, usize{64}, usize{128}})
    {
      ids[q] = 1;
    }
    auto fixture = Build(129, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
    auto observed = ObserveIds(fixture, ids);
    const auto actual = Run(fixture, true);
    RequireCentroid(actual, 1, 64.5F, 0.5F, 0.5F);
    REQUIRE(observed->readCount() == 5); // validator + original + three windows, no weighted pass
    budget.requireNoReservationLeak();
  }

  SECTION("subminimum and zero grants fail before output publication")
  {
    for(uint64 budgetBytes : {uint64{28}, uint64{0}})
    {
      const std::vector<int32> ids = {1, 2, 2, 1, 1};
      auto fixture = Build(5, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
      PrepareSentinelOutput(fixture, 3);
      CentroidsBudgetSentinel budget(budgetBytes);
      const std::atomic_bool cancel = false;
      const auto result = RunDirect(fixture, cancel);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      REQUIRE(result.errors().front().code == -5321);
      RequireUnpublished(fixture);
      budget.requireNoReservationLeak();
    }
  }

  SECTION("occupancy and weighted read failures leave the final output unpublished")
  {
    const std::vector<int32> ids = {1, 2, 2, 1, 1};
    for(usize failOnRead : {usize{3}, usize{4}})
    {
      auto fixture = Build(5, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
      PrepareSentinelOutput(fixture, 3);
      auto observed = ObserveIds(fixture, ids, failOnRead, nullptr, 0, failOnRead == 4 ? usize{3} : usize{0});
      const uint64 previousReservation = CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes();
      const std::atomic_bool cancel = false;
      const auto result = RunDirect(fixture, cancel);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      REQUIRE(result.errors().front().code == -91912);
      REQUIRE(observed->readCount() == failOnRead);
      REQUIRE(result.warnings().size() == (failOnRead == 4 ? 1 : 0));
      RequireUnpublished(fixture);
      REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == previousReservation);
    }
  }

  SECTION("cancellation in occupancy or weighted pass leaves the final output unpublished")
  {
    const std::vector<int32> ids = {1, 2, 2, 1, 1};
    for(usize cancelOnRead : {usize{3}, usize{4}})
    {
      auto fixture = Build(5, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
      PrepareSentinelOutput(fixture, 3);
      std::atomic_bool cancel = false;
      auto observed = ObserveIds(fixture, ids, 0, &cancel, cancelOnRead, cancelOnRead);
      const uint64 previousReservation = CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes();
      const auto result = RunDirect(fixture, cancel);
      REQUIRE(cancel);
      REQUIRE(observed->readCount() == cancelOnRead);
      REQUIRE(result.warnings().size() == 1);
      RequireUnpublished(fixture);
      REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == previousReservation);
    }
  }

  SECTION("successful occupancy and weighted read warnings reach the caller")
  {
    const std::vector<int32> ids = {1, 2, 2, 1, 1};
    for(usize warnOnRead : {usize{3}, usize{4}})
    {
      auto fixture = Build(5, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 3, ids);
      auto observed = ObserveIds(fixture, ids, 0, nullptr, 0, warnOnRead);
      ComputeFeatureCentroidsFilter filter;
      const auto executeResult = filter.execute(fixture.ds, MakeArguments(fixture, true));
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
      REQUIRE(executeResult.result.warnings().size() == 1);
      REQUIRE(executeResult.result.warnings().front().code == -91913);
      REQUIRE(observed->readCount() == 4);
      REQUIRE_NOTHROW(fixture.ds.getDataRefAs<Float32Array>(fixture.centroidsPath));
      REQUIRE(fixture.ds.getDataRefAs<Float32Array>(fixture.centroidsPath)[3] == Approx(4.5F));
    }
  }

  SECTION("opt-in bounded grant timing probe")
  {
    if(std::getenv("SIMPLNX_CENTROID_TIMING") != nullptr)
    {
      constexpr uint64 k_MiB = 1024ULL * 1024ULL;
      constexpr usize k_DimX = 32768;
      constexpr usize k_DimY = 16;
      constexpr uint64 k_UsefulBytes = k_DimY * (k_DimX / 8) + sizeof(uint64); // 16 X pairs plus one full-Y background pair
      std::vector<int32> ids(k_DimX * k_DimY, static_cast<int32>(k_DimY + 1));
      for(usize y = 0; y < k_DimY; y++)
      {
        ids[y * k_DimX] = static_cast<int32>(y + 1);
        ids[y * k_DimX + k_DimX - 1] = static_cast<int32>(y + 1);
      }
      CentroidsBudgetSentinel budget(256 * k_MiB); // fixed cache budget, 64 MiB maximum working allowance
      auto& manager = CacheMemoryBudgetManager::instance();
      REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
      std::vector<float32> reference;
      for(const uint64 headroom : std::array<uint64, 5>{uint64{8} * k_MiB, uint64{32} * k_MiB, uint64{64} * k_MiB, uint64{64} * 1024ULL, uint64{8} * 1024ULL})
      {
        const uint64 pressureBytes = 64 * k_MiB - headroom;
        auto pressureReservation = manager.reserveWorkingMemory(pressureBytes);
        REQUIRE(pressureReservation.sizeBytes() == pressureBytes);
        std::vector<uint8> pressureBuffer(static_cast<usize>(pressureBytes), uint8{0});
        REQUIRE(pressureBuffer.size() == pressureBytes);
        auto fixture = Build(k_DimX, k_DimY, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, k_DimY + 2, ids);
        auto observed = ObserveIds(fixture, ids);
        const auto started = std::chrono::steady_clock::now();
        const auto actual = Run(fixture, true);
        const auto elapsedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
        const uint64 granted = observed->maxWorkingReserved() - pressureReservation.sizeBytes();
        const usize occupancyScans = (observed->readCount() - 14 - 8 - 8) / 8;
        const usize expectedScans = headroom >= k_UsefulBytes ? 1 : (headroom >= 64 * 1024ULL ? 2 : 9);
        REQUIRE(granted == std::min(k_UsefulBytes, headroom));
        REQUIRE(occupancyScans == expectedScans);
        REQUIRE(observed->readCount() == 14 + (occupancyScans + 2) * 8);
        RequireCentroid(actual, 1, 0.0F, 0.5F, 0.5F);
        if(reference.empty())
        {
          reference = actual;
        }
        else
        {
          REQUIRE(actual == reference);
        }
        std::cout << "CENTROID_TIMING headroom=" << headroom << " useful=" << k_UsefulBytes << " grant=" << granted << " occupancy_scans=" << occupancyScans
                  << " logical_reads=" << observed->readCount() << " elapsed_ms=" << elapsedMs << '\n';
      }
      budget.requireNoReservationLeak();
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: checked working sizes", "[SimplnxCore][ComputeFeatureCentroidsFilter]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();
  const PreferencesSentinel resident(DataStorageMode::ForceInCore, 0);
  const std::atomic_bool cancel = false;

  SECTION("feature-sized byte multiplication fails before allocating scratch")
  {
    constexpr usize k_HugeFeatureCount = std::numeric_limits<usize>::max() / (3 * sizeof(uint64)) + 1;
    auto fixture = Build(1, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, k_HugeFeatureCount, {0});
    auto plannedStore = EmptyDataStore<float32>::Create(ShapeType{k_HugeFeatureCount}, ShapeType{3}, "");
    SIMPLNX_RESULT_REQUIRE_VALID(plannedStore);
    REQUIRE_NOTHROW(fixture.ds.getDataRefAs<AttributeMatrix>(fixture.featureAMPath));
    const auto& featureAM = fixture.ds.getDataRefAs<AttributeMatrix>(fixture.featureAMPath);
    auto* output = Float32Array::Create(fixture.ds, k_CentroidsName, std::shared_ptr<AbstractDataStore<float32>>(std::move(plannedStore.value())), featureAM.getId());
    REQUIRE(output != nullptr);
    const auto result = RunDirect(fixture, cancel);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -5320);
  }

  SECTION("geometry XY and XYZ products fail before voxel scans")
  {
    for(const SizeVec3& dimensions : {SizeVec3{std::numeric_limits<usize>::max(), 2, 1}, SizeVec3{std::numeric_limits<usize>::max() / 2, 2, 2}})
    {
      auto fixture = Build(1, 1, 1, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}, 2, {0});
      PrepareSentinelOutput(fixture, 2);
      REQUIRE_NOTHROW(fixture.ds.getDataRefAs<ImageGeom>(fixture.geomPath));
      fixture.ds.getDataRefAs<ImageGeom>(fixture.geomPath).setDimensions(dimensions);
      const auto result = RunDirect(fixture, cancel);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      REQUIRE(result.errors().front().code == -5320);
      RequireUnpublished(fixture);
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: Error - FeatureId exceeds Feature AM", "[SimplnxCore][ComputeFeatureCentroidsFilter]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();

  // 2x1x1 with FeatureIds {0, 5} but only 2 feature tuples -> id 5 >= numFeatures -> error -5351.
  auto s = Build(2, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 2, {0, 5});

  ComputeFeatureCentroidsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(s.geomPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(s.featureIdsPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(s.featureAMPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_IsPeriodic_Key, std::make_any<bool>(false));

  auto executeResult = filter.execute(s.ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: Bulk boundary and failure propagation", "[SimplnxCore][ComputeFeatureCentroidsFilter]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();
  constexpr usize k_CellCount = 65537;
  const std::array<float32, 3> k_Spacing = {2.0F, 3.0F, 4.0F};
  const std::array<float32, 3> k_Origin = {10.0F, -2.0F, 1.0F};

  SECTION("feature crosses the 65536-cell boundary with nonunit coordinates")
  {
    std::vector<int32> featureIds(k_CellCount, 2);
    featureIds[0] = 1;
    featureIds.back() = 1;
    auto scaffold = Build(k_CellCount, 1, 1, k_Spacing, k_Origin, 3, featureIds);
    const auto centroids = Run(scaffold, false);

    RequireCentroid(centroids, 0, 0.0F, 0.0F, 0.0F);
    // Feature 1 contains x indices 0 and 65536. Their centers are 11 and 131083.
    RequireCentroid(centroids, 1, 65547.0F, -0.5F, 3.0F);
    // Feature 2 contains the contiguous interior [1, 65535]. Its independently derived mean is also 65547.
    RequireCentroid(centroids, 2, 65547.0F, -0.5F, 3.0F);
    UnitTest::CheckArraysInheritTupleDims(scaffold.ds);
  }

  SECTION("second page FeatureIds failure leaves the final centroid publication untouched")
  {
    constexpr int32 k_ReadError = -91911;
    std::vector<int32> featureIds(k_CellCount, 1);
    auto scaffold = Build(k_CellCount, 1, 1, k_Spacing, k_Origin, 2, featureIds);
    auto failingStore = std::make_shared<CentroidsFailOnLaterReadStore>(ShapeType{k_CellCount}, k_ReadError);
    for(usize cellIdx = 0; cellIdx < k_CellCount; cellIdx++)
    {
      (*failingStore)[cellIdx] = 1;
    }
    auto setStoreResult = scaffold.ds.getDataRefAs<Int32Array>(scaffold.featureIdsPath).setDataStore(failingStore);
    SIMPLNX_RESULT_REQUIRE_VALID(setStoreResult);

    ComputeFeatureCentroidsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(scaffold.geomPath));
    args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(scaffold.featureIdsPath));
    args.insertOrAssign(ComputeFeatureCentroidsFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(scaffold.featureAMPath));
    args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));
    args.insertOrAssign(ComputeFeatureCentroidsFilter::k_IsPeriodic_Key, std::make_any<bool>(false));

    const auto executeResult = filter.execute(scaffold.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
    REQUIRE(executeResult.result.errors().front().code == k_ReadError);
    REQUIRE_NOTHROW(scaffold.ds.getDataRefAs<Float32Array>(scaffold.centroidsPath));
    const auto& centroids = scaffold.ds.getDataRefAs<Float32Array>(scaffold.centroidsPath);
    REQUIRE(centroids[3] == 0.0F);
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeatureCentroidsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureCentroidsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureCentroidsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureCentroidsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (DataArrayCreationToAMFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<std::string>(ComputeFeatureCentroidsFilter::k_CentroidsArrayName_Key) == "TestArray");
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: Geometry cell tuple count", "[SimplnxCore][ComputeFeatureCentroidsFilter][TupleCount]")
{
  using FilterType = ComputeFeatureCentroidsFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 3, 2}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // Matching arrays remain valid outside the geometry's Cell Attribute Matrix, with a different tuple shape.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 0), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(inputPath));
  REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{2}, imageGeomPtr->getId()) != nullptr);
  args.insertOrAssign(FilterType::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Image", "Feature Data"})));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -12701);
    REQUIRE(errorRef.message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", tuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}
