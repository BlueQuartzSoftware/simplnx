#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataIOManager.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/DBSCAN.hpp"
#include "SimplnxCore/Filters/Algorithms/DBSCANScanline.hpp"
#include "SimplnxCore/Filters/DBSCANFilter.hpp"

#include <array>
#include <atomic>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
// 2D Cases - Derived From https://scikit-learn.org/stable/auto_examples/cluster/plot_cluster_comparison.html
const std::string k_AnisoArrayName = "aniso";
const DataPath k_AnisoArrayPath = DataPath({k_AnisoArrayName});
const std::string k_BlobsArrayName = "blobs";
const DataPath k_BlobsArrayPath = DataPath({k_BlobsArrayName});
const std::string k_CirclesArrayName = "noisy_circles";
const DataPath k_CirclesArrayPath = DataPath({k_CirclesArrayName});
const std::string k_MoonsArrayName = "noisy_moons";
const DataPath k_MoonsArrayPath = DataPath({k_MoonsArrayName});
const std::string k_NoStructureArrayName = "no_structure";
const DataPath k_NoStructureArrayPath = DataPath({k_NoStructureArrayName});
const std::string k_VariedArrayName = "varied";
const DataPath k_VariedArrayPath = DataPath({k_VariedArrayName});

const DataPath k_AnisoGeomPath = DataPath({"AnisoGeometry"});
const DataPath k_AnsioClusterArrayPath = k_AnisoGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Aniso Cluster Ids");
const DataPath k_BlobsGeomPath = DataPath({"BlobsGeometry"});
const DataPath k_BlobsClusterArrayPath = k_BlobsGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Blobs Cluster Ids");
const DataPath k_CirclesGeomPath = DataPath({"CirclesGeometry"});
const DataPath k_CirclesClusterArrayPath = k_CirclesGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Circles Cluster Ids");
const DataPath k_MoonsGeomPath = DataPath({"MoonsGeometry"});
const DataPath k_MoonsClusterArrayPath = k_MoonsGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Moons Cluster Ids");
const DataPath k_NoStructureGeomPath = DataPath({"NoStructureGeometry"});
const DataPath k_NoStructureClusterArrayPath = k_NoStructureGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("No Structure Cluster Ids");
const DataPath k_VariedGeomPath = DataPath({"VariedGeometry"});
const DataPath k_VariedClusterArrayPath = k_VariedGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Varied Cluster Ids");

const std::string k_IdsPostFix = " Ids";
const std::string k_AMPostFix = " AM";

const fs::path k_2DTestFile(fmt::format("{}/dbscan_test/7_0_2d_dbscan_test_data.dream3d", unit_test::k_TestFilesDir));

void CheckClusterInvariants(const DataStructure& dataStructure, const DataPath& idsPath, const DataPath& amPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(idsPath));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(idsPath);

  // Invariant 1: all IDs non-negative (0 = noise, >=1 = cluster label)
  for(int32 id : ids)
  {
    REQUIRE(id >= 0);
  }

  // Invariant 2: IDs are contiguous — no gap between 0 and maxId
  const int32 maxId = *std::max_element(ids.begin(), ids.end());
  std::vector<bool> seen(static_cast<usize>(maxId + 1), false);
  for(const int32 id : ids)
  {
    seen[static_cast<usize>(id)] = true;
  }
  // Ignore ID zero because that's reserved for unlabeled points
  for(int32 i = 1; i <= maxId; i++)
  {
    REQUIRE(seen[static_cast<usize>(i)]);
  }

  // Invariant 3: AM tuple count equals maxId + 1
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(amPath));
  REQUIRE(static_cast<usize>(maxId + 1) == dataStructure.getDataRefAs<AttributeMatrix>(amPath).getNumberOfTuples());
}

