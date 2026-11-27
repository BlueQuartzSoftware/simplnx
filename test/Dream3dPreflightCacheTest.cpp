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
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
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
constexpr StringLiteralType k_CacheTestFormat = "Task5-Cache-Test-Format";

class CacheRecordingResolver : public IDataStoreFormatResolver
{
public:
  std::string selected_format;
  std::string failure_message;
  std::optional<DataPath> failure_path;
  mutable std::vector<DataPath> paths;
  mutable std::vector<DataType> types;
  mutable std::vector<uint64> bytes;

  std::string resolveFormat([[maybe_unused]] const DataStructure& dataStructure, const DataPath& path, DataType numericType, uint64 logicalBytes) const override
  {
    paths.push_back(path);
    types.push_back(numericType);
    bytes.push_back(logicalBytes);
    if(!failure_message.empty() && (!failure_path.has_value() || path == *failure_path))
    {
      throw std::runtime_error(failure_message);
    }
    return selected_format;
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
                                                       [[maybe_unused]] const std::optional<ShapeType>& chunkShape,
                                                       [[maybe_unused]] DataStoreInitializationMode initializationMode) -> std::unique_ptr<IDataStore> {
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
  dream3d::Dream3dPreflightCache& cache = dream3d::Dream3dPreflightCache::Instance();
  std::shared_ptr<CacheRecordingResolver> resolver = std::make_shared<CacheRecordingResolver>();
  std::shared_ptr<usize> factory_calls = std::make_shared<usize>(0);
  std::shared_ptr<InMemoryFormatResolver> in_memory_resolver = std::make_shared<InMemoryFormatResolver>();

  CacheTestContext()
  {
    REQUIRE(Application::GetOrCreateInstance()->getIOCollection().addIOManager(std::make_shared<CacheTestDataIOManager>(factory_calls)).valid());
    cache.clear();
    cache.resetStats();
    DataStructure::SetDefaultFormatResolver(resolver);
  }

  ~CacheTestContext()
  {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    dream3d::Dream3dPreflightCache::SetForceFileMetadataFailure(false);
#endif
    cache.clear();
    DataStructure::SetDefaultFormatResolver(in_memory_resolver);
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
  auto* groupPtr = DataGroup::Create(dataStructure, "TestGroup");
  auto* attrMatPtr = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{numTuples}, groupPtr->getId());
  using FloatStoreType = DataStore<float32>;
  auto floatStore = std::make_shared<FloatStoreType>(ShapeType{numTuples}, ShapeType{3}, 1.5f);
  DataArray<float32>::Create(dataStructure, "Floats", floatStore, attrMatPtr->getId());
  auto intStore = std::make_shared<DataStore<int32>>(ShapeType{numTuples}, ShapeType{1}, 7);
  DataArray<int32>::Create(dataStructure, "Ints", intStore, attrMatPtr->getId());
  StringArray::CreateWithValues(dataStructure, "Strings", ShapeType{2}, {"alpha", "beta"}, groupPtr->getId());

  fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  fs::remove(filePath);
  Result<> writeResult = dream3d::WriteFile(filePath, dataStructure, Pipeline{}, false);
  REQUIRE(writeResult.valid());
  // Backdate mtime past the trust window so validation relies on the (size, mtime) token.
  AgeFile(filePath);
  return filePath;
}

fs::path WriteNumericPolicyFile(const std::string& fileName, DataPath& expectedFirstSharedPath)
{
  DataStructure dataStructure;
  auto* firstGroupPtr = DataGroup::Create(dataStructure, "First");
  auto* secondGroupPtr = DataGroup::Create(dataStructure, "Second");
  REQUIRE(firstGroupPtr != nullptr);
  REQUIRE(secondGroupPtr != nullptr);

  auto sharedStore = std::make_shared<DataStore<int32>>(ShapeType{4}, ShapeType{1}, 7);
  auto* sharedArrayPtr = Int32ArrayType::Create(dataStructure, "Shared", sharedStore, firstGroupPtr->getId());
  REQUIRE(sharedArrayPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(sharedArrayPtr->getId(), secondGroupPtr->getId()));
  REQUIRE_FALSE(sharedArrayPtr->getDataPaths().empty());
  expectedFirstSharedPath = sharedArrayPtr->getDataPaths().front();

  auto scalarStore = std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{}, 0);
  REQUIRE(Int32ArrayType::Create(dataStructure, "ScalarComponents", scalarStore) != nullptr);
  auto zeroStore = std::make_shared<DataStore<float32>>(ShapeType{2, 0}, ShapeType{3}, 0.0F);
  REQUIRE(Float32ArrayType::Create(dataStructure, "ExplicitZero", zeroStore) != nullptr);

  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  fs::remove(filePath);
  REQUIRE(dream3d::WriteFile(filePath, dataStructure, Pipeline{}, false).valid());
  AgeFile(filePath);
  return filePath;
}

fs::path WriteLegacyStatisticsWarningFile(const std::string& fileName)
{
  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  fs::remove(filePath);
  {
    auto file = hdf5::FileIO::WriteFile(filePath);
    REQUIRE(file.isValid());
    REQUIRE(file.writeStringAttribute("FileVersion", dream3d::k_LegacyFileVersion.str()).valid());
    auto containers = file.createGroup("DataContainers");
    auto container = containers.createGroup("StatsContainer");
    auto attributeMatrix = container.createGroup("StatsAM");
    REQUIRE(attributeMatrix.writeVectorAttribute("TupleDimensions", ShapeType{1}).valid());
    REQUIRE(attributeMatrix.writeScalarAttribute<uint32>("AttributeMatrixType", 13).valid());

    auto statistics = attributeMatrix.createGroup("StatisticsSource");
    REQUIRE(statistics.writeStringAttribute(constants::k_ObjectTypeTag, "Statistics").valid());
    auto values = statistics.createDataset("Values");
    const std::vector<float32> payload = {1.0F};
    REQUIRE(values.writeSpan<float32>({1, 1}, nonstd::span<const float32>(payload.data(), payload.size())).valid());
    REQUIRE(values.writeStringAttribute(constants::k_ObjectTypeTag, DataArray<float32>::GetTypeName()).valid());
    REQUIRE(values.writeVectorAttribute("TupleDimensions", ShapeType{1}).valid());
    REQUIRE(values.writeVectorAttribute("ComponentDimensions", ShapeType{1}).valid());
    REQUIRE(values.writeVectorAttribute<float32>("Mean", {4.0F, 5.0F, 6.0F}).valid());

    auto unsupported = attributeMatrix.createGroup("Unsupported");
    REQUIRE(unsupported.writeStringAttribute(constants::k_ObjectTypeTag, "UnsupportedLegacyObject").valid());
  }
  AgeFile(filePath);
  return filePath;
}

