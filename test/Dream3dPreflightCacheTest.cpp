#include "DelayVfd.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <fmt/core.h>

#include <nonstd/span.hpp>

#include <H5Ppublic.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
constexpr StringLiteral k_CacheTestFormat = "Task5-Cache-Test-Format";

class CacheRecordingResolver : public IDataStoreFormatResolver
{
public:
  std::string selectedFormat;
  std::string failureMessage;
  mutable std::vector<DataPath> paths;
  mutable std::vector<DataType> types;
  mutable std::vector<uint64> bytes;

  std::string resolveFormat([[maybe_unused]] const DataStructure& dataStructure, const DataPath& path, DataType numericType, uint64 logicalBytes) const override
  {
    paths.push_back(path);
    types.push_back(numericType);
    bytes.push_back(logicalBytes);
    if(!failureMessage.empty())
    {
      throw std::runtime_error(failureMessage);
    }
    return selectedFormat;
  }

  void clearRecords() const
  {
    paths.clear();
    types.clear();
    bytes.clear();
  }
};

class CacheTestDataIOManager : public IDataIOManager
{
public:
  explicit CacheTestDataIOManager(std::shared_ptr<usize> factoryCalls)
  : m_FactoryCalls(std::move(factoryCalls))
  {
    addDataStoreCreationFnc(k_CacheTestFormat.str(),
                            [counter = m_FactoryCalls]([[maybe_unused]] DataType dataType, [[maybe_unused]] const ShapeType& tupleShape, [[maybe_unused]] const ShapeType& componentShape,
                                                       [[maybe_unused]] const std::optional<ShapeType>& chunkShape) -> std::unique_ptr<IDataStore> {
                              ++(*counter);
                              throw std::runtime_error("injected cache value-store factory failure");
                            });
  }

  std::string formatName() const override
  {
    return "Task5-Cache-Test-Manager";
  }

private:
  std::shared_ptr<usize> m_FactoryCalls;
};

struct CacheTestContext
{
  DREAM3D::Dream3dPreflightCache& cache = DREAM3D::Dream3dPreflightCache::Instance();
  std::shared_ptr<CacheRecordingResolver> resolver = std::make_shared<CacheRecordingResolver>();
  std::shared_ptr<usize> factoryCalls = std::make_shared<usize>(0);
  std::shared_ptr<InMemoryFormatResolver> inMemoryResolver = std::make_shared<InMemoryFormatResolver>();

  CacheTestContext()
  {
    REQUIRE(Application::GetOrCreateInstance()->getIOCollection().addIOManager(std::make_shared<CacheTestDataIOManager>(factoryCalls)).valid());
    cache.clear();
    cache.resetStats();
    DataStructure::setDefaultFormatResolver(resolver);
  }

  ~CacheTestContext()
  {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    DREAM3D::Dream3dPreflightCache::SetForceFileMetadataFailure(false);
#endif
    cache.clear();
    DataStructure::setDefaultFormatResolver(inMemoryResolver);
  }

  CacheTestContext(const CacheTestContext&) = delete;
  CacheTestContext(CacheTestContext&&) = delete;
  CacheTestContext& operator=(const CacheTestContext&) = delete;
  CacheTestContext& operator=(CacheTestContext&&) = delete;
};

void AgeFile(const fs::path& filePath)
{
  fs::last_write_time(filePath, fs::last_write_time(filePath) - std::chrono::seconds(10));
}

/**
 * @brief Writes a small .dream3d file and ages its timestamp for cache tests.
 * @param fileName Output file name.
 * @param numTuples Number of tuples in each numeric array.
 * @return The written file path.
 *
 * The file contains two DataArrays and one StringArray. Aging the timestamp
 * prevents the young-file guard from changing hit and miss assertions.
 */
fs::path WriteTestFile(const std::string& fileName, usize numTuples = 10)
{
  DataStructure dataStructure;
  auto* group = DataGroup::Create(dataStructure, "TestGroup");
  auto* attrMat = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{numTuples}, group->getId());
  using FloatStore = DataStore<float32>;
  auto floatStore = std::make_shared<FloatStore>(ShapeType{numTuples}, ShapeType{3}, 1.5f);
  DataArray<float32>::Create(dataStructure, "Floats", floatStore, attrMat->getId());
  auto intStore = std::make_shared<DataStore<int32>>(ShapeType{numTuples}, ShapeType{1}, 7);
  DataArray<int32>::Create(dataStructure, "Ints", intStore, attrMat->getId());
  StringArray::CreateWithValues(dataStructure, "Strings", ShapeType{2}, {"alpha", "beta"}, group->getId());

  fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  fs::remove(filePath);
  Result<> writeResult = DREAM3D::WriteFile(filePath, dataStructure, Pipeline{}, false);
  REQUIRE(writeResult.valid());
  // Backdate mtime past the trust window so validation relies on the (size, mtime) token.
  AgeFile(filePath);
  return filePath;
}