void LDFTestCase2D(const DataPath& targetPath, float32 epsilonVal, int32 minPtsVal, const DataPath& exemplarClusterIds)
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "dbscan_test.tar.gz", "dbscan_test");
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_2DTestFile);

  const std::string generatedIdsName = targetPath.getTargetName() + k_IdsPostFix;
  const auto generatedIdsPath = DataPath{{generatedIdsName}};
  const auto generatedAMPath = DataPath{{targetPath.getTargetName() + k_AMPostFix}};

  {
    // Configure the filter arguments.
    DBSCANFilter filter;
    Arguments args;

    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(epsilonVal));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(minPtsVal));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(targetPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(generatedIdsName));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(generatedAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_LDF_2d_{}_test.dream3d", unit_test::k_BinaryTestOutputDir, targetPath.getTargetName())));
#endif

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath));
  const auto& generatedIds = dataStructure.getDataRefAs<Int32Array>(generatedIdsPath);
  const int32 maxVal = *std::max_element(generatedIds.begin(), generatedIds.end()) + 1;

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath));
  REQUIRE(maxVal == dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath).getNumberOfTuples());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));
  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath), dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));

  ::CheckClusterInvariants(dataStructure, generatedIdsPath, generatedAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

std::vector<usize> BinPoints(const Int32Array& dataArray)
{
  const int32 maxVal = *std::max_element(dataArray.begin(), dataArray.end());
  std::vector<usize> bins(maxVal + 1, 0);

  for(const int32 val : dataArray)
  {
    bins[val]++;
  }

  return bins;
}

void RandomTestCase2D(const DataPath& targetPath, float32 epsilonVal, int32 minPtsVal, const DataPath& exemplarClusterIds, ChoicesParameter::ValueType randomType)
{
  REQUIRE((randomType == to_underlying(DBSCAN::ParseOrder::Random) || randomType == to_underlying(DBSCAN::ParseOrder::SeededRandom)));

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "dbscan_test.tar.gz", "dbscan_test");
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_2DTestFile);

  const std::string generatedIdsName = targetPath.getTargetName() + k_IdsPostFix;
  const auto generatedIdsPath = DataPath{{generatedIdsName}};
  const auto generatedAMPath = DataPath{{targetPath.getTargetName() + k_AMPostFix}};

  uint64 seed = std::mt19937_64::default_seed;

  {
    // Configure the filter arguments.
    DBSCANFilter filter;
    Arguments args;

    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(randomType));
    args.insertOrAssign(DBSCANFilter::k_SeedValue_Key, std::make_any<uint64>(seed)); // Will be ignored if randomType == DBSCAN::ParseOrder::Random
    args.insertOrAssign(DBSCANFilter::k_SeedArrayName_Key, std::make_any<std::string>("seed_array"));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(epsilonVal));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(minPtsVal));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(targetPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(generatedIdsName));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(generatedAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_Random_2d_{}_test.dream3d", unit_test::k_BinaryTestOutputDir, targetPath.getTargetName())));
#endif

  // ParseOrder::Random draws a time-based seed inside the filter, so the shuffle differs on every
  // run. Report the seed the run actually used; without it a failure below is not reproducible.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64Array>(DataPath{{"seed_array"}}));
  const uint64 usedSeed = dataStructure.getDataRefAs<UInt64Array>(DataPath{{"seed_array"}})[0];
  INFO(fmt::format("Parse order {} ran with seed {}", randomType, usedSeed));
  if(randomType == to_underlying(DBSCAN::ParseOrder::SeededRandom))
  {
    // SeededRandom must round-trip the user-supplied seed unchanged
    REQUIRE(usedSeed == seed);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath));
  const auto& generatedIds = dataStructure.getDataRefAs<Int32Array>(generatedIdsPath);
  std::vector<usize> generatedBins = ::BinPoints(generatedIds);
  REQUIRE_FALSE(generatedBins.empty());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath));
  REQUIRE(generatedBins.size() == dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath).getNumberOfTuples());

  // SeededRandom is deterministic so its output can be compared against the LDF exemplar.
  // Random uses a time-based seed: border-grid assignment is order-dependent, so cluster sizes
  // may differ from the exemplar across runs. Only structural invariants are checked for that case.
  if(randomType == to_underlying(DBSCAN::ParseOrder::SeededRandom))
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));
    const auto& exemplarIds = dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds);
    std::vector<usize> exemplarBins = ::BinPoints(exemplarIds);
    REQUIRE_FALSE(exemplarBins.empty());

    REQUIRE(generatedBins.size() == exemplarBins.size());

    // Identifier 0 is unlabeled points so these should match regardless
    REQUIRE(generatedBins[0] == exemplarBins[0]);

    // Clusters should be the same but id labels may be different
    std::vector<bool> visited(exemplarBins.size(), false);
    for(usize i = 1; i < generatedBins.size(); i++)
    {
      bool found = false;
      for(usize j = 1; j < exemplarBins.size(); j++)
      {
        if(!visited[j])
        {
          if(generatedBins[i] == exemplarBins[j])
          {
            found = true;
            visited[j] = true;
            break;
          }
        }
      }
      REQUIRE(found);
    }
  }

  ::CheckClusterInvariants(dataStructure, generatedIdsPath, generatedAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Keeps its values in memory but reports out-of-core residency.
 *
 * DBSCANScanline selects its external implementation from the reported store
 * type, so this store reaches that implementation in a build without an
 * out-of-core storage plugin.
 * @tparam T Element type.
 */
template <typename T>
class ReportedOutOfCoreDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }
};