template <class T>
const DataArray<T>& RequirePlannedArray(const DataStructure& dataStructure, const DataPath& path, const std::string& expectedFormat, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  const auto* arrayPtr = dataStructure.getDataAs<DataArray<T>>(path);
  REQUIRE(arrayPtr != nullptr);
  REQUIRE(arrayPtr->getStoreType() == IDataStore::StoreType::Empty);
  CHECK(arrayPtr->getDataFormat() == expectedFormat);
  CHECK(arrayPtr->getTupleShape() == tupleShape);
  CHECK(arrayPtr->getComponentShape() == componentShape);
  return *arrayPtr;
}
} // namespace

TEST_CASE("Dream3dPreflightCache: miss then hit with equivalent structure", "[Dream3dPreflightCache]")
{
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
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
  auto fileReader = hdf5::FileIO::ReadFile(filePath);
  Result<DataStructure> direct = dream3d::ImportDataStructureFromFile(fileReader, true);
  REQUIRE(direct.valid());
  auto expectedPaths = direct.value().getAllDataPaths();
  auto actualPaths = second.value().getAllDataPaths();
  REQUIRE(actualPaths.size() == expectedPaths.size());
  for(const auto& path : expectedPaths)
  {
    INFO(path.toString());
    REQUIRE(second.value().containsData(path));
  }
  const auto* floatsPtr = second.value().getDataAs<Float32ArrayType>(DataPath({"TestGroup", "CellData", "Floats"}));
  REQUIRE(floatsPtr != nullptr);
  REQUIRE(floatsPtr->getTupleShape() == ShapeType{10});
  REQUIRE(floatsPtr->getComponentShape() == ShapeType{3});
}

TEST_CASE("Dream3dPreflightCache: fetch of a file containing a StringArray succeeds", "[Dream3dPreflightCache]")
{
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
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

  const auto* stringsPtr = result.value().getDataAs<StringArray>(stringsPath);
  REQUIRE(stringsPtr != nullptr);
  REQUIRE(stringsPtr->isPlaceholder());
  REQUIRE(stringsPtr->getNumberOfTuples() == 2);
}

TEST_CASE("Dream3dPreflightCache: missing file returns open error", "[Dream3dPreflightCache]")
{
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  Result<DataStructure> result = cache.fetch(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "does_not_exist.dream3d");
  REQUIRE(result.invalid());
  REQUIRE(result.errors()[0].code == -25);
}

TEST_CASE("Dream3dPreflightCache: handouts are isolated from master and each other", "[Dream3dPreflightCache]")
{
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
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

  auto* arrayAPtr = handoutA.getDataAs<Float32ArrayType>(floatsPath);
  auto* arrayBPtr = handoutB.getDataAs<Float32ArrayType>(floatsPath);
  REQUIRE(arrayAPtr != nullptr);
  REQUIRE(arrayBPtr != nullptr);

  // Each handout must own a distinct EmptyDataStore instance.
  // Pointer inequality proves that mutating one handout cannot reach another.
  Result<DataStructure> resultC = cache.fetch(filePath);
  REQUIRE(resultC.valid());
  DataStructure handoutC = std::move(resultC.value());
  auto* arrayCPtr = handoutC.getDataAs<Float32ArrayType>(floatsPath);
  REQUIRE(arrayCPtr != nullptr);
  REQUIRE(arrayAPtr->getIDataStore() != arrayBPtr->getIDataStore());
  REQUIRE(arrayAPtr->getIDataStore() != arrayCPtr->getIDataStore());
  REQUIRE(arrayBPtr->getIDataStore() != arrayCPtr->getIDataStore());

  // StringArray placeholders support resizing but not element access.
  // Resizing one handout must not change the tuple count of other handouts.
  auto* stringsAPtr = handoutA.getDataAs<StringArray>(stringsPath);
  auto* stringsBPtr = handoutB.getDataAs<StringArray>(stringsPath);
  auto* stringsCPtr = handoutC.getDataAs<StringArray>(stringsPath);
  REQUIRE(stringsAPtr != nullptr);
  REQUIRE(stringsBPtr != nullptr);
  REQUIRE(stringsCPtr != nullptr);
  REQUIRE(stringsAPtr->isPlaceholder());
  REQUIRE(stringsBPtr->isPlaceholder());
  REQUIRE(stringsCPtr->isPlaceholder());
  REQUIRE(stringsAPtr->resizeTuples(ShapeType{5}).valid());
  REQUIRE(stringsAPtr->getNumberOfTuples() == 5);
  REQUIRE(stringsBPtr->getNumberOfTuples() == 2);
  REQUIRE(stringsCPtr->getNumberOfTuples() == 2);
}

TEST_CASE("Dream3dPreflightCache: modified file is detected and re-read", "[Dream3dPreflightCache]")
{
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
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
  REQUIRE(refreshed.value().getDataAs<Float32ArrayType>(DataPath({"TestGroup", "CellData", "Floats"}))->getTupleShape() == ShapeType{20});
}