fs::path WriteNumericPolicyFile(const std::string& fileName, DataPath& expectedFirstSharedPath)
{
  DataStructure dataStructure;
  auto* firstGroup = DataGroup::Create(dataStructure, "First");
  auto* secondGroup = DataGroup::Create(dataStructure, "Second");
  REQUIRE(firstGroup != nullptr);
  REQUIRE(secondGroup != nullptr);

  auto sharedStore = std::make_shared<DataStore<int32>>(ShapeType{4}, ShapeType{1}, 7);
  auto* sharedArray = Int32Array::Create(dataStructure, "Shared", sharedStore, firstGroup->getId());
  REQUIRE(sharedArray != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(sharedArray->getId(), secondGroup->getId()));
  REQUIRE_FALSE(sharedArray->getDataPaths().empty());
  expectedFirstSharedPath = sharedArray->getDataPaths().front();

  auto scalarStore = std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{}, 0);
  REQUIRE(Int32Array::Create(dataStructure, "ScalarComponents", scalarStore) != nullptr);
  auto zeroStore = std::make_shared<DataStore<float32>>(ShapeType{2, 0}, ShapeType{3}, 0.0F);
  REQUIRE(Float32Array::Create(dataStructure, "ExplicitZero", zeroStore) != nullptr);

  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  fs::remove(filePath);
  REQUIRE(DREAM3D::WriteFile(filePath, dataStructure, Pipeline{}, false).valid());
  AgeFile(filePath);
  return filePath;
}

fs::path WriteLegacyStatisticsWarningFile(const std::string& fileName)
{
  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  fs::remove(filePath);
  {
    auto file = HDF5::FileIO::WriteFile(filePath);
    REQUIRE(file.isValid());
    REQUIRE(file.writeStringAttribute("FileVersion", DREAM3D::k_LegacyFileVersion.str()).valid());
    auto containers = file.createGroup("DataContainers");
    auto container = containers.createGroup("StatsContainer");
    auto attributeMatrix = container.createGroup("StatsAM");
    REQUIRE(attributeMatrix.writeVectorAttribute("TupleDimensions", ShapeType{1}).valid());
    REQUIRE(attributeMatrix.writeScalarAttribute<uint32>("AttributeMatrixType", 13).valid());

    auto statistics = attributeMatrix.createGroup("StatisticsSource");
    REQUIRE(statistics.writeStringAttribute(Constants::k_ObjectTypeTag, "Statistics").valid());
    auto values = statistics.createDataset("Values");
    const std::vector<float32> payload = {1.0F};
    REQUIRE(values.writeSpan<float32>({1, 1}, nonstd::span<const float32>(payload.data(), payload.size())).valid());
    REQUIRE(values.writeStringAttribute(Constants::k_ObjectTypeTag, DataArray<float32>::GetTypeName()).valid());
    REQUIRE(values.writeVectorAttribute("TupleDimensions", ShapeType{1}).valid());
    REQUIRE(values.writeVectorAttribute("ComponentDimensions", ShapeType{1}).valid());
    REQUIRE(values.writeVectorAttribute<float32>("Mean", {4.0F, 5.0F, 6.0F}).valid());

    auto unsupported = attributeMatrix.createGroup("Unsupported");
    REQUIRE(unsupported.writeStringAttribute(Constants::k_ObjectTypeTag, "UnsupportedLegacyObject").valid());
  }
  AgeFile(filePath);
  return filePath;
}

template <class T>
const DataArray<T>& RequirePlannedArray(const DataStructure& dataStructure, const DataPath& path, const std::string& expectedFormat, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  const auto* array = dataStructure.getDataAs<DataArray<T>>(path);
  REQUIRE(array != nullptr);
  REQUIRE(array->getStoreType() == IDataStore::StoreType::Empty);
  CHECK(array->getDataFormat() == expectedFormat);
  CHECK(array->getTupleShape() == tupleShape);
  CHECK(array->getComponentShape() == componentShape);
  return *array;
}
} // namespace

TEST_CASE("Dream3dPreflightCache: miss then hit with equivalent structure", "[Dream3dPreflightCache]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  const fs::path filePath = WriteTestFile("preflight_cache_basic.dream3d");

  // First fetch: miss (reads the file).
  Result<DataStructure> first = cache.fetch(filePath);
  REQUIRE(first.valid());
  REQUIRE(cache.missCount() == 1);
  REQUIRE(cache.hitCount() == 0);

  // Second fetch: hit (no file read).
  Result<DataStructure> second = cache.fetch(filePath);
  REQUIRE(second.valid());
  REQUIRE(cache.missCount() == 1);
  REQUIRE(cache.hitCount() == 1);

  // Hit handout is structurally equivalent to a direct preflight import.
  auto fileReader = HDF5::FileIO::ReadFile(filePath);
  Result<DataStructure> direct = DREAM3D::ImportDataStructureFromFile(fileReader, true);
  REQUIRE(direct.valid());
  auto expectedPaths = direct.value().getAllDataPaths();
  auto actualPaths = second.value().getAllDataPaths();
  REQUIRE(actualPaths.size() == expectedPaths.size());
  for(const auto& path : expectedPaths)
  {
    INFO(path.toString());
    REQUIRE(second.value().containsData(path));
  }
  const auto* floats = second.value().getDataAs<Float32Array>(DataPath({"TestGroup", "CellData", "Floats"}));
  REQUIRE(floats != nullptr);
  REQUIRE(floats->getTupleShape() == ShapeType{10});
  REQUIRE(floats->getComponentShape() == ShapeType{3});
}