/**
 * @brief The built-in HDF5 manager, plus external-sort and temporary-record-store providers that fail when used.
 *
 * DBSCAN's external implementation requires both capabilities before it validates
 * input shapes. Its shape checks run before it creates a sorter or record store,
 * so k_ProviderReachedCode shows that execution passed every shape check.
 *
 * The manager keeps the "HDF5" format name and factories so it can stand in for
 * the built-in manager (see ScopedIOManagerReplacement). DataIOCollection asks
 * its managers in name order, and "HDF5" sorts before the out-of-core manager
 * "HDF5-OOC", so these providers answer in both in-core and out-of-core builds.
 */
class DBSCANShapeGuardHDF5Manager : public HDF5::DataIOManager
{
public:
  static constexpr int32 k_ProviderReachedCode = -54199;

  bool supportsExternalSort() const override
  {
    return true;
  }

  Result<std::unique_ptr<IExternalSort>> createExternalSort(const ExternalSortConfig&) const override
  {
    return MakeErrorResult<std::unique_ptr<IExternalSort>>(k_ProviderReachedCode, "DBSCAN shape-guard test: execution reached external-sort creation.");
  }

  bool supportsTemporaryRecordStore() const override
  {
    return true;
  }

  Result<std::unique_ptr<ITemporaryRecordStore>> createTemporaryRecordStore(const TemporaryRecordStoreConfig&) const override
  {
    return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(k_ProviderReachedCode, "DBSCAN shape-guard test: execution reached temporary record-store creation.");
  }
};

/**
 * @brief Replaces one registered I/O manager for the lifetime of this object.
 *
 * DataIOCollection cannot remove a manager, so the replacement takes the format
 * name of an existing manager and the destructor registers the saved original
 * again. The destructor also runs when a failed assertion unwinds the test, so
 * afterwards the collection holds exactly the managers it held before.
 */
class ScopedIOManagerReplacement
{
public:
  /**
   * @brief Registers replacement in place of the manager with the same format name.
   * @param collection Collection to change; it must outlive this object.
   * @param replacement Manager to register. Nothing is registered when no manager has its format name.
   */
  ScopedIOManagerReplacement(DataIOCollection& collection, std::shared_ptr<IDataIOManager> replacement)
  : m_Collection(collection)
  , m_Original(collection.getManager(replacement->formatName()))
  {
    // Registering under a new name would add a manager that nothing can remove.
    if(m_Original != nullptr)
    {
      m_Replaced = m_Collection.addIOManager(std::move(replacement)).valid();
    }
  }

  ~ScopedIOManagerReplacement() noexcept
  {
    if(m_Replaced)
    {
      // A destructor cannot report failure; re-registering an accepted name succeeds.
      (void)m_Collection.addIOManager(m_Original);
    }
  }

  ScopedIOManagerReplacement(const ScopedIOManagerReplacement&) = delete;
  ScopedIOManagerReplacement(ScopedIOManagerReplacement&&) noexcept = delete;
  ScopedIOManagerReplacement& operator=(const ScopedIOManagerReplacement&) = delete;
  ScopedIOManagerReplacement& operator=(ScopedIOManagerReplacement&&) noexcept = delete;

  /**
   * @brief Reports whether the replacement is registered.
   * @return True when the original manager was found and replaced.
   */
  bool replaced() const noexcept
  {
    return m_Replaced;
  }

private:
  DataIOCollection& m_Collection;
  std::shared_ptr<IDataIOManager> m_Original;
  bool m_Replaced = false;
};

/**
 * @brief Calls DBSCANScanline directly, without the filter or its preflight, on out-of-core coordinates.
 * @param pointComponents Coordinate component count.
 * @param maskShape Mask tuple and component counts, or std::nullopt to run without a mask.
 * @return The algorithm result.
 */