TEST_CASE("Dream3dPreflightCache: files younger than the trust window are never served from cache", "[Dream3dPreflightCache]")
{
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
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
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
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
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  std::vector<fs::path> files;
  files.reserve(dream3d::Dream3dPreflightCache::k_Capacity + 1);
  for(usize i = 0; i < dream3d::Dream3dPreflightCache::k_Capacity + 1; i++)
  {
    files.push_back(WriteTestFile(fmt::format("preflight_cache_lru_{}.dream3d", i)));
  }
  // Fill the cache, then touch file 0 so file 1 becomes the least-recently used entry.
  for(usize i = 0; i < dream3d::Dream3dPreflightCache::k_Capacity; i++)
  {
    REQUIRE(cache.fetch(files[i]).valid());
  }
  REQUIRE(cache.fetch(files[0]).valid());                                          // hit; refreshes recency
  REQUIRE(cache.fetch(files[dream3d::Dream3dPreflightCache::k_Capacity]).valid()); // miss; evicts files[1]

  cache.resetStats();
  REQUIRE(cache.fetch(files[0]).valid());
  REQUIRE(cache.hitCount() == 1); // still cached
  REQUIRE(cache.fetch(files[1]).valid());
  REQUIRE(cache.missCount() == 1); // was evicted
}