TEST_CASE("Dream3dPreflightCache: fetch of a file containing a StringArray succeeds", "[Dream3dPreflightCache]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  const fs::path filePath = WriteTestFile("preflight_cache_stringarray.dream3d");
  const DataPath stringsPath({"TestGroup", "Strings"});

  // RefreshStores must preserve StringArray placeholders without reading values.
  // Both miss and hit paths run RefreshStores, so this case exercises both paths.
  REQUIRE_NOTHROW(cache.fetch(filePath));
  Result<DataStructure> result = cache.fetch(filePath);
  REQUIRE(result.valid());
  REQUIRE(cache.hitCount() == 1);

  const auto* strings = result.value().getDataAs<StringArray>(stringsPath);
  REQUIRE(strings != nullptr);
  REQUIRE(strings->isPlaceholder());
  REQUIRE(strings->getNumberOfTuples() == 2);
}

TEST_CASE("Dream3dPreflightCache: missing file returns open error", "[Dream3dPreflightCache]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  Result<DataStructure> result = cache.fetch(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "does_not_exist.dream3d");
  REQUIRE(result.invalid());
  REQUIRE(result.errors()[0].code == -25);
}

TEST_CASE("Dream3dPreflightCache: handouts are isolated from master and each other", "[Dream3dPreflightCache]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  const fs::path filePath = WriteTestFile("preflight_cache_isolation.dream3d");
  const DataPath floatsPath({"TestGroup", "CellData", "Floats"});
  const DataPath stringsPath({"TestGroup", "Strings"});

  Result<DataStructure> resultA = cache.fetch(filePath);
  Result<DataStructure> resultB = cache.fetch(filePath);
  REQUIRE(resultA.valid());
  REQUIRE(resultB.valid());
  DataStructure handoutA = std::move(resultA.value());
  DataStructure handoutB = std::move(resultB.value());

  auto* arrayA = handoutA.getDataAs<Float32Array>(floatsPath);
  auto* arrayB = handoutB.getDataAs<Float32Array>(floatsPath);
  REQUIRE(arrayA != nullptr);
  REQUIRE(arrayB != nullptr);

  // Each handout must own a distinct EmptyDataStore instance.
  // Pointer inequality proves that mutating one handout cannot reach another.
  Result<DataStructure> resultC = cache.fetch(filePath);
  REQUIRE(resultC.valid());
  DataStructure handoutC = std::move(resultC.value());
  auto* arrayC = handoutC.getDataAs<Float32Array>(floatsPath);
  REQUIRE(arrayC != nullptr);
  REQUIRE(arrayA->getIDataStore() != arrayB->getIDataStore());
  REQUIRE(arrayA->getIDataStore() != arrayC->getIDataStore());
  REQUIRE(arrayB->getIDataStore() != arrayC->getIDataStore());

  // StringArray placeholders support resizing but not element access.
  // Resizing one handout must not change the tuple count of other handouts.
  auto* stringsA = handoutA.getDataAs<StringArray>(stringsPath);
  auto* stringsB = handoutB.getDataAs<StringArray>(stringsPath);
  auto* stringsC = handoutC.getDataAs<StringArray>(stringsPath);
  REQUIRE(stringsA != nullptr);
  REQUIRE(stringsB != nullptr);
  REQUIRE(stringsC != nullptr);
  REQUIRE(stringsA->isPlaceholder());
  REQUIRE(stringsB->isPlaceholder());
  REQUIRE(stringsC->isPlaceholder());
  REQUIRE(stringsA->resizeTuples(ShapeType{5}).valid());
  REQUIRE(stringsA->getNumberOfTuples() == 5);
  REQUIRE(stringsB->getNumberOfTuples() == 2);
  REQUIRE(stringsC->getNumberOfTuples() == 2);
}