Result<> RunDirectOutOfCoreScanline(usize pointComponents, std::optional<std::pair<usize, usize>> maskShape)
{
  constexpr usize k_PointTuples = 4;
  const DataPath pointsPath({"Points"});
  const DataPath maskPath({"Mask"});
  const DataPath idsPath({"Ids"});

  DataStructure dataStructure;
  auto pointStore = std::make_shared<ReportedOutOfCoreDataStore<float32>>(ShapeType{k_PointTuples}, ShapeType{pointComponents}, 0.0F);
  for(usize index = 0; index < pointStore->getSize(); ++index)
  {
    pointStore->setValue(index, static_cast<float32>(index) * 0.25F);
  }
  auto* points = Float32Array::Create(dataStructure, pointsPath.getTargetName(), pointStore);
  REQUIRE(points != nullptr);
  // The external implementation runs only for out-of-core arrays.
  REQUIRE(IsOutOfCore(*points));
  auto* ids = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, idsPath.getTargetName(), ShapeType{k_PointTuples}, ShapeType{1});
  REQUIRE(ids != nullptr);
  if(maskShape.has_value())
  {
    auto* mask = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, maskPath.getTargetName(), ShapeType{maskShape->first}, ShapeType{maskShape->second});
    REQUIRE(mask != nullptr);
    mask->fill(1);
  }

  DBSCANInputValues inputValues;
  inputValues.ClusteringArrayPath = pointsPath;
  inputValues.MaskArrayPath = maskPath;
  inputValues.UseMask = maskShape.has_value();
  inputValues.FeatureIdsArrayPath = idsPath;
  inputValues.Epsilon = 1.0F;
  inputValues.MinPoints = 2;
  inputValues.DistanceMetric = ClusterUtilities::DistanceMetric::Euclidean;
  inputValues.FeatureAM = DataPath({"Features"});
  inputValues.ParseOrder = to_underlying(DBSCAN::ParseOrder::LowDensityFirst);
  inputValues.Seed = 5489;

  const std::atomic_bool shouldCancel = false;
  DBSCANScanline algorithm(dataStructure, IFilter::MessageHandler{}, shouldCancel, &inputValues);
  return algorithm();
}
} // namespace

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Aniso", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  float32 epsVal = 0.15f;
  int32 minPtsVal = 4;
  // The exemplars were generated with LDF
  scope.execute([&] { ::LDFTestCase2D(k_AnisoArrayPath, epsVal, minPtsVal, k_AnsioClusterArrayPath); });
  scope.execute([&] { ::RandomTestCase2D(k_AnisoArrayPath, epsVal, minPtsVal, k_AnsioClusterArrayPath, DBSCAN::ParseOrder::Random); });
  scope.execute([&] { ::RandomTestCase2D(k_AnisoArrayPath, epsVal, minPtsVal, k_AnsioClusterArrayPath, DBSCAN::ParseOrder::SeededRandom); });
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Blobs", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  float32 epsVal = 0.3f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  scope.execute([&] { ::LDFTestCase2D(k_BlobsArrayPath, epsVal, minPtsVal, k_BlobsClusterArrayPath); });
  scope.execute([&] { ::RandomTestCase2D(k_BlobsArrayPath, epsVal, minPtsVal, k_BlobsClusterArrayPath, DBSCAN::ParseOrder::Random); });
  scope.execute([&] { ::RandomTestCase2D(k_BlobsArrayPath, epsVal, minPtsVal, k_BlobsClusterArrayPath, DBSCAN::ParseOrder::SeededRandom); });
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Noisy Circles", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  float32 epsVal = 0.3f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  scope.execute([&] { ::LDFTestCase2D(k_CirclesArrayPath, epsVal, minPtsVal, k_CirclesClusterArrayPath); });
  scope.execute([&] { ::RandomTestCase2D(k_CirclesArrayPath, epsVal, minPtsVal, k_CirclesClusterArrayPath, DBSCAN::ParseOrder::Random); });
  scope.execute([&] { ::RandomTestCase2D(k_CirclesArrayPath, epsVal, minPtsVal, k_CirclesClusterArrayPath, DBSCAN::ParseOrder::SeededRandom); });
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Noisy Moons", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  float32 epsVal = 0.3f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  scope.execute([&] { ::LDFTestCase2D(k_MoonsArrayPath, epsVal, minPtsVal, k_MoonsClusterArrayPath); });
  scope.execute([&] { ::RandomTestCase2D(k_MoonsArrayPath, epsVal, minPtsVal, k_MoonsClusterArrayPath, DBSCAN::ParseOrder::Random); });
  scope.execute([&] { ::RandomTestCase2D(k_MoonsArrayPath, epsVal, minPtsVal, k_MoonsClusterArrayPath, DBSCAN::ParseOrder::SeededRandom); });
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: No Structure", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  float32 epsVal = 0.3f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  scope.execute([&] { ::LDFTestCase2D(k_NoStructureArrayPath, epsVal, minPtsVal, k_NoStructureClusterArrayPath); });
  scope.execute([&] { ::RandomTestCase2D(k_NoStructureArrayPath, epsVal, minPtsVal, k_NoStructureClusterArrayPath, DBSCAN::ParseOrder::Random); });
  scope.execute([&] { ::RandomTestCase2D(k_NoStructureArrayPath, epsVal, minPtsVal, k_NoStructureClusterArrayPath, DBSCAN::ParseOrder::SeededRandom); });
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Varied", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  float32 epsVal = 0.18f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  scope.execute([&] { ::LDFTestCase2D(k_VariedArrayPath, epsVal, minPtsVal, k_VariedClusterArrayPath); });
  scope.execute([&] { ::RandomTestCase2D(k_VariedArrayPath, epsVal, minPtsVal, k_VariedClusterArrayPath, DBSCAN::ParseOrder::Random); });
  scope.execute([&] { ::RandomTestCase2D(k_VariedArrayPath, epsVal, minPtsVal, k_VariedClusterArrayPath, DBSCAN::ParseOrder::SeededRandom); });
}