TEST_CASE("Dream3dPreflightCache: concurrent fetches are safe and consistent", "[Dream3dPreflightCache]")
{
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();

  const fs::path fileA = WriteTestFile("preflight_cache_conc_a.dream3d");
  const fs::path fileB = WriteTestFile("preflight_cache_conc_b.dream3d", 20);
  const DataPath floatsPath({"TestGroup", "CellData", "Floats"});

  constexpr usize k_NumThreads = 8;
  constexpr usize k_FetchesPerThread = 25;
  std::atomic<usize> failures{0};
  std::vector<std::thread> threads;
  threads.reserve(k_NumThreads);
  for(usize t = 0; t < k_NumThreads; t++)
  {
    threads.emplace_back([&, t]() {
      for(usize i = 0; i < k_FetchesPerThread; i++)
      {
        const fs::path& target = (t + i) % 2 == 0 ? fileA : fileB;
        const usize expectedTuples = (t + i) % 2 == 0 ? 10 : 20;
        Result<DataStructure> result = cache.fetch(target);
        if(result.invalid() || result.value().getDataAs<Float32ArrayType>(floatsPath) == nullptr ||
           result.value().getDataAs<Float32ArrayType>(floatsPath)->getTupleShape() != ShapeType{expectedTuples})
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
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
  cache.clear();

  // Many small arrays make metadata traversal dominate this fixture.
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "BenchGroup");
  for(usize i = 0; i < 150; i++)
  {
    auto store = std::make_shared<DataStore<float32>>(ShapeType{8}, ShapeType{1}, 0.0f);
    Float32ArrayType::Create(dataStructure, fmt::format("Array_{}", i), store, groupPtr->getId());
  }
  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "preflight_cache_benchmark.dream3d";
  fs::remove(filePath);
  REQUIRE(dream3d::WriteFile(filePath, dataStructure, Pipeline{}, false).valid());
  fs::last_write_time(filePath, fs::last_write_time(filePath) - std::chrono::seconds(10));

  // The delay driver adds 2 ms to each HDF5 open or read.
  const hid_t driverId = delay_vfd::Register();
  REQUIRE(driverId != H5I_INVALID_HID);
  delay_vfd::SetDelayMicroseconds(2000);
  hdf5::FileIO::SetFaplConfigurator([driverId](hid_t faplId) { H5Pset_driver(faplId, driverId, nullptr); });

  using ClockType = std::chrono::steady_clock;

  // Use microseconds because a warm cache hit is less than one millisecond.
  // The cold fetch performs a full metadata traversal.
  cache.clear();
  const auto coldStart = ClockType::now();
  REQUIRE(cache.fetch(filePath).valid());
  const auto coldUs = std::chrono::duration_cast<std::chrono::microseconds>(ClockType::now() - coldStart).count();

  // The warm fetch performs a timestamp check only.
  const auto warmStart = ClockType::now();
  REQUIRE(cache.fetch(filePath).valid());
  const auto warmUs = std::chrono::duration_cast<std::chrono::microseconds>(ClockType::now() - warmStart).count();

  hdf5::FileIO::SetFaplConfigurator({});
  delay_vfd::SetDelayMicroseconds(0);

  WARN(fmt::format("PreflightCache benchmark @2ms/op: cold (uncached, = old per-edit cost) {} us | warm (cached) {} us", coldUs, warmUs));
  REQUIRE(warmUs * 10 < coldUs); // the cache must be at least 10x faster under latency
}

TEST_CASE("StorageFormatPlan: cache isolates numeric list and string handout stores", "[Dream3dPreflightCache][StorageFormatPlan][T16]")
{
  DataStructure source;
  auto* numericPtr = Int32ArrayType::Create(source, "Numeric", std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{1}, 9));
  REQUIRE(numericPtr != nullptr);
  auto* listsPtr = NeighborList<int32>::Create(source, "Lists", ShapeType{3});
  REQUIRE(listsPtr != nullptr);
  listsPtr->setList(0, std::vector<int32>{3, 6});
  auto* stringsPtr = StringArray::CreateWithValues(source, "Strings", ShapeType{2}, {"alpha", "beta"});
  REQUIRE(stringsPtr != nullptr);
  const auto filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "preflight_store_copy_resolution.dream3d";
  auto fileWriteResult = dream3d::WriteFile(filePath, source, Pipeline{}, false);
  SIMPLNX_RESULT_REQUIRE_VALID(fileWriteResult);
  AgeFile(filePath);

  CacheTestContext context;
  context.resolver->selected_format.clear();
  auto first = context.cache.fetch(filePath);
  REQUIRE(first.valid());
  context.resolver->clearRecords();
  auto second = context.cache.fetch(filePath);
  REQUIRE(second.valid());
  REQUIRE(context.cache.hitCount() == 1);
  REQUIRE(std::find(context.resolver->paths.begin(), context.resolver->paths.end(), DataPath({"Numeric"})) != context.resolver->paths.end());
  REQUIRE(std::find(context.resolver->paths.begin(), context.resolver->paths.end(), DataPath({"Lists"})) != context.resolver->paths.end());
  auto* firstNumericPtr = first.value().getDataAs<Int32ArrayType>(DataPath({"Numeric"}));
  auto* secondNumericPtr = second.value().getDataAs<Int32ArrayType>(DataPath({"Numeric"}));
  auto* firstListsPtr = first.value().getDataAs<NeighborList<int32>>(DataPath({"Lists"}));
  auto* secondListsPtr = second.value().getDataAs<NeighborList<int32>>(DataPath({"Lists"}));
  auto* firstStringsPtr = first.value().getDataAs<StringArray>(DataPath({"Strings"}));
  auto* secondStringsPtr = second.value().getDataAs<StringArray>(DataPath({"Strings"}));
  REQUIRE(firstNumericPtr != nullptr);
  REQUIRE(secondNumericPtr != nullptr);
  REQUIRE(firstListsPtr != nullptr);
  REQUIRE(secondListsPtr != nullptr);
  REQUIRE(firstStringsPtr != nullptr);
  REQUIRE(secondStringsPtr != nullptr);
  REQUIRE(secondNumericPtr->getIDataStore()->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(secondNumericPtr->getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(secondNumericPtr->getIDataStore()->getDataFormat().empty());
  REQUIRE(firstNumericPtr->getIDataStore() != secondNumericPtr->getIDataStore());
  REQUIRE(dynamic_cast<EmptyListStore<int32>*>(secondListsPtr->getStore().get()) != nullptr);
  REQUIRE(firstListsPtr->getStore() != secondListsPtr->getStore());
  REQUIRE(firstStringsPtr->isPlaceholder());
  REQUIRE(secondStringsPtr->isPlaceholder());
  REQUIRE(firstStringsPtr->resizeTuples(ShapeType{5}).valid());
  REQUIRE(firstListsPtr->getStore()->resizeTuples(ShapeType{5}).valid());
  CHECK(secondStringsPtr->getTupleShape() == ShapeType{2});
  CHECK(secondListsPtr->getTupleShape() == ShapeType{3});
}

TEST_CASE("StorageFormatPlan: cache replans every fetch route with current policy", "[Dream3dPreflightCache][StorageFormatPlan][T16]")
{
  CacheTestContext context;
  DataPath firstSharedPath;
  const fs::path cachedFile = WriteNumericPolicyFile("preflight_cache_policy_routes.dream3d", firstSharedPath);
  const DataPath alternateSharedPath = firstSharedPath == DataPath({"First", "Shared"}) ? DataPath({"Second", "Shared"}) : DataPath({"First", "Shared"});
  const DataPath scalarPath({"ScalarComponents"});
  const DataPath zeroPath({"ExplicitZero"});

  context.resolver->selected_format = k_CacheTestFormat.str();
  auto cold = context.cache.fetch(cachedFile);
  SIMPLNX_RESULT_REQUIRE_VALID(cold);
  REQUIRE(context.cache.missCount() == 1);
  REQUIRE(context.cache.hitCount() == 0);
  const auto& coldShared = RequirePlannedArray<int32>(cold.value(), firstSharedPath, k_CacheTestFormat.str(), ShapeType{4}, ShapeType{1});
  const auto& coldScalar = RequirePlannedArray<int32>(cold.value(), scalarPath, k_CacheTestFormat.str(), ShapeType{3}, ShapeType{});
  const auto& coldZero = RequirePlannedArray<float32>(cold.value(), zeroPath, k_CacheTestFormat.str(), ShapeType{2, 0}, ShapeType{3});
  REQUIRE(*context.factory_calls == 0);

  context.resolver->clearRecords();
  context.resolver->selected_format.clear();
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

  context.resolver->selected_format = k_CacheTestFormat.str();
  auto newest = context.cache.fetch(cachedFile);
  SIMPLNX_RESULT_REQUIRE_VALID(newest);
  const auto& newestShared = RequirePlannedArray<int32>(newest.value(), firstSharedPath, k_CacheTestFormat.str(), ShapeType{4}, ShapeType{1});
  CHECK(newestShared.getIDataStore() != coldShared.getIDataStore());
  CHECK(newestShared.getIDataStore() != warmShared.getIDataStore());
  CHECK(warmShared.getDataFormat().empty());
  REQUIRE(*context.factory_calls == 0);

  DataPath recentFirstPath;
  const fs::path recentFile = WriteNumericPolicyFile("preflight_cache_policy_recent.dream3d", recentFirstPath);
  fs::last_write_time(recentFile, fs::file_time_type::clock::now());
  context.cache.resetStats();
  context.resolver->selected_format.clear();
  auto recentMemory = context.cache.fetch(recentFile);
  SIMPLNX_RESULT_REQUIRE_VALID(recentMemory);
  context.resolver->selected_format = k_CacheTestFormat.str();
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
  dream3d::Dream3dPreflightCache::SetForceFileMetadataFailure(true);
  auto restoreStat = MakeScopeGuard([]() noexcept { dream3d::Dream3dPreflightCache::SetForceFileMetadataFailure(false); });
  auto statBypass = context.cache.fetch(statFile);
  SIMPLNX_RESULT_REQUIRE_VALID(statBypass);
  REQUIRE(context.cache.missCount() == 1);
  REQUIRE(context.cache.hitCount() == 0);
  RequirePlannedArray<int32>(statBypass.value(), statFirstPath, k_CacheTestFormat.str(), ShapeType{4}, ShapeType{1});
  dream3d::Dream3dPreflightCache::SetForceFileMetadataFailure(false);
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
  context.resolver->selected_format.clear();
  REQUIRE(context.cache.fetch(filePath).valid());

  context.cache.resetStats();
  context.resolver->clearRecords();
  context.resolver->failure_message = "injected cache resolver failure";
  auto failed = context.cache.fetch(filePath);
  REQUIRE(failed.invalid());
  REQUIRE_FALSE(failed.errors().empty());
  CHECK(failed.errors().front().code == -10601);
  CHECK(failed.errors().front().message.find(filePath.string()) != std::string::npos);
  CHECK(failed.errors().front().message.find(firstSharedPath.toString()) != std::string::npos);
  CHECK(failed.errors().front().message.find(context.resolver->failure_message) != std::string::npos);
  REQUIRE(context.cache.hitCount() == 1);
  REQUIRE(context.cache.missCount() == 0);
  REQUIRE(*context.factory_calls == 0);

  context.resolver->failure_message.clear();
  context.resolver->selected_format = k_CacheTestFormat.str();
  auto recovered = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(recovered);
  REQUIRE(context.cache.hitCount() == 2);
  REQUIRE(context.cache.missCount() == 0);
  RequirePlannedArray<int32>(recovered.value(), firstSharedPath, k_CacheTestFormat.str(), ShapeType{4}, ShapeType{1});
}

TEST_CASE("StorageFormatPlan: cache preserves populated legacy Statistics values and isolation", "[Dream3dPreflightCache][StorageFormatPlan][T16]")
{
  CacheTestContext context;
  context.resolver->selected_format.clear();
  const fs::path filePath = WriteLegacyStatisticsWarningFile("preflight_cache_statistics_isolation.dream3d");
  const DataPath statisticsPath({"StatsContainer", "Statistics", "Values_Mean"});

  auto cold = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(cold);
  auto* coldArrayPtr = cold.value().getDataAs<Float32ArrayType>(statisticsPath);
  REQUIRE(coldArrayPtr != nullptr);
  REQUIRE(coldArrayPtr->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(std::find(context.resolver->paths.begin(), context.resolver->paths.end(), statisticsPath) != context.resolver->paths.end());
  CHECK(coldArrayPtr->getDataStoreRef().getValue(0) == 4.0F);
  coldArrayPtr->getDataStoreRef().setValue(0, 99.0F);

  auto warm = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(warm);
  auto* warmArrayPtr = warm.value().getDataAs<Float32ArrayType>(statisticsPath);
  REQUIRE(warmArrayPtr != nullptr);
  REQUIRE(warmArrayPtr->getIDataStore() != coldArrayPtr->getIDataStore());
  CHECK(warmArrayPtr->getDataStoreRef().getValue(0) == 4.0F);
  warmArrayPtr->getDataStoreRef().setValue(0, 88.0F);

  auto third = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(third);
  auto* thirdArrayPtr = third.value().getDataAs<Float32ArrayType>(statisticsPath);
  REQUIRE(thirdArrayPtr != nullptr);
  REQUIRE(thirdArrayPtr->getIDataStore() != coldArrayPtr->getIDataStore());
  REQUIRE(thirdArrayPtr->getIDataStore() != warmArrayPtr->getIDataStore());
  CHECK(thirdArrayPtr->getDataStoreRef().getValue(0) == 4.0F);

  context.cache.resetStats();
  context.resolver->selected_format = k_CacheTestFormat.str();
  *context.factory_calls = 0;
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
  REQUIRE(*context.factory_calls == 1);

  context.resolver->selected_format.clear();
  auto restored = context.cache.fetch(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(restored);
  REQUIRE_NOTHROW(restored.value().getDataRefAs<Float32ArrayType>(statisticsPath));
  CHECK(restored.value().getDataRefAs<Float32ArrayType>(statisticsPath).getDataStoreRef().getValue(0) == 4.0F);
}

TEST_CASE("StorageFormatPlan: disk warnings survive cache bypass preparation", "[Dream3dPreflightCache][StorageFormatPlan][T16][T30]")
{
  auto hasLegacyWarning = [](const Result<DataStructure>& result) {
    return std::any_of(result.warnings().begin(), result.warnings().end(), [](const Warning& warning) { return warning.code == -298012; });
  };

  CacheTestContext context;
  context.resolver->selected_format.clear();

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
  context.resolver->failure_message = "injected cache resolver failure";
  auto failedRecent = context.cache.fetch(failedRecentFile);
  REQUIRE(failedRecent.invalid());
  CHECK(hasLegacyWarning(failedRecent));

  context.resolver->failure_message.clear();
  const fs::path failedColdFile = WriteLegacyStatisticsWarningFile("preflight_cache_warning_cold_failure.dream3d");
  context.resolver->failure_message = "injected cache resolver failure";
  auto failedCold = context.cache.fetch(failedColdFile);
  REQUIRE(failedCold.invalid());
  CHECK(hasLegacyWarning(failedCold));

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  context.resolver->failure_message.clear();
  const fs::path statFile = WriteLegacyStatisticsWarningFile("preflight_cache_warning_stat.dream3d");
  dream3d::Dream3dPreflightCache::SetForceFileMetadataFailure(true);
  auto restoreStat = MakeScopeGuard([]() noexcept { dream3d::Dream3dPreflightCache::SetForceFileMetadataFailure(false); });
  auto stat = context.cache.fetch(statFile);
  SIMPLNX_RESULT_REQUIRE_VALID(stat);
  CHECK(hasLegacyWarning(stat));

  context.resolver->failure_message = "injected cache resolver failure";
  auto failedStat = context.cache.fetch(statFile);
  REQUIRE(failedStat.invalid());
  CHECK(hasLegacyWarning(failedStat));
#endif
}

TEST_CASE("C8 neutral metadata bypasses process policy on every cache route", "[C8][Dream3dPreflightCache]")
{
  CacheTestContext context;
  const int route = GENERATE(0, 1, 2);
  CAPTURE(route);
  const auto path = WriteTestFile("neutral_cache_routes.dream3d", 4);
  const auto cleanup = MakeScopeGuard([&path]() noexcept {
    std::error_code error;
    fs::remove(path, error);
  });
  if(route == 1)
  {
    fs::last_write_time(path, fs::file_time_type::clock::now());
  }
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  dream3d::Dream3dPreflightCache::SetForceFileMetadataFailure(route == 2);
#endif
  context.resolver->clearRecords();
  context.resolver->failure_message = "neutral cache must bypass process policy";
  auto first = context.cache.fetchNeutralMetadata(path);
  auto second = context.cache.fetchNeutralMetadata(path);
  SIMPLNX_RESULT_REQUIRE_VALID(first);
  SIMPLNX_RESULT_REQUIRE_VALID(second);
  CHECK(first.value().file_version == dream3d::k_CurrentFileVersion);
  CHECK(second.value().file_version == first.value().file_version);
  CHECK(context.resolver->paths.empty());
  CHECK(context.cache.missCount() == (route == 0 ? 1 : 2));
  CHECK(context.cache.hitCount() == (route == 0 ? 1 : 0));
  const DataPath arrayPath({"TestGroup", "CellData", "Ints"});
  const auto& firstArray = RequirePlannedArray<int32>(first.value().data_structure, arrayPath, "", {4}, {1});
  const auto& secondArray = RequirePlannedArray<int32>(second.value().data_structure, arrayPath, "", {4}, {1});
  CHECK(firstArray.getIDataStore() != secondArray.getIDataStore());
  CHECK(firstArray.getId() == secondArray.getId());
  CHECK(first.value().data_structure.getData(firstArray.getId()) == &firstArray);
  CHECK(second.value().data_structure.getData(secondArray.getId()) == &secondArray);
  unit_test::CheckArraysInheritTupleDims(second.value().data_structure);
}

TEST_CASE("C8 neutral and ordinary cache modes stay separate and invalidate together", "[C8][Dream3dPreflightCache]")
{
  CacheTestContext context;
  const auto path = WriteTestFile("neutral_cache_mode_separation.dream3d", 4);
  const auto cleanup = MakeScopeGuard([&path]() noexcept {
    std::error_code error;
    fs::remove(path, error);
  });
  REQUIRE(context.cache.fetch(path).valid());
  context.resolver->clearRecords();
  context.resolver->failure_message = "ordinary mode still resolves policy";
  REQUIRE(context.cache.fetchNeutralMetadata(path).valid());
  REQUIRE(context.cache.fetchNeutralMetadata(path).valid());
  CHECK(context.resolver->paths.empty());
  CHECK(context.cache.missCount() == 2);
  CHECK(context.cache.hitCount() == 1);
  auto ordinary = context.cache.fetch(path);
  REQUIRE(ordinary.invalid());
  CHECK_FALSE(context.resolver->paths.empty());
  CHECK(context.cache.hitCount() == 2);
  context.resolver->failure_message.clear();
  context.cache.invalidate(path);
  REQUIRE(context.cache.fetch(path).valid());
  REQUIRE(context.cache.fetchNeutralMetadata(path).valid());
  CHECK(context.cache.missCount() == 4);
}

TEST_CASE("C8 neutral legacy metadata replays warnings and isolates eager values", "[C8][Dream3dPreflightCache]")
{
  CacheTestContext context;
  const int route = GENERATE(0, 1, 2);
  CAPTURE(route);
  const auto path = WriteLegacyStatisticsWarningFile("neutral_cache_legacy_warnings.dream3d");
  const auto cleanup = MakeScopeGuard([&path]() noexcept {
    std::error_code error;
    fs::remove(path, error);
  });
  context.resolver->failure_message = "legacy neutral metadata must bypass process policy";
  context.resolver->clearRecords();
  if(route == 1)
  {
    fs::last_write_time(path, fs::file_time_type::clock::now());
  }
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  dream3d::Dream3dPreflightCache::SetForceFileMetadataFailure(route == 2);
#endif
  const DataPath valuesPath({"StatsContainer", "Statistics", "Values_Mean"});
  auto cold = context.cache.fetchNeutralMetadata(path);
  SIMPLNX_RESULT_REQUIRE_VALID(cold);
  REQUIRE(cold.value().file_version == dream3d::k_LegacyFileVersion);
  REQUIRE(cold.warnings().size() == 1);
  CHECK(cold.warnings().front().code == -298012);
  auto* firstPtr = cold.value().data_structure.getDataAs<Float32ArrayType>(valuesPath);
  REQUIRE(firstPtr != nullptr);
  REQUIRE(firstPtr->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(firstPtr->size() == 3);
  CHECK((*firstPtr)[0] == 4.0F);
  CHECK((*firstPtr)[1] == 5.0F);
  CHECK((*firstPtr)[2] == 6.0F);
  (*firstPtr)[0] = 99.0F;
  auto warm = context.cache.fetchNeutralMetadata(path);
  SIMPLNX_RESULT_REQUIRE_VALID(warm);
  REQUIRE(warm.value().file_version == dream3d::k_LegacyFileVersion);
  REQUIRE(warm.warnings().size() == cold.warnings().size());
  for(usize index = 0; index < cold.warnings().size(); ++index)
  {
    CHECK(warm.warnings()[index].code == cold.warnings()[index].code);
    CHECK(warm.warnings()[index].message == cold.warnings()[index].message);
  }
  const auto* secondPtr = warm.value().data_structure.getDataAs<Float32ArrayType>(valuesPath);
  REQUIRE(secondPtr != nullptr);
  REQUIRE(secondPtr->size() == 3);
  REQUIRE(firstPtr->getIDataStore() != secondPtr->getIDataStore());
  CHECK((*secondPtr)[0] == 4.0F);
  CHECK((*secondPtr)[1] == 5.0F);
  CHECK((*secondPtr)[2] == 6.0F);
  CHECK(context.resolver->paths.empty());
  CHECK(context.cache.hitCount() == (route == 0 ? 1 : 0));
  CHECK(context.cache.missCount() == (route == 0 ? 1 : 2));
  unit_test::CheckArraysInheritTupleDims(warm.value().data_structure);
}

TEST_CASE("C8 neutral cache isolates list and string placeholders", "[C8][Dream3dPreflightCache]")
{
  CacheTestContext context;
  DataStructure source;
  source.setFormatResolver(context.in_memory_resolver);
  auto* listsPtr = NeighborList<int32>::Create(source, "Lists", ShapeType{3});
  REQUIRE(listsPtr != nullptr);
  listsPtr->setList(0, std::vector<int32>{3, 6});
  REQUIRE(StringArray::CreateWithValues(source, "Strings", ShapeType{2}, {"alpha", "beta"}) != nullptr);
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "neutral_list_string.dream3d";
  const auto cleanup = MakeScopeGuard([&path]() noexcept {
    std::error_code error;
    fs::remove(path, error);
  });
  auto write = dream3d::WriteFile(path, source);
  SIMPLNX_RESULT_REQUIRE_VALID(write);
  AgeFile(path);
  context.resolver->clearRecords();
  context.resolver->failure_message = "neutral list metadata must bypass process policy";
  auto first = context.cache.fetchNeutralMetadata(path);
  auto second = context.cache.fetchNeutralMetadata(path);
  SIMPLNX_RESULT_REQUIRE_VALID(first);
  SIMPLNX_RESULT_REQUIRE_VALID(second);
  auto* firstListPtr = first.value().data_structure.getDataAs<NeighborList<int32>>(DataPath({"Lists"}));
  const auto* secondListPtr = second.value().data_structure.getDataAs<NeighborList<int32>>(DataPath({"Lists"}));
  auto* firstStringPtr = first.value().data_structure.getDataAs<StringArray>(DataPath({"Strings"}));
  const auto* secondStringPtr = second.value().data_structure.getDataAs<StringArray>(DataPath({"Strings"}));
  REQUIRE(firstListPtr != nullptr);
  REQUIRE(secondListPtr != nullptr);
  REQUIRE(firstStringPtr != nullptr);
  REQUIRE(secondStringPtr != nullptr);
  CHECK(firstListPtr->getStore() != secondListPtr->getStore());
  REQUIRE(dynamic_cast<EmptyListStore<int32>*>(secondListPtr->getStore().get()) != nullptr);
  REQUIRE(firstStringPtr->isPlaceholder());
  REQUIRE(secondStringPtr->isPlaceholder());
  REQUIRE(firstListPtr->getStore()->resizeTuples({5}).valid());
  REQUIRE(firstStringPtr->resizeTuples({5}).valid());
  CHECK(secondListPtr->getTupleShape() == ShapeType{3});
  CHECK(secondStringPtr->getTupleShape() == ShapeType{2});
  CHECK(context.resolver->paths.empty());
  unit_test::CheckArraysInheritTupleDims(second.value().data_structure);
}

TEST_CASE("C8 NX Statistics remains deferred metadata", "[C8][Dream3dPreflightCache]")
{
  CacheTestContext context;
  DataStructure source;
  source.setFormatResolver(context.in_memory_resolver);
  auto* containerPtr = DataGroup::Create(source, "StatsContainer");
  REQUIRE(containerPtr != nullptr);
  auto* statisticsPtr = DataGroup::Create(source, "Statistics", containerPtr->getId());
  REQUIRE(statisticsPtr != nullptr);
  auto values = std::make_shared<DataStore<float32>>(ShapeType{3}, ShapeType{1}, 17.0F);
  REQUIRE(Float32ArrayType::Create(source, "Values_Mean", values, statisticsPtr->getId()) != nullptr);
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "c8_nx_statistics.dream3d";
  const auto cleanup = MakeScopeGuard([&path]() noexcept {
    std::error_code error;
    fs::remove(path, error);
  });
  auto write = dream3d::WriteFile(path, source);
  SIMPLNX_RESULT_REQUIRE_VALID(write);
  AgeFile(path);
  context.resolver->failure_message = "NX Statistics metadata must not consult process policy";
  context.resolver->clearRecords();
  const DataPath valuesPath({"StatsContainer", "Statistics", "Values_Mean"});
  auto cold = context.cache.fetchNeutralMetadata(path);
  auto warm = context.cache.fetchNeutralMetadata(path);
  SIMPLNX_RESULT_REQUIRE_VALID(cold);
  SIMPLNX_RESULT_REQUIRE_VALID(warm);
  REQUIRE(cold.value().file_version == dream3d::k_CurrentFileVersion);
  REQUIRE(warm.value().file_version == dream3d::k_CurrentFileVersion);
  const auto& first = RequirePlannedArray<float32>(cold.value().data_structure, valuesPath, "", {3}, {1});
  const auto& second = RequirePlannedArray<float32>(warm.value().data_structure, valuesPath, "", {3}, {1});
  CHECK(first.getIDataStore() != second.getIDataStore());
  CHECK(cold.warnings().empty());
  CHECK(warm.warnings().empty());
  CHECK(context.resolver->paths.empty());

  DataStructure destination;
  destination.setFormatResolver(context.in_memory_resolver);
  ImportH5ObjectPathsAction action(path, {DataPath({"StatsContainer"}), DataPath({"StatsContainer", "Statistics"}), valuesPath});
  auto preflight = action.apply(destination, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight);
  RequirePlannedArray<float32>(destination, valuesPath, "", {3}, {1});
  unit_test::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("C8 legacy Statistics action preserves literal values and warning on planning failure", "[C8][Dream3dPreflightCache][StorageFormatPlan]")
{
  CacheTestContext context;
  const bool warmCache = GENERATE(false, true);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool failDestination = GENERATE(false, true);
  CAPTURE(warmCache, mode, failDestination);
  const auto path = WriteLegacyStatisticsWarningFile("c8_legacy_statistics_action.dream3d");
  const auto cleanup = MakeScopeGuard([&path]() noexcept {
    std::error_code error;
    fs::remove(path, error);
  });
  if(warmCache)
  {
    auto warm = context.cache.fetchNeutralMetadata(path);
    SIMPLNX_RESULT_REQUIRE_VALID(warm);
  }
  context.resolver->failure_message = "legacy action must not consult process policy";
  context.resolver->clearRecords();
  const DataPath valuesPath({"StatsContainer", "Statistics", "Values_Mean"});
  const DataPath datasetPath({"StatsContainer", "Statistics", "Values"});
  auto destinationResolver = std::make_shared<CacheRecordingResolver>();
  if(failDestination)
  {
    destinationResolver->failure_message = "injected legacy destination planning failure";
    destinationResolver->failure_path = datasetPath;
  }
  DataStructure destination;
  destination.setFormatResolver(destinationResolver);
  auto* markerPtr = DataGroup::Create(destination, "DestinationMarker");
  REQUIRE(markerPtr != nullptr);
  const auto markerId = markerPtr->getId();
  const auto originalPaths = destination.getAllDataPaths();
  ImportH5ObjectPathsAction action(path, {DataPath({"StatsContainer"}), DataPath({"StatsContainer", "Statistics"}), valuesPath, datasetPath});
  auto result = action.apply(destination, mode);
  REQUIRE(result.warnings().size() == 1);
  CHECK(result.warnings().front().code == -298012);
  CHECK(context.resolver->paths.empty());
  const auto attributeDecisions = std::count(destinationResolver->paths.begin(), destinationResolver->paths.end(), valuesPath);
  const auto datasetDecisions = std::count(destinationResolver->paths.begin(), destinationResolver->paths.end(), datasetPath);
  CHECK(datasetDecisions == 1);
  CHECK(destinationResolver->paths.size() == static_cast<usize>(attributeDecisions + datasetDecisions));
  CHECK(destination.getData(markerId) == markerPtr);
  if(failDestination)
  {
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    CHECK(result.errors().front().message.find(destinationResolver->failure_message) != std::string::npos);
    CHECK(result.errors().front().message.find(datasetPath.toString()) != std::string::npos);
    CHECK(attributeDecisions <= 1);
    CHECK(destination.getAllDataPaths() == originalPaths);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(attributeDecisions == 1);
    auto* valuesPtr = destination.getDataAs<Float32ArrayType>(valuesPath);
    auto* datasetPtr = destination.getDataAs<Float32ArrayType>(datasetPath);
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(datasetPtr != nullptr);
    REQUIRE(valuesPtr->getStoreType() == IDataStore::StoreType::InMemory);
    REQUIRE(datasetPtr->getStoreType() == IDataStore::StoreType::InMemory);
    REQUIRE(valuesPtr->size() == 3);
    REQUIRE(datasetPtr->size() == 1);
    CHECK((*datasetPtr)[0] == 1.0F);
    CHECK((*valuesPtr)[0] == 4.0F);
    CHECK((*valuesPtr)[1] == 5.0F);
    CHECK((*valuesPtr)[2] == 6.0F);
    (*valuesPtr)[0] = -99.0F;
    (*datasetPtr)[0] = -88.0F;
    auto laterHandout = context.cache.fetchNeutralMetadata(path);
    SIMPLNX_RESULT_REQUIRE_VALID(laterHandout);
    const auto* originalPtr = laterHandout.value().data_structure.getDataAs<Float32ArrayType>(valuesPath);
    const auto* originalDatasetPtr = laterHandout.value().data_structure.getDataAs<Float32ArrayType>(datasetPath);
    REQUIRE(originalPtr != nullptr);
    REQUIRE(originalDatasetPtr != nullptr);
    REQUIRE(originalPtr->getIDataStore() != valuesPtr->getIDataStore());
    REQUIRE(originalDatasetPtr->getIDataStore() != datasetPtr->getIDataStore());
    REQUIRE(originalDatasetPtr->size() == 1);
    CHECK((*originalDatasetPtr)[0] == 1.0F);
    CHECK((*originalPtr)[0] == 4.0F);
    CHECK((*originalPtr)[1] == 5.0F);
    CHECK((*originalPtr)[2] == 6.0F);
    unit_test::CheckArraysInheritTupleDims(destination);
  }
}

TEST_CASE("C8 neutral and ordinary cache masters share one LRU capacity", "[C8][Dream3dPreflightCache]")
{
  CacheTestContext context;
  std::vector<fs::path> files;
  const auto cleanup = MakeScopeGuard([&files]() noexcept {
    for(const auto& path : files)
    {
      std::error_code error;
      fs::remove(path, error);
    }
  });
  for(usize fileIdx = 0; fileIdx < dream3d::Dream3dPreflightCache::k_Capacity; ++fileIdx)
  {
    files.push_back(WriteTestFile(fmt::format("c8_mixed_cache_capacity_{}.dream3d", fileIdx), 4));
  }
  REQUIRE(context.cache.fetchNeutralMetadata(files.front()).valid());
  REQUIRE(context.cache.fetch(files.front()).valid());
  for(usize fileIdx = 1; fileIdx + 1 < files.size(); ++fileIdx)
  {
    REQUIRE(context.cache.fetch(files[fileIdx]).valid());
  }
  REQUIRE(context.cache.missCount() == dream3d::Dream3dPreflightCache::k_Capacity);
  REQUIRE(context.cache.fetchNeutralMetadata(files.back()).valid());
  context.cache.resetStats();
  REQUIRE(context.cache.fetch(files.front()).valid());
  CHECK(context.cache.hitCount() == 1);
  CHECK(context.cache.missCount() == 0);
  REQUIRE(context.cache.fetchNeutralMetadata(files.front()).valid());
  CHECK(context.cache.hitCount() == 1);
  CHECK(context.cache.missCount() == 1);
}