TEST_CASE("Dream3dPreflightCache: modified file is detected and re-read", "[Dream3dPreflightCache]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  const fs::path filePath = WriteTestFile("preflight_cache_stale.dream3d");
  const auto cachedMtime = fs::last_write_time(filePath);
  REQUIRE(cache.fetch(filePath).valid());
  REQUIRE(cache.missCount() == 1);

  // Rewrite the file and assign an explicit older timestamp.
  // HDF5 can give different tuple counts the same file size.
  // Filesystem timestamp granularity can also hide rapid rewrites.
  // A distinct timestamp makes staleness detection deterministic.
  WriteTestFile("preflight_cache_stale.dream3d", 20);
  fs::last_write_time(filePath, cachedMtime - std::chrono::seconds(20));

  Result<DataStructure> refreshed = cache.fetch(filePath);
  REQUIRE(refreshed.valid());
  REQUIRE(cache.missCount() == 2);
  REQUIRE(refreshed.value().getDataAs<Float32Array>(DataPath({"TestGroup", "CellData", "Floats"}))->getTupleShape() == ShapeType{20});
}

TEST_CASE("Dream3dPreflightCache: files younger than the trust window are never served from cache", "[Dream3dPreflightCache]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  const fs::path filePath = WriteTestFile("preflight_cache_young.dream3d");
  // A young timestamp must bypass the cache.
  fs::last_write_time(filePath, fs::file_time_type::clock::now());

  REQUIRE(cache.fetch(filePath).valid());
  REQUIRE(cache.fetch(filePath).valid());
  REQUIRE(cache.missCount() == 2);
  REQUIRE(cache.hitCount() == 0);

  // Once the mtime ages past the window, caching resumes.
  fs::last_write_time(filePath, fs::last_write_time(filePath) - std::chrono::seconds(10));
  REQUIRE(cache.fetch(filePath).valid());
  REQUIRE(cache.fetch(filePath).valid());
  REQUIRE(cache.hitCount() == 1);
}

TEST_CASE("Dream3dPreflightCache: invalidate() forces a re-read", "[Dream3dPreflightCache]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  const fs::path filePath = WriteTestFile("preflight_cache_invalidate.dream3d");
  REQUIRE(cache.fetch(filePath).valid());
  cache.invalidate(filePath);
  REQUIRE(cache.fetch(filePath).valid());
  REQUIRE(cache.missCount() == 2);
  REQUIRE(cache.hitCount() == 0);
}

TEST_CASE("Dream3dPreflightCache: least-recently-used entry is evicted past capacity", "[Dream3dPreflightCache]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  std::vector<fs::path> files;
  for(usize i = 0; i < DREAM3D::Dream3dPreflightCache::k_Capacity + 1; i++)
  {
    files.push_back(WriteTestFile(fmt::format("preflight_cache_lru_{}.dream3d", i)));
  }
  // Fill the cache, then touch file 0 so file 1 becomes the least-recently used entry.
  for(usize i = 0; i < DREAM3D::Dream3dPreflightCache::k_Capacity; i++)
  {
    REQUIRE(cache.fetch(files[i]).valid());
  }
  REQUIRE(cache.fetch(files[0]).valid());                                          // hit; refreshes recency
  REQUIRE(cache.fetch(files[DREAM3D::Dream3dPreflightCache::k_Capacity]).valid()); // miss; evicts files[1]

  cache.resetStats();
  REQUIRE(cache.fetch(files[0]).valid());
  REQUIRE(cache.hitCount() == 1); // still cached
  REQUIRE(cache.fetch(files[1]).valid());
  REQUIRE(cache.missCount() == 1); // was evicted
}

TEST_CASE("Dream3dPreflightCache: concurrent fetches are safe and consistent", "[Dream3dPreflightCache]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  const fs::path fileA = WriteTestFile("preflight_cache_conc_a.dream3d");
  const fs::path fileB = WriteTestFile("preflight_cache_conc_b.dream3d", 20);
  const DataPath floatsPath({"TestGroup", "CellData", "Floats"});

  constexpr usize k_NumThreads = 8;
  constexpr usize k_FetchesPerThread = 25;
  std::atomic<usize> failures{0};
  std::vector<std::thread> threads;
  for(usize t = 0; t < k_NumThreads; t++)
  {
    threads.emplace_back([&, t]() {
      for(usize i = 0; i < k_FetchesPerThread; i++)
      {
        const fs::path& target = (t + i) % 2 == 0 ? fileA : fileB;
        const usize expectedTuples = (t + i) % 2 == 0 ? 10 : 20;
        Result<DataStructure> result = cache.fetch(target);
        if(result.invalid() || result.value().getDataAs<Float32Array>(floatsPath) == nullptr || result.value().getDataAs<Float32Array>(floatsPath)->getTupleShape() != ShapeType{expectedTuples})
        {
          failures++;
        }
      }
    });
  }
  for(auto& thread : threads)
  {
    thread.join();
  }
  REQUIRE(failures.load() == 0);
  REQUIRE(cache.hitCount() + cache.missCount() == k_NumThreads * k_FetchesPerThread);
}