TEST_CASE("SimplnxCore::DBSCAN: 3D Test (LowDensityFirst)", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "dbscan_test.tar.gz", "dbscan_test");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/dbscan_test/7_0_3d_dbscan_test_data.dream3d", unit_test::k_TestFilesDir)));

  const auto vertexGeom = DataPath{{"Reduced Vertex Geom"}};
  const DataPath targetPath = vertexGeom.createChildPath("Shared Vertex List");
  const DataPath exemplarClusterIds = vertexGeom.createChildPath("VertexData").createChildPath("Cluster Ids");

  const std::string generatedIdsName = targetPath.getTargetName() + k_IdsPostFix;
  const DataPath generatedIdsPath = vertexGeom.createChildPath(generatedIdsName);
  const DataPath generatedAMPath = vertexGeom.createChildPath(targetPath.getTargetName() + k_AMPostFix);

  {
    // Configure the filter arguments.
    DBSCANFilter filter;
    Arguments args;

    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.0099999998f));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(5));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(targetPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(generatedIdsName));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(generatedAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_LDF_3d_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath));
  const auto& generatedIds = dataStructure.getDataRefAs<Int32Array>(generatedIdsPath);
  int32 maxVal = *std::max_element(generatedIds.begin(), generatedIds.end()) + 1;

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath));
  REQUIRE(maxVal == dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath).getNumberOfTuples());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));
  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath), dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::DBSCAN: Analytical Fixture F1 - No Clusters Warning", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Class 1 oracle: 4 points at unit-square corners, epsilon=0.1, minPoints=5.
  // Cell side = 0.1/sqrt(2) ~= 0.0707 -> each point occupies its own 1-point cell -> no core grids -> warning -85640.
  // Expected: all cluster IDs = 0, AM has 1 tuple.
  DataStructure dataStructure;

  const DataPath pointsPath{{"points"}};
  const DataPath clusterIdsPath{{"cluster_ids"}};
  const DataPath featureAMPath{{"cluster_am"}};

  auto* pointsArr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "points", {4}, {2});
  REQUIRE(pointsArr != nullptr);
  auto& pointsRef = pointsArr->getDataStoreRef();
  pointsRef[0] = 0.0f;
  pointsRef[1] = 0.0f; // P0 = (0, 0)
  pointsRef[2] = 1.0f;
  pointsRef[3] = 0.0f; // P1 = (1, 0)
  pointsRef[4] = 0.0f;
  pointsRef[5] = 1.0f; // P2 = (0, 1)
  pointsRef[6] = 1.0f;
  pointsRef[7] = 1.0f; // P3 = (1, 1)

  {
    DBSCANFilter filter;
    Arguments args;

    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.1f));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(5));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(pointsPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("cluster_ids"));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(featureAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    // No core grids -> warning, not error
    REQUIRE(executeResult.result.valid());
    REQUIRE_FALSE(executeResult.result.warnings().empty());
    REQUIRE(executeResult.result.warnings()[0].code == -85640);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(clusterIdsPath));
  const auto& clusterIds = dataStructure.getDataRefAs<Int32Array>(clusterIdsPath);
  for(int32 id : clusterIds)
  {
    REQUIRE(id == 0);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath));
  REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath).getNumberOfTuples() == 1);

  ::CheckClusterInvariants(dataStructure, clusterIdsPath, featureAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::DBSCAN: Analytical Fixture F2 - Mask Exclusion", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Class 1 oracle: 3 points, P2 masked out.
  // epsilon=1.0, minPoints=2 -> cell side = 1.0/sqrt(2) ~= 0.707.
  // Active points P0=(0.0, 0.0) and P1=(0.1, 0.0) both land in grid cell 0 -> 2 points >= minPoints -> core grid -> Cluster 1.
  // P2=(0.0, 0.1) is masked -> excluded from binning -> stays cluster ID 0.
  // Expected: cluster_ids = [1, 1, 0], AM has 2 tuples.
  DataStructure dataStructure;

  const DataPath pointsPath{{"points"}};
  const DataPath maskPath{{"mask"}};
  const DataPath clusterIdsPath{{"cluster_ids"}};
  const DataPath featureAMPath{{"cluster_am"}};

  auto* pointsArr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "points", {3}, {2});
  REQUIRE(pointsArr != nullptr);
  auto& pointsRef = pointsArr->getDataStoreRef();
  pointsRef[0] = 0.0f;
  pointsRef[1] = 0.0f; // P0 = (0.0, 0.0) included
  pointsRef[2] = 0.1f;
  pointsRef[3] = 0.0f; // P1 = (0.1, 0.0) included
  pointsRef[4] = 0.0f;
  pointsRef[5] = 0.1f; // P2 = (0.0, 0.1) masked out

  auto* maskArr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "mask", {3}, {1});
  REQUIRE(maskArr != nullptr);
  auto& maskRef = maskArr->getDataStoreRef();
  maskRef[0] = 1; // P0 included
  maskRef[1] = 1; // P1 included
  maskRef[2] = 0; // P2 excluded

  {
    DBSCANFilter filter;
    Arguments args;

    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(2));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(DBSCANFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(pointsPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("cluster_ids"));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(featureAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(clusterIdsPath));
  const auto& clusterIds = dataStructure.getDataRefAs<Int32Array>(clusterIdsPath);
  REQUIRE(clusterIds[0] == 1); // P0 -> Cluster 1
  REQUIRE(clusterIds[1] == 1); // P1 -> Cluster 1
  REQUIRE(clusterIds[2] == 0); // P2 masked -> noise
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath));
  REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath).getNumberOfTuples() == 2);

  ::CheckClusterInvariants(dataStructure, clusterIdsPath, featureAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::DBSCAN: Analytical Fixture F3 - All Points Masked", "[SimplnxCore][DBSCAN]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Class 1 oracle: same 3 points as F2, but every point is masked off.
  // With no active point there are no grid bounds to derive, so no grid cell can be occupied and no
  // core grid can exist. This pins the contract for that case; before the bounds guard was added,
  // the grid dimensions were computed by casting NaN to usize, which is undefined behavior.
  // Expected: warning -85640, all cluster IDs = 0, AM has 1 tuple.
  DataStructure dataStructure;

  const DataPath pointsPath{{"points"}};
  const DataPath maskPath{{"mask"}};
  const DataPath clusterIdsPath{{"cluster_ids"}};
  const DataPath featureAMPath{{"cluster_am"}};

  auto* pointsArr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "points", {3}, {2});
  REQUIRE(pointsArr != nullptr);
  auto& pointsRef = pointsArr->getDataStoreRef();
  pointsRef[0] = 0.0f;
  pointsRef[1] = 0.0f; // P0 = (0.0, 0.0)
  pointsRef[2] = 0.1f;
  pointsRef[3] = 0.0f; // P1 = (0.1, 0.0)
  pointsRef[4] = 0.0f;
  pointsRef[5] = 0.1f; // P2 = (0.0, 0.1)

  auto* maskArr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "mask", {3}, {1});
  REQUIRE(maskArr != nullptr);
  auto& maskRef = maskArr->getDataStoreRef();
  maskRef[0] = 0;
  maskRef[1] = 0;
  maskRef[2] = 0;

  {
    DBSCANFilter filter;
    Arguments args;

    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(2));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(DBSCANFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(pointsPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("cluster_ids"));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(featureAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    // Nothing to cluster -> warning, not error and not a crash
    REQUIRE(executeResult.result.valid());
    REQUIRE_FALSE(executeResult.result.warnings().empty());
    REQUIRE(executeResult.result.warnings()[0].code == -85640);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(clusterIdsPath));
  const auto& clusterIds = dataStructure.getDataRefAs<Int32Array>(clusterIdsPath);
  for(int32 id : clusterIds)
  {
    REQUIRE(id == 0);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath));
  REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath).getNumberOfTuples() == 1);

  ::CheckClusterInvariants(dataStructure, clusterIdsPath, featureAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::DBSCAN: genuine HDF5 65536-batch tail oracle", "[SimplnxCore][DBSCAN][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  constexpr usize k_BatchTuples = 65536;
  constexpr usize k_PointTuples = k_BatchTuples + 2;
  DataStructure dataStructure;
  const DataPath pointsPath({"Boundary Points"});
  const DataPath maskPath({"Boundary Mask"});
  const DataPath idsPath({"Boundary ClusterIds"});
  const DataPath featurePath({"Boundary Clusters"});
  auto pointStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, pointsPath, {k_PointTuples}, {2});
  auto maskStore = DataStoreUtilities::CreateDataStore<uint8>(dataStructure, maskPath, {k_PointTuples}, {1});
  auto* points = Float32Array::Create(dataStructure, pointsPath.getTargetName(), pointStore);
  auto* mask = UInt8Array::Create(dataStructure, maskPath.getTargetName(), maskStore);
  REQUIRE(points != nullptr);
  REQUIRE(mask != nullptr);
  points->fill(100.0F);
  mask->fill(0);
  (*points)[0] = 0.0F;
  (*points)[1] = 0.0F;
  (*mask)[0] = 1;

  // Four nearby points form two adjacent core grids with epsilon=1 and
  // minPoints=2. They form one cluster; the distant origin point is noise.
  const std::array<float32, 4> xValues = {10.0F, 10.125F, 10.75F, 10.875F};
  for(usize witnessIdx = 0; witnessIdx < xValues.size(); witnessIdx++)
  {
    const usize pointIdx = k_BatchTuples - 2 + witnessIdx;
    (*points)[pointIdx * 2] = xValues[witnessIdx];
    (*points)[pointIdx * 2 + 1] = 10.0F;
    (*mask)[pointIdx] = 1;
  }
  REQUIRE(pointStore->getDataFormat() == "HDF5-OOC");
  REQUIRE(maskStore->getDataFormat() == "HDF5-OOC");

  DBSCANFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
  args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(1.0F));
  args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(2));
  args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(DBSCANFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(pointsPath));
  args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(idsPath.getTargetName()));
  args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(featurePath));
  const auto before = GetAlgorithmPathExecutionCounts();
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto after = GetAlgorithmPathExecutionCounts();
  REQUIRE(after.OutOfCoreOnOutOfCoreStore == before.OutOfCoreOnOutOfCoreStore + 1);
  REQUIRE(after.InCore == before.InCore);
  REQUIRE(executeResult.result.warnings().empty());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(idsPath));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(idsPath);
  REQUIRE(ids.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  std::vector<int32> actual(k_PointTuples);
  auto readResult = ids.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  for(usize pointIdx = 0; pointIdx < k_PointTuples; pointIdx++)
  {
    INFO("point " << pointIdx);
    REQUIRE(actual[pointIdx] == (pointIdx >= k_BatchTuples - 2 ? 1 : 0));
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featurePath));
  REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(featurePath).getNumberOfTuples() == 2);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::DBSCANFilter: No-mask preflight does not create a cell-sized temporary mask", "[SimplnxCore][DBSCANFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* points = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Points", ShapeType{4}, ShapeType{2});
  REQUIRE(points != nullptr);

  DBSCANFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
  args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(1.0F));
  args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(2));
  args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Points"})));
  args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("Ids"));
  args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"Features"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  CHECK(preflightResult.outputActions.value().actions.size() == 2);
  CHECK(preflightResult.outputActions.value().deferredActions.empty());
}

