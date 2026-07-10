#include "DelayVfd.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <fmt/core.h>

#include <H5Ppublic.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <thread>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
/**
 * @brief Writes a small .dream3d file containing a group, an attribute matrix,
 * two DataArrays, and a StringArray, then backdates its mtime so the cache's
 * young-mtime guard (files newer than k_MtimeTrustWindow are never trusted)
 * does not interfere with hit/miss assertions.
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
  fs::last_write_time(filePath, fs::last_write_time(filePath) - std::chrono::seconds(10));
  return filePath;
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

  // Every handout's DataArray owns a distinct store instance. Preflight arrays
  // are backed by EmptyDataStore, whose mutators throw (it holds no data to
  // mutate in place), so pointer inequality is the correct proxy for
  // "mutating one handout cannot reach another". A third fetch must likewise
  // get a store distinct from both prior handouts.
  Result<DataStructure> resultC = cache.fetch(filePath);
  REQUIRE(resultC.valid());
  DataStructure handoutC = std::move(resultC.value());
  auto* arrayC = handoutC.getDataAs<Float32Array>(floatsPath);
  REQUIRE(arrayC != nullptr);
  REQUIRE(arrayA->getIDataStore() != arrayB->getIDataStore());
  REQUIRE(arrayA->getIDataStore() != arrayC->getIDataStore());
  REQUIRE(arrayB->getIDataStore() != arrayC->getIDataStore());

  // StringArrays are backed by a real, mutable StringStore (of placeholder
  // empty strings under preflight), so isolation is proven behaviorally: an
  // in-place edit to one handout's StringArray must not reach another handout
  // or a later fetch. This exercises the StringStore deep copy directly —
  // without it, stringsB would observe "mutated".
  auto* stringsA = handoutA.getDataAs<StringArray>(stringsPath);
  auto* stringsB = handoutB.getDataAs<StringArray>(stringsPath);
  auto* stringsC = handoutC.getDataAs<StringArray>(stringsPath);
  REQUIRE(stringsA != nullptr);
  REQUIRE(stringsB != nullptr);
  REQUIRE(stringsC != nullptr);
  const std::vector<std::string> stringsBBefore = stringsB->values();
  const std::vector<std::string> stringsCBefore = stringsC->values();
  stringsA->setValue(0, "mutated");
  REQUIRE(stringsB->values() == stringsBBefore);
  REQUIRE(stringsC->values() == stringsCBefore);
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

  // Rewrite with different content, then stamp the file with a modification time
  // that is unambiguously different from the cached entry's while remaining
  // older than the trust window. The rewrite's natural (size, mtime) token is
  // not a reliable change signal: HDF5 aggregates small allocations into fixed
  // blocks, so the 10- and 20-tuple files can be byte-identical in size, and
  // filesystem timestamp granularity (notably ~15 ms on Windows) can give two
  // writes milliseconds apart the same mtime. An explicit distinct mtime makes
  // the staleness check deterministic on every platform.
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
  // Make the file look just-written.
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
  // Fill to capacity, then touch file 0 so file 1 is the LRU victim.
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

// Hidden benchmark ([.benchmark]): demonstrates the cache's effect under
// simulated network-storage latency. Excluded from default runs because it
// asserts on wall-clock behavior, which is only meaningful with the delay
// driver installed. Run manually:
//   ./simplnx_test "[.benchmark]"
TEST_CASE("Dream3dPreflightCache: benchmark under simulated storage latency", "[.benchmark]")
{
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();

  // Metadata-heavy file: many small arrays make traversal cost dominate.
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

  // Install the delay driver: every HDF5 open/read now costs 2 ms, like a
  // high-latency network mount.
  const hid_t driverId = DelayVfd::Register();
  REQUIRE(driverId != H5I_INVALID_HID);
  DelayVfd::SetDelayMicroseconds(2000);
  HDF5::FileIO::SetFaplConfigurator([driverId](hid_t faplId) { H5Pset_driver(faplId, driverId, nullptr); });

  using Clock = std::chrono::steady_clock;

  // Microsecond resolution: a warm cache hit is sub-millisecond, so timing in
  // milliseconds would round it to 0 and let a several-ms regression slip past
  // the ratio assertion below.
  // BEFORE (per-edit behavior without a cache): clear + fetch = full traversal.
  cache.clear();
  const auto coldStart = Clock::now();
  REQUIRE(cache.fetch(filePath).valid());
  const auto coldUs = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - coldStart).count();

  // AFTER: warm fetch = stat only.
  const auto warmStart = Clock::now();
  REQUIRE(cache.fetch(filePath).valid());
  const auto warmUs = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - warmStart).count();

  HDF5::FileIO::SetFaplConfigurator({});
  DelayVfd::SetDelayMicroseconds(0);

  WARN(fmt::format("PreflightCache benchmark @2ms/op: cold (uncached, = old per-edit cost) {} us | warm (cached) {} us", coldUs, warmUs));
  REQUIRE(warmUs * 10 < coldUs); // the cache must be at least 10x faster under latency
}