// This hidden benchmark measures cache behavior under simulated storage latency.
// It requires the delay driver and is excluded from default runs.
// Run it manually with:
//   ./simplnx_test "[.benchmark]"
TEST_CASE("Dream3dPreflightCache: benchmark under simulated storage latency", "[.benchmark]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();

  // Many small arrays make metadata traversal dominate this fixture.
  DataStructure dataStructure;
  auto* group = DataGroup::Create(dataStructure, "BenchGroup");
  for(usize i = 0; i < 150; i++)
  {
    auto store = std::make_shared<DataStore<float32>>(ShapeType{8}, ShapeType{1}, 0.0f);
    Float32Array::Create(dataStructure, fmt::format("Array_{}", i), store, group->getId());
  }
  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "preflight_cache_benchmark.dream3d";
  fs::remove(filePath);
  REQUIRE(DREAM3D::WriteFile(filePath, dataStructure, Pipeline{}, false).valid());
  fs::last_write_time(filePath, fs::last_write_time(filePath) - std::chrono::seconds(10));

  // The delay driver adds 2 ms to each HDF5 open or read.
  const hid_t driverId = DelayVfd::Register();
  REQUIRE(driverId != H5I_INVALID_HID);
  DelayVfd::SetDelayMicroseconds(2000);
  HDF5::FileIO::SetFaplConfigurator([driverId](hid_t faplId) { H5Pset_driver(faplId, driverId, nullptr); });

  using Clock = std::chrono::steady_clock;

  // Use microseconds because a warm cache hit is less than one millisecond.
  // The cold fetch performs a full metadata traversal.
  cache.clear();
  const auto coldStart = Clock::now();
  REQUIRE(cache.fetch(filePath).valid());
  const auto coldUs = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - coldStart).count();

  // The warm fetch performs a timestamp check only.
  const auto warmStart = Clock::now();
  REQUIRE(cache.fetch(filePath).valid());
  const auto warmUs = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - warmStart).count();

  HDF5::FileIO::SetFaplConfigurator({});
  DelayVfd::SetDelayMicroseconds(0);

  WARN(fmt::format("PreflightCache benchmark @2ms/op: cold (uncached, = old per-edit cost) {} us | warm (cached) {} us", coldUs, warmUs));
  REQUIRE(warmUs * 10 < coldUs); // the cache must be at least 10x faster under latency
}