TEST_CASE("SimplnxCore::DBSCAN: Reject multi-component mask in preflight", "[SimplnxCore][DBSCAN][ComponentShape]")
{
  DataStructure dataStructure;
  auto* points = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Points", ShapeType{4}, ShapeType{2});
  REQUIRE(points != nullptr);
  auto* mask = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", ShapeType{4}, ShapeType{2});
  REQUIRE(mask != nullptr);

  DBSCANFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Points"})));
  args.insertOrAssign(DBSCANFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Mask"})));
  args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"Features"})));
  args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
  auto unmaskedResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(unmaskedResult.outputActions);

  args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(true));
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -208);
}

TEST_CASE("SimplnxCore::DBSCAN: Reject short mask in preflight", "[SimplnxCore][DBSCAN][ComponentShape]")
{
  DataStructure dataStructure;
  auto* points = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Points", ShapeType{4}, ShapeType{2});
  REQUIRE(points != nullptr);
  auto* mask = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", ShapeType{3}, ShapeType{1});
  REQUIRE(mask != nullptr);

  DBSCANFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Points"})));
  args.insertOrAssign(DBSCANFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Mask"})));
  args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"Features"})));
  args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
  auto unmaskedResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(unmaskedResult.outputActions);

  args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(true));
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -7587);
  const auto& message = preflightResult.outputActions.errors()[0].message;
  REQUIRE_THAT(message, Catch::Matchers::Contains("DataArray 'Points' has 4 tuples"));
  REQUIRE_THAT(message, Catch::Matchers::Contains("DataArray 'Mask' has 3 tuples"));
}

TEST_CASE("SimplnxCore::DBSCAN: Reject short StringArray mask in preflight", "[SimplnxCore][DBSCAN][ComponentShape]")
{
  DataStructure dataStructure;
  auto* points = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Points", ShapeType{4}, ShapeType{2});
  REQUIRE(points != nullptr);
  auto* mask = StringArray::CreateWithValues(dataStructure, "Mask", ShapeType{3}, {"1", "1", "1"});
  REQUIRE(mask != nullptr);

  DBSCANFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Points"})));
  args.insertOrAssign(DBSCANFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Mask"})));
  args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"Features"})));
  args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(true));
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -7587);
  const auto& message = preflightResult.outputActions.errors()[0].message;
  REQUIRE_THAT(message, Catch::Matchers::Contains("DataArray 'Points' has 4 tuples"));
  REQUIRE_THAT(message, Catch::Matchers::Contains("DataArray 'Mask' has 3 tuples"));
}

TEST_CASE("SimplnxCore::DBSCANScanline: Direct out-of-core call rejects invalid input shapes", "[SimplnxCore][DBSCAN][ComponentShape]")
{
  DataIOCollection& collection = DataStoreUtilities::GetIOCollection();
  const std::shared_ptr<IDataIOManager> builtInHDF5Manager = collection.getManager("HDF5");
  REQUIRE(builtInHDF5Manager != nullptr);
  const bool hadExternalSort = collection.hasExternalSortCapability();
  const bool hadTemporaryRecordStore = collection.hasTemporaryRecordStoreCapability();

  {
    const ScopedIOManagerReplacement providers(collection, std::make_shared<DBSCANShapeGuardHDF5Manager>());
    REQUIRE(providers.replaced());

    SECTION("Valid shapes pass the shape checks")
    {
      const Result<> result = RunDirectOutOfCoreScanline(2, std::make_pair(usize{4}, usize{1}));
      REQUIRE(result.invalid());
      REQUIRE(result.errors().size() == 1);
      REQUIRE(result.errors()[0].code == DBSCANShapeGuardHDF5Manager::k_ProviderReachedCode);
    }
    SECTION("Four coordinate components")
    {
      const Result<> result = RunDirectOutOfCoreScanline(4, std::nullopt);
      REQUIRE(result.invalid());
      REQUIRE(result.errors().size() == 1);
      REQUIRE(result.errors()[0].code == -54060);
      REQUIRE_THAT(result.errors()[0].message, Catch::Matchers::Contains("has a component count of 4"));
    }
    SECTION("Two-component mask")
    {
      const Result<> result = RunDirectOutOfCoreScanline(2, std::make_pair(usize{4}, usize{2}));
      REQUIRE(result.invalid());
      REQUIRE(result.errors().size() == 1);
      REQUIRE(result.errors()[0].code == -54062);
      REQUIRE_THAT(result.errors()[0].message, Catch::Matchers::Contains("it has 4 tuples and a component count of 2"));
    }
    SECTION("Short mask")
    {
      const Result<> result = RunDirectOutOfCoreScanline(2, std::make_pair(usize{3}, usize{1}));
      REQUIRE(result.invalid());
      REQUIRE(result.errors().size() == 1);
      REQUIRE(result.errors()[0].code == -54062);
      REQUIRE_THAT(result.errors()[0].message, Catch::Matchers::Contains("it has 3 tuples and a component count of 1"));
    }
  }

  SECTION("An exception inside the replacement scope restores the built-in manager")
  {
    // A failed REQUIRE also leaves the scope by throwing.
    const auto failInsideScope = [&collection]() {
      const ScopedIOManagerReplacement providers(collection, std::make_shared<DBSCANShapeGuardHDF5Manager>());
      if(providers.replaced())
      {
        throw std::runtime_error("Simulated assertion failure inside the replacement scope");
      }
    };
    REQUIRE_THROWS_AS(failInsideScope(), std::runtime_error);
  }

  // Later tests in the same process must see the collection as it was before this test.
  REQUIRE(collection.getManager("HDF5") == builtInHDF5Manager);
  REQUIRE(collection.hasExternalSortCapability() == hadExternalSort);
  REQUIRE(collection.hasTemporaryRecordStoreCapability() == hadTemporaryRecordStore);
}

TEST_CASE("SimplnxCore::DBSCANFilter: SIMPL Backwards Compatibility", "[SimplnxCore][DBSCANFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "DBSCANFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "DBSCANFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<DBSCANFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Successful pipeline loading verifies the AMPathBuilderFilterParameterConverter value.
      CHECK(args.value<float32>(DBSCANFilter::k_Epsilon_Key) == 2.5f);
      CHECK(args.value<int32>(DBSCANFilter::k_MinPoints_Key) == 5);
      CHECK(args.value<ChoicesParameter::ValueType>(DBSCANFilter::k_DistanceMetric_Key) == 0);
      CHECK(args.value<bool>(DBSCANFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(DBSCANFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(DBSCANFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(DBSCANFilter::k_FeatureIdsArrayName_Key) == "TestName");
    }
  }
}