TEST_CASE("StorageFormatPlan: cache isolates numeric list and string handout stores", "[Dream3dPreflightCache][StorageFormatPlan][T16]")
{
  DataStructure source;
  auto* numeric = Int32Array::Create(source, "Numeric", std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{1}, 9));
  REQUIRE(numeric != nullptr);
  auto* lists = NeighborList<int32>::Create(source, "Lists", ShapeType{3});
  REQUIRE(lists != nullptr);
  lists->setList(0, std::vector<int32>{3, 6});
  auto* strings = StringArray::CreateWithValues(source, "Strings", ShapeType{2}, {"alpha", "beta"});
  REQUIRE(strings != nullptr);
  const auto filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "preflight_store_copy_resolution.dream3d";
  auto fileWriteResult = DREAM3D::WriteFile(filePath, source, Pipeline{}, false);
  SIMPLNX_RESULT_REQUIRE_VALID(fileWriteResult);
  AgeFile(filePath);

  CacheTestContext context;
  context.resolver->selectedFormat.clear();
  auto first = context.cache.fetch(filePath);
  REQUIRE(first.valid());
  context.resolver->clearRecords();
  auto second = context.cache.fetch(filePath);
  REQUIRE(second.valid());
  REQUIRE(context.cache.hitCount() == 1);
  REQUIRE(std::find(context.resolver->paths.begin(), context.resolver->paths.end(), DataPath({"Numeric"})) != context.resolver->paths.end());
  REQUIRE(std::find(context.resolver->paths.begin(), context.resolver->paths.end(), DataPath({"Lists"})) != context.resolver->paths.end());
  auto* firstNumeric = first.value().getDataAs<Int32Array>(DataPath({"Numeric"}));
  auto* secondNumeric = second.value().getDataAs<Int32Array>(DataPath({"Numeric"}));
  auto* firstLists = first.value().getDataAs<NeighborList<int32>>(DataPath({"Lists"}));
  auto* secondLists = second.value().getDataAs<NeighborList<int32>>(DataPath({"Lists"}));
  auto* firstStrings = first.value().getDataAs<StringArray>(DataPath({"Strings"}));
  auto* secondStrings = second.value().getDataAs<StringArray>(DataPath({"Strings"}));
  REQUIRE(firstNumeric != nullptr);
  REQUIRE(secondNumeric != nullptr);
  REQUIRE(firstLists != nullptr);
  REQUIRE(secondLists != nullptr);
  REQUIRE(firstStrings != nullptr);
  REQUIRE(secondStrings != nullptr);
  REQUIRE(secondNumeric->getIDataStore()->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(secondNumeric->getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(secondNumeric->getIDataStore()->getDataFormat().empty());
  REQUIRE(firstNumeric->getIDataStore() != secondNumeric->getIDataStore());
  REQUIRE(dynamic_cast<EmptyListStore<int32>*>(secondLists->getStore().get()) != nullptr);
  REQUIRE(firstLists->getStore() != secondLists->getStore());
  REQUIRE(firstStrings->isPlaceholder());
  REQUIRE(secondStrings->isPlaceholder());
  REQUIRE(firstStrings->resizeTuples(ShapeType{5}).valid());
  REQUIRE(firstLists->getStore()->resizeTuples(ShapeType{5}).valid());
  CHECK(secondStrings->getTupleShape() == ShapeType{2});
  CHECK(secondLists->getTupleShape() == ShapeType{3});
}

TEST_CASE("StorageFormatPlan: cache replans every fetch route with current policy", "[Dream3dPreflightCache][StorageFormatPlan][T16]")
{
  CacheTestContext context;
  DataPath firstSharedPath;
  const fs::path cachedFile = WriteNumericPolicyFile("preflight_cache_policy_routes.dream3d", firstSharedPath);
  const DataPath alternateSharedPath = firstSharedPath == DataPath({"First", "Shared"}) ? DataPath({"Second", "Shared"}) : DataPath({"First", "Shared"});
  const DataPath scalarPath({"ScalarComponents"});
  const DataPath zeroPath({"ExplicitZero"});

  context.resolver->selectedFormat = k_CacheTestFormat.str();
  auto cold = context.cache.fetch(cachedFile);
  SIMPLNX_RESULT_REQUIRE_VALID(cold);
  REQUIRE(context.cache.missCount() == 1);
  REQUIRE(context.cache.hitCount() == 0);
  const auto& coldShared = RequirePlannedArray<int32>(cold.value(), firstSharedPath, k_CacheTestFormat.str(), ShapeType{4}, ShapeType{1});
  const auto& coldScalar = RequirePlannedArray<int32>(cold.value(), scalarPath, k_CacheTestFormat.str(), ShapeType{3}, ShapeType{});
  const auto& coldZero = RequirePlannedArray<float32>(cold.value(), zeroPath, k_CacheTestFormat.str(), ShapeType{2, 0}, ShapeType{3});
  REQUIRE(*context.factoryCalls == 0);

  context.resolver->clearRecords();
  context.resolver->selectedFormat.clear();
  auto warm = context.cache.fetch(cachedFile);
  SIMPLNX_RESULT_REQUIRE_VALID(warm);
  REQUIRE(context.cache.missCount() == 1);
  REQUIRE(context.cache.hitCount() == 1);
  REQUIRE(context.resolver->paths.size() == 3);
  REQUIRE(std::count(context.resolver->paths.begin(), context.resolver->paths.end(), firstSharedPath) == 1);
  REQUIRE(std::find(context.resolver->paths.begin(), context.resolver->paths.end(), alternateSharedPath) == context.resolver->paths.end());
  const auto scalarRecord = std::find(context.resolver->paths.begin(), context.resolver->paths.end(), scalarPath);
  REQUIRE(scalarRecord != context.resolver->paths.end());
  CHECK(context.resolver->bytes[static_cast<usize>(std::distance(context.resolver->paths.begin(), scalarRecord))] == 3 * sizeof(int32));
  const auto zeroRecord = std::find(context.resolver->paths.begin(), context.resolver->paths.end(), zeroPath);
  REQUIRE(zeroRecord != context.resolver->paths.end());
  CHECK(context.resolver->bytes[static_cast<usize>(std::distance(context.resolver->paths.begin(), zeroRecord))] == 0);
  const auto& warmShared = RequirePlannedArray<int32>(warm.value(), firstSharedPath, "", ShapeType{4}, ShapeType{1});
  const auto& warmScalar = RequirePlannedArray<int32>(warm.value(), scalarPath, "", ShapeType{3}, ShapeType{});
  const auto& warmZero = RequirePlannedArray<float32>(warm.value(), zeroPath, "", ShapeType{2, 0}, ShapeType{3});
  CHECK(coldShared.getIDataStore() != warmShared.getIDataStore());
  CHECK(coldScalar.getIDataStore() != warmScalar.getIDataStore());
  CHECK(coldZero.getIDataStore() != warmZero.getIDataStore());
  CHECK(coldShared.getDataFormat() == k_CacheTestFormat.str());

  context.resolver->selectedFormat = k_CacheTestFormat.str();
  auto newest = context.cache.fetch(cachedFile);
  SIMPLNX_RESULT_REQUIRE_VALID(newest);
  const auto& newestShared = RequirePlannedArray<int32>(newest.value(), firstSharedPath, k_CacheTestFormat.str(), ShapeType{4}, ShapeType{1});
  CHECK(newestShared.getIDataStore() != coldShared.getIDataStore());
  CHECK(newestShared.getIDataStore() != warmShared.getIDataStore());
  CHECK(warmShared.getDataFormat().empty());
  REQUIRE(*context.factoryCalls == 0);

  DataPath recentFirstPath;
  const fs::path recentFile = WriteNumericPolicyFile("preflight_cache_policy_recent.dream3d", recentFirstPath);
  fs::last_write_time(recentFile, fs::file_time_type::clock::now());
  context.cache.resetStats();
  context.resolver->selectedFormat.clear();
  auto recentMemory = context.cache.fetch(recentFile);
  SIMPLNX_RESULT_REQUIRE_VALID(recentMemory);
  context.resolver->selectedFormat = k_CacheTestFormat.str();
  auto recentPlanned = context.cache.fetch(recentFile);
  SIMPLNX_RESULT_REQUIRE_VALID(recentPlanned);
  REQUIRE(context.cache.missCount() == 2);
  REQUIRE(context.cache.hitCount() == 0);
  CHECK(RequirePlannedArray<int32>(recentMemory.value(), recentFirstPath, "", ShapeType{4}, ShapeType{1}).getIDataStore() !=
        RequirePlannedArray<int32>(recentPlanned.value(), recentFirstPath, k_CacheTestFormat.str(), ShapeType{4}, ShapeType{1}).getIDataStore());

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  DataPath statFirstPath;
  const fs::path statFile = WriteNumericPolicyFile("preflight_cache_policy_stat.dream3d", statFirstPath);
  context.cache.resetStats();
  DREAM3D::Dream3dPreflightCache::SetForceFileMetadataFailure(true);
  auto restoreStat = MakeScopeGuard([]() noexcept { DREAM3D::Dream3dPreflightCache::SetForceFileMetadataFailure(false); });
  auto statBypass = context.cache.fetch(statFile);
  SIMPLNX_RESULT_REQUIRE_VALID(statBypass);
  REQUIRE(context.cache.missCount() == 1);
  REQUIRE(context.cache.hitCount() == 0);
  RequirePlannedArray<int32>(statBypass.value(), statFirstPath, k_CacheTestFormat.str(), ShapeType{4}, ShapeType{1});
  DREAM3D::Dream3dPreflightCache::SetForceFileMetadataFailure(false);
  auto cachedAfterStat = context.cache.fetch(statFile);
  SIMPLNX_RESULT_REQUIRE_VALID(cachedAfterStat);
  REQUIRE(context.cache.missCount() == 2);
#endif
}

TEST_CASE("StorageFormatPlan: cache hit planning failure is not retried as a miss", "[Dream3dPreflightCache][StorageFormatPlan][T30]")
{
  CacheTestContext context;
  DataPath firstSharedPath;
  const fs::path filePath = WriteNumericPolicyFile("preflight_cache_planning_failure.dream3d", firstSharedPath);
  context.resolver->selectedFormat.clear();
  REQUIRE(context.cache.fetch(filePath).valid());

  context.cache.resetStats();
  context.resolver->clearRecords();
  context.resolver->failureMessage = "injected cache resolver failure";
  auto failed = context.cache.fetch(filePath);
  REQUIRE(failed.invalid());
  REQUIRE_FALSE(failed.errors().empty());
  CHECK(failed.errors().front().code == -10601);
  CHECK(failed.errors().front().message.find(filePath.string()) != std::string::npos);
  CHECK(failed.errors().front().message.find(firstSharedPath.toString()) != std::string::npos);
  CHECK(failed.errors().front().message.find(context.resolver->failureMessage) != std::string::npos);
  REQUIRE(context.cache.hitCount() == 1);
  REQUIRE(context.cache.missCount() == 0);
  REQUIRE(*context.factoryCalls == 0);

  context.resolver->failureMessage.clear();
  context.resolver->selectedFormat = k_CacheTestFormat.str();
  auto recovered = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(recovered);
  REQUIRE(context.cache.hitCount() == 2);
  REQUIRE(context.cache.missCount() == 0);
  RequirePlannedArray<int32>(recovered.value(), firstSharedPath, k_CacheTestFormat.str(), ShapeType{4}, ShapeType{1});
}

TEST_CASE("StorageFormatPlan: cache preserves populated legacy Statistics values and isolation", "[Dream3dPreflightCache][StorageFormatPlan][T16]")
{
  CacheTestContext context;
  context.resolver->selectedFormat.clear();
  const fs::path filePath = WriteLegacyStatisticsWarningFile("preflight_cache_statistics_isolation.dream3d");
  const DataPath statisticsPath({"StatsContainer", "Statistics", "Values_Mean"});

  auto cold = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(cold);
  auto* coldArray = cold.value().getDataAs<Float32Array>(statisticsPath);
  REQUIRE(coldArray != nullptr);
  REQUIRE(coldArray->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(std::find(context.resolver->paths.begin(), context.resolver->paths.end(), statisticsPath) != context.resolver->paths.end());
  CHECK(coldArray->getDataStoreRef().getValue(0) == 4.0F);
  coldArray->getDataStoreRef().setValue(0, 99.0F);

  auto warm = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(warm);
  auto* warmArray = warm.value().getDataAs<Float32Array>(statisticsPath);
  REQUIRE(warmArray != nullptr);
  REQUIRE(warmArray->getIDataStore() != coldArray->getIDataStore());
  CHECK(warmArray->getDataStoreRef().getValue(0) == 4.0F);
  warmArray->getDataStoreRef().setValue(0, 88.0F);

  auto third = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(third);
  auto* thirdArray = third.value().getDataAs<Float32Array>(statisticsPath);
  REQUIRE(thirdArray != nullptr);
  REQUIRE(thirdArray->getIDataStore() != coldArray->getIDataStore());
  REQUIRE(thirdArray->getIDataStore() != warmArray->getIDataStore());
  CHECK(thirdArray->getDataStoreRef().getValue(0) == 4.0F);

  context.cache.resetStats();
  context.resolver->selectedFormat = k_CacheTestFormat.str();
  *context.factoryCalls = 0;
  auto failed = context.cache.fetch(filePath);
  REQUIRE(failed.invalid());
  REQUIRE_FALSE(failed.errors().empty());
  CHECK(failed.errors().front().code == -6210);
  CHECK(failed.errors().front().message.find(filePath.string()) != std::string::npos);
  CHECK(failed.errors().front().message.find(statisticsPath.toString()) != std::string::npos);
  CHECK(failed.errors().front().message.find(k_CacheTestFormat.str()) != std::string::npos);
  CHECK(failed.errors().front().message.find("injected cache value-store factory failure") != std::string::npos);
  REQUIRE(context.cache.hitCount() == 1);
  REQUIRE(context.cache.missCount() == 0);
  REQUIRE(*context.factoryCalls == 1);

  context.resolver->selectedFormat.clear();
  auto restored = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(restored);
  REQUIRE_NOTHROW(restored.value().getDataRefAs<Float32Array>(statisticsPath));
  CHECK(restored.value().getDataRefAs<Float32Array>(statisticsPath).getDataStoreRef().getValue(0) == 4.0F);
}

TEST_CASE("StorageFormatPlan: disk warnings survive cache bypass preparation", "[Dream3dPreflightCache][StorageFormatPlan][T16][T30]")
{
  auto hasLegacyWarning = [](const Result<DataStructure>& result) {
    return std::any_of(result.warnings().begin(), result.warnings().end(), [](const Warning& warning) { return warning.code == -298012; });
  };

  CacheTestContext context;
  context.resolver->selectedFormat.clear();

  const fs::path coldFile = WriteLegacyStatisticsWarningFile("preflight_cache_warning_cold.dream3d");
  auto cold = context.cache.fetch(coldFile);
  SIMPLNX_RESULT_REQUIRE_VALID(cold);
  CHECK(hasLegacyWarning(cold));

  const fs::path recentFile = WriteLegacyStatisticsWarningFile("preflight_cache_warning_recent.dream3d");
  fs::last_write_time(recentFile, fs::file_time_type::clock::now());
  auto recent = context.cache.fetch(recentFile);
  SIMPLNX_RESULT_REQUIRE_VALID(recent);
  CHECK(hasLegacyWarning(recent));

  const fs::path failedRecentFile = WriteLegacyStatisticsWarningFile("preflight_cache_warning_recent_failure.dream3d");
  fs::last_write_time(failedRecentFile, fs::file_time_type::clock::now());
  context.resolver->failureMessage = "injected cache resolver failure";
  auto failedRecent = context.cache.fetch(failedRecentFile);
  REQUIRE(failedRecent.invalid());
  CHECK(hasLegacyWarning(failedRecent));

  context.resolver->failureMessage.clear();
  const fs::path failedColdFile = WriteLegacyStatisticsWarningFile("preflight_cache_warning_cold_failure.dream3d");
  context.resolver->failureMessage = "injected cache resolver failure";
  auto failedCold = context.cache.fetch(failedColdFile);
  REQUIRE(failedCold.invalid());
  CHECK(hasLegacyWarning(failedCold));

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  context.resolver->failureMessage.clear();
  const fs::path statFile = WriteLegacyStatisticsWarningFile("preflight_cache_warning_stat.dream3d");
  DREAM3D::Dream3dPreflightCache::SetForceFileMetadataFailure(true);
  auto restoreStat = MakeScopeGuard([]() noexcept { DREAM3D::Dream3dPreflightCache::SetForceFileMetadataFailure(false); });
  auto stat = context.cache.fetch(statFile);
  SIMPLNX_RESULT_REQUIRE_VALID(stat);
  CHECK(hasLegacyWarning(stat));

  context.resolver->failureMessage = "injected cache resolver failure";
  auto failedStat = context.cache.fetch(statFile);
  REQUIRE(failedStat.invalid());
  CHECK(hasLegacyWarning(failedStat));
#endif
}
