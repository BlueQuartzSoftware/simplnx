#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/UnitTest/HDF5DatasetProbe.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <H5Dpublic.h>
#include <H5Ipublic.h>
#include <H5Lpublic.h>
#include <H5Ppublic.h>

#include <catch2/catch.hpp>

#include <nonstd/span.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <limits>
#include <mutex>
#include <numeric>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
// Builds a small two-dataset fixture file the concurrency test reads back. Returns the
// path on success. Kept separate so the writer's HDF5 work happens once, single-threaded,
// before any reader thread is spawned.
std::filesystem::path BuildFixtureFile(const std::filesystem::path& path, usize numElems)
{
  std::vector<int32> sourceA(numElems);
  std::iota(sourceA.begin(), sourceA.end(), 0);
  std::vector<int32> sourceB(numElems);
  std::iota(sourceB.begin(), sourceB.end(), 1000);

  auto file = HDF5::FileIO::WriteFile(path);
  REQUIRE(file.isValid());
  auto group = file.createGroup("g");
  REQUIRE(group.isValid());

  HDF5::ObjectIO::DimsType dims{numElems};

  auto dsA = group.createDataset("a");
  REQUIRE(dsA.writeSpan<int32>(dims, nonstd::span<const int32>(sourceA.data(), sourceA.size())).valid());

  auto dsB = group.createDataset("b");
  REQUIRE(dsB.writeSpan<int32>(dims, nonstd::span<const int32>(sourceB.data(), sourceB.size())).valid());

  return path;
}

template <class OperationT>
bool BlocksOnApiLock(OperationT&& operation)
{
  std::promise<void> startedPromise;
  std::future<void> started = startedPromise.get_future();
  std::promise<void> finishedPromise;
  std::future<void> finished = finishedPromise.get_future();
  std::exception_ptr workerException;

  std::unique_lock<std::mutex> apiLock(HDF5::Support::ApiLock());
  std::thread worker([&]() {
    startedPromise.set_value();
    try
    {
      operation();
    } catch(...)
    {
      workerException = std::current_exception();
    }
    finishedPromise.set_value();
  });

  started.wait();
  const bool blocked = finished.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout;
  apiLock.unlock();
  const bool completed = finished.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
  worker.join();

  if(workerException != nullptr)
  {
    std::rethrow_exception(workerException);
  }
  return blocked && completed;
}

class DatasetIOProbe : public HDF5::DatasetIO
{
public:
  using HDF5::DatasetIO::CreateH5DatasetChunkProperties;
};
} // namespace

TEST_CASE("HDF5 standalone helpers self-lock their C API calls", "[simplnx][HDF5][concurrency]")
{
  const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "h5io_helper_locking.h5";
  std::filesystem::remove(tmp);

  auto file = HDF5::FileIO::WriteFile(tmp);
  REQUIRE(file.isValid());
  auto group = file.createGroup("g");
  REQUIRE(group.isValid());
  auto dataset = group.createDataset("d");
  const std::vector<int32> values = {1, 2, 3, 4};
  REQUIRE(dataset.writeSpan<int32>({values.size()}, nonstd::span<const int32>(values.data(), values.size())).valid());
  REQUIRE(dataset.writeScalarAttribute<int32>("answer", 42).valid());

  const hid_t datasetId = dataset.getId();
  const hid_t typeId = dataset.getTypeId();
  REQUIRE(datasetId > 0);
  REQUIRE(typeId > 0);

  SECTION("GetPathFromId")
  {
    std::string path;
    REQUIRE(BlocksOnApiLock([&]() { path = HDF5::GetPathFromId(datasetId); }));
    REQUIRE(path == "/g/d");
  }

  SECTION("GetNameFromId")
  {
    std::string name;
    REQUIRE(BlocksOnApiLock([&]() { name = HDF5::GetNameFromId(datasetId); }));
    REQUIRE(name == "d");
  }

  SECTION("getTypeFromId")
  {
    HDF5::Type type = HDF5::Type::unknown;
    REQUIRE(BlocksOnApiLock([&]() { type = HDF5::getTypeFromId(typeId); }));
    REQUIRE(type == HDF5::Type::int32);
  }

  SECTION("FindAttribute")
  {
    herr_t found = -1;
    REQUIRE(BlocksOnApiLock([&]() { found = HDF5::Support::FindAttribute(datasetId, "answer"); }));
    REQUIRE(found == 1);
  }

  SECTION("Support object queries")
  {
    bool isGroup = true;
    std::string objectPath;
    REQUIRE(BlocksOnApiLock([&]() { isGroup = HDF5::Support::IsGroup(group.getId(), "d"); }));
    REQUIRE_FALSE(isGroup);
    REQUIRE(BlocksOnApiLock([&]() { objectPath = HDF5::Support::GetObjectPath(datasetId); }));
    REQUIRE(objectPath == "g/d");
  }

  SECTION("Support dataset type queries")
  {
    hid_t queriedTypeId = H5I_INVALID_HID;
    REQUIRE(BlocksOnApiLock([&]() { queriedTypeId = HDF5::Support::GetDatasetType(group.getId(), "d"); }));
    REQUIRE(queriedTypeId > 0);
    std::string typeName;
    REQUIRE(BlocksOnApiLock([&]() { typeName = HDF5::Support::StringForHDFType(queriedTypeId); }));
    REQUIRE(typeName == "H5T_NATIVE_INT32");
    std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
    REQUIRE(H5Tclose(queriedTypeId) >= 0);
  }

  SECTION("SetFaplConfigurator")
  {
    REQUIRE(BlocksOnApiLock([]() { HDF5::FileIO::SetFaplConfigurator({}); }));
  }

  SECTION("ProbeHdf5Dataset")
  {
    std::optional<UnitTest::DatasetProbeInfo> probe;
    REQUIRE(BlocksOnApiLock([&]() { probe = UnitTest::ProbeHdf5Dataset(tmp, "/g/d"); }));
    REQUIRE(probe.has_value());
  }

  {
    std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
    H5Tclose(typeId);
  }
}

TEST_CASE("HDF5 chunk helpers self-lock their C API calls", "[simplnx][HDF5][concurrency]")
{
  const std::filesystem::path tmp = BuildFixtureFile(std::filesystem::temp_directory_path() / "h5io_chunk_helper_locking.h5", 4);
  auto file = HDF5::FileIO::ReadFile(tmp);
  REQUIRE(file.isValid());
  auto group = file.openGroup("g");
  REQUIRE(group.isValid());
  auto dataset = group.openDataset("a");
  REQUIRE(dataset.getId() > 0);

  SECTION("CreateH5DatasetChunkProperties")
  {
    hid_t propertiesId = H5I_INVALID_HID;
    REQUIRE(BlocksOnApiLock([&]() { propertiesId = DatasetIOProbe::CreateH5DatasetChunkProperties({4}); }));
    REQUIRE(propertiesId > 0);
    std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
    REQUIRE(H5Pclose(propertiesId) >= 0);
  }

  SECTION("closeChunkedDataset")
  {
    HDF5::ChunkedDataInfo chunkInfo;
    {
      std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
      const hsize_t dims = 4;
      chunkInfo.dataspaceId = H5Screate_simple(1, &dims, nullptr);
    }
    REQUIRE(chunkInfo.dataspaceId > 0);
    Result<> closeResult;
    REQUIRE(BlocksOnApiLock([&]() { closeResult = dataset.closeChunkedDataset(chunkInfo); }));
    REQUIRE(closeResult.valid());
  }

  SECTION("readChunk")
  {
    HDF5::ChunkedDataInfo chunkInfo;
    chunkInfo.dataType = H5T_NATIVE_INT32;
    chunkInfo.datasetId = dataset.getId();
    {
      std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
      chunkInfo.dataspaceId = H5Dget_space(chunkInfo.datasetId);
    }
    REQUIRE(chunkInfo.dataspaceId > 0);

    std::vector<int32> values(4);
    Result<> readResult;
    const std::vector<usize> offset = {0};
    REQUIRE(BlocksOnApiLock([&]() { readResult = dataset.readChunk<int32>(chunkInfo, {4}, nonstd::span<int32>(values.data(), values.size()), {4}, offset); }));
    REQUIRE(readResult.valid());
    REQUIRE(values == std::vector<int32>{0, 1, 2, 3});

    std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
    REQUIRE(H5Sclose(chunkInfo.dataspaceId) >= 0);
  }
}

TEST_CASE("HDF5 numeric attribute writers serialize complete HDF5 lifecycles", "[simplnx][HDF5][concurrency]")
{
  constexpr int k_Threads = 8;
  constexpr int k_Passes = 50;
  std::atomic<int> mismatches{0};
  std::vector<std::filesystem::path> paths;
  std::vector<std::thread> threads;
  paths.reserve(k_Threads);
  threads.reserve(k_Threads);

  for(int threadIndex = 0; threadIndex < k_Threads; ++threadIndex)
  {
    paths.push_back(std::filesystem::temp_directory_path() / fmt::format("h5io_attribute_writer_locking_{}.h5", threadIndex));
    std::filesystem::remove(paths.back());
    threads.emplace_back([&, threadIndex]() {
      auto file = HDF5::FileIO::WriteFile(paths[threadIndex]);
      auto group = file.createGroup("g");
      for(int pass = 0; pass < k_Passes; ++pass)
      {
        const int32 scalar = threadIndex * k_Passes + pass;
        const std::vector<int32> vector = {scalar, scalar + 1};
        if(group.writeScalarAttribute<int32>("scalar", scalar).invalid() || group.writeVectorAttribute<int32>("vector", vector).invalid())
        {
          mismatches.fetch_add(1, std::memory_order_relaxed);
          continue;
        }
        const auto scalarResult = group.readScalarAttribute<int32>("scalar");
        const auto vectorResult = group.readVectorAttribute<int32>("vector");
        if(scalarResult.invalid() || scalarResult.value() != scalar || vectorResult.invalid() || vectorResult.value() != vector)
        {
          mismatches.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  for(auto& thread : threads)
  {
    thread.join();
  }
  REQUIRE(mismatches.load() == 0);

  for(const auto& path : paths)
  {
    std::filesystem::remove(path);
  }
}

TEST_CASE("StorageFormatPlan: HDF5 vector attributes convert into the requested native memory type", "[simplnx][HDF5][StorageFormatPlan]")
{
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "h5io_vector_attribute_native_conversion.h5";
  std::filesystem::remove(path);

  {
    auto file = HDF5::FileIO::WriteFile(path);
    REQUIRE(file.isValid());
    auto group = file.createGroup("g");
    REQUIRE(group.isValid());
    REQUIRE(group.writeVectorAttribute<uint16>("values", {1, 257, 4095}).valid());
    REQUIRE(group.writeStringAttribute("text", "not numeric").valid());

    auto converted = group.readVectorAttribute<uint64>("values");
    REQUIRE(converted.valid());
    CHECK(converted.value() == std::vector<uint64>{1, 257, 4095});

    auto rejected = group.readVectorAttribute<uint64>("text");
    REQUIRE(rejected.invalid());
    REQUIRE_FALSE(rejected.errors().empty());
    CHECK(rejected.errors().front().code == -102);
  }

  std::filesystem::remove(path);
}

// Eight threads open the same file, query both datasets, and use full-array and hyperslab reads.
// Matching values and clean thread joins prove safe concurrent open, query, read, and close paths.
// Leaf locking prevents races against the non-thread-safe HDF5 library.
TEST_CASE("HDF5 IO layer self-locks: concurrent open+query+read does not race", "[simplnx][HDF5][concurrency]")
{
  const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "h5io_selflock.h5";
  std::filesystem::remove(tmp);

  constexpr usize k_Elems = 4096;
  BuildFixtureFile(tmp, k_Elems);

  std::atomic<int> mismatches{0};
  constexpr int k_Threads = 8;
  std::vector<std::thread> threads;
  threads.reserve(k_Threads);
  for(int t = 0; t < k_Threads; ++t)
  {
    threads.emplace_back([&]() {
      for(int pass = 0; pass < 25; ++pass)
      {
        // Each iteration constructs a fresh FileIO so the open/close (and destructor
        // H5Fclose/H5Gclose/H5Dclose) paths run concurrently across threads.
        auto file = HDF5::FileIO::ReadFile(tmp);
        if(!file.isValid())
        {
          mismatches.fetch_add(1, std::memory_order_relaxed);
          continue;
        }
        auto group = file.openGroup("g");
        auto dsA = group.openDataset("a");
        auto dsB = group.openDataset("b");

        // Query methods use their internal locking path on every read.
        const auto dimsA = dsA.getDimensions();
        if(dimsA.size() != 1 || dimsA[0] != k_Elems)
        {
          mismatches.fetch_add(1, std::memory_order_relaxed);
        }
        (void)dsA.getChunkDimensions();
        (void)dsB.getChunkDimensions();

        // Full-array read of dataset "a".
        std::vector<int32> readA(k_Elems);
        if(dsA.readIntoSpan<int32>(nonstd::span<int32>(readA.data(), readA.size())).invalid())
        {
          mismatches.fetch_add(1, std::memory_order_relaxed);
        }
        for(usize i = 0; i < k_Elems; ++i)
        {
          if(readA[i] != static_cast<int32>(i))
          {
            mismatches.fetch_add(1, std::memory_order_relaxed);
            break;
          }
        }

        // Hyperslab read of the middle half of dataset "b".
        constexpr usize k_HalfStart = k_Elems / 4;
        constexpr usize k_HalfCount = k_Elems / 2;
        std::vector<int32> readB(k_HalfCount);
        std::optional<std::vector<uint64>> start = std::vector<uint64>{static_cast<uint64>(k_HalfStart)};
        std::optional<std::vector<uint64>> count = std::vector<uint64>{static_cast<uint64>(k_HalfCount)};
        if(dsB.readIntoSpan<int32>(nonstd::span<int32>(readB.data(), readB.size()), start, count).invalid())
        {
          mismatches.fetch_add(1, std::memory_order_relaxed);
        }
        for(usize i = 0; i < k_HalfCount; ++i)
        {
          if(readB[i] != static_cast<int32>(1000 + k_HalfStart + i))
          {
            mismatches.fetch_add(1, std::memory_order_relaxed);
            break;
          }
        }
      }
    });
  }
  for(auto& th : threads)
  {
    th.join(); // bounded: every thread must join, no hang
  }
  REQUIRE(mismatches.load() == 0);

  std::filesystem::remove(tmp);
}

namespace
{
// The callback releases all HDF5 wrappers before the file cleanup checks run.
template <class BodyT>
void WithHyperslabTestFile(BodyT&& body)
{
  const auto directory = std::filesystem::temp_directory_path() / ("h5io_hyperslab_" + Uuid::GenerateV4().str());
  const auto path = directory / "selection.h5";
  REQUIRE(std::filesystem::create_directory(directory));
  auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code error;
    std::filesystem::remove(path, error);
    std::filesystem::remove(directory, error);
  });
  body(path);
  CHECK(std::filesystem::remove(path));
  CHECK(std::filesystem::remove(directory));
}

void CreateHyperslabSentinelFile(const std::filesystem::path& path)
{
  auto file = HDF5::FileIO::WriteFile(path);
  REQUIRE(file.isValid());
  auto outer = file.createGroup("outer");
  REQUIRE(outer.isValid());
  auto inner = outer.createGroup("inner");
  REQUIRE(inner.isValid());
  auto dataset = inner.createDataset("values");
  const std::array<int32, 6> sentinels{9, 9, 9, 9, 9, 9};
  REQUIRE(dataset.writeSpan<int32>({2, 3}, {sentinels.data(), sentinels.size()}).valid());
}
} // namespace

TEST_CASE("HDF5 hyperslab rejects malformed selections", "[simplnx][HDF5][hyperslab]")
{
  const auto caseIndex = GENERATE(0, 1);
  DYNAMIC_SECTION((caseIndex == 0 ? "excess start rank" : "short logical span"))
  {
    WithHyperslabTestFile([&](const std::filesystem::path& path) {
      CreateHyperslabSentinelFile(path);
      auto file = HDF5::FileIO::AppendFile(path);
      REQUIRE(file.isValid());
      auto outer = file.openGroup("outer");
      REQUIRE(outer.isValid());
      auto inner = outer.openGroup("inner");
      REQUIRE(inner.isValid());
      auto dataset = inner.openDataset("values");
      REQUIRE(dataset.getId() > 0);

      // Both source elements exist even when the logical span contains only one.
      const std::array<int32, 2> source{4, 5};
      const std::vector<uint64> start = caseIndex == 0 ? std::vector<uint64>{1, 1, 0} : std::vector<uint64>{1, 1};
      const usize spanLength = caseIndex == 0 ? source.size() : 1;
      const auto result = dataset.writeSpanHyperslab<int32>({source.data(), spanLength}, start, {1, 2});
      CHECK(result.invalid());
      CHECK(dataset.readAsVector<int32>() == std::vector<int32>{9, 9, 9, 9, 9, 9});
    });
  }
}

TEMPLATE_TEST_CASE("HDF5 hyperslab typed writes preserve unselected values", "[simplnx][HDF5][hyperslab]", int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32, float64, bool)
{
  const usize inputCount = GENERATE(usize{2}, usize{3});
  DYNAMIC_SECTION("input values " << inputCount)
  {
    WithHyperslabTestFile([&](const std::filesystem::path& path) {
      auto file = HDF5::FileIO::WriteFile(path);
      REQUIRE(file.isValid());
      auto group = file.createGroup("typed");
      REQUIRE(group.isValid());
      auto dataset = group.createDataset("values");
      std::array<TestType, 6> sentinels{};
      std::array<TestType, 3> source{};
      std::array<TestType, 6> expected{};
      if constexpr(std::is_same_v<TestType, bool>)
      {
        sentinels = {true, true, true, true, true, true};
        source = {false, true, false};
        expected = {true, true, true, true, false, true};
      }
      else
      {
        sentinels = {9, 9, 9, 9, 9, 9};
        source = {4, 5, 7};
        expected = {9, 9, 9, 9, 4, 5};
      }
      REQUIRE(dataset.writeSpan<TestType>({2, 3}, {sentinels.data(), sentinels.size()}).valid());
      REQUIRE(dataset.writeSpanHyperslab<TestType>({source.data(), inputCount}, {1, 1}, {1, 2}).valid());
      std::array<TestType, 6> actual{};
      REQUIRE(dataset.readIntoSpan<TestType>({actual.data(), actual.size()}).valid());
      CHECK(actual == expected);
    });
  }
}

namespace
{
void CheckHyperslabErrorContext(const Result<>& result, const std::filesystem::path& path, int32 expectedCode)
{
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  const auto& error = result.errors().front();
  CHECK(error.code == expectedCode);
  CHECK(error.message.find("outer/inner/values") != std::string::npos);
  CHECK(error.message.find(path.string()) != std::string::npos);
}

// The caller resolves parentId before this helper takes the nonrecursive HDF5 lock.
void CreateSpecialHyperslabDataset(hid_t parentId, H5S_class_t spaceType, const std::vector<hsize_t>& dimensions)
{
  std::lock_guard<std::mutex> apiLock(HDF5::Support::ApiLock());
  const hid_t spaceId = spaceType == H5S_SIMPLE ? H5Screate_simple(static_cast<int>(dimensions.size()), dimensions.data(), nullptr) : H5Screate(spaceType);
  REQUIRE(spaceId >= 0);
  auto spaceGuard = MakeScopeGuard([spaceId]() noexcept { H5Sclose(spaceId); });
  REQUIRE(H5Sget_simple_extent_type(spaceId) == spaceType);
  const int expectedRank = spaceType == H5S_SIMPLE ? static_cast<int>(dimensions.size()) : 0;
  REQUIRE(H5Sget_simple_extent_ndims(spaceId) == expectedRank);
  if(spaceType == H5S_SIMPLE)
  {
    std::vector<hsize_t> actual(dimensions.size());
    REQUIRE(H5Sget_simple_extent_dims(spaceId, actual.data(), nullptr) == expectedRank);
    REQUIRE(actual == dimensions);
  }
  const hid_t datasetId = H5Dcreate2(parentId, "values", H5T_NATIVE_INT32, spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  REQUIRE(datasetId >= 0);
  auto datasetGuard = MakeScopeGuard([datasetId]() noexcept { H5Dclose(datasetId); });
}
} // namespace

TEST_CASE("HDF5 hyperslab rejects malformed selections after rank and size validation", "[simplnx][HDF5][hyperslab]")
{
  struct Selection
  {
    const char* name;
    std::vector<uint64> start;
    std::vector<uint64> count;
    const char* diagnostic;
  };
  const std::array<Selection, 13> selections{{
      {"empty start rank", {}, {1, 2}, "rank"},
      {"short start rank", {1}, {1, 2}, "rank"},
      {"empty count rank", {1, 1}, {}, "rank"},
      {"short count rank", {1, 1}, {1}, "rank"},
      {"excess count rank", {1, 1}, {1, 2, 1}, "rank"},
      {"both ranks empty", {}, {}, "rank"},
      {"start at nonempty boundary", {2, 0}, {1, 1}, "axis"},
      {"start beyond extent", {3, 0}, {1, 1}, "axis"},
      {"count beyond remaining extent", {1, 2}, {1, 2}, "axis"},
      {"maximum start", {std::numeric_limits<uint64>::max(), 0}, {1, 1}, "axis"},
      {"maximum count", {0, 0}, {std::numeric_limits<uint64>::max(), 1}, "axis"},
      {"empty first axis does not hide invalid second axis", {0, 4}, {0, 0}, "axis"},
      {"empty selection beyond both boundaries", {2, 4}, {0, 0}, "axis"},
  }};
  const usize caseIndex = GENERATE(Catch::Generators::range(usize{0}, usize{13}));
  const auto& selection = selections[caseIndex];
  DYNAMIC_SECTION(selection.name)
  {
    WithHyperslabTestFile([&](const std::filesystem::path& path) {
      CreateHyperslabSentinelFile(path);
      auto file = HDF5::FileIO::AppendFile(path);
      REQUIRE(file.isValid());
      auto outer = file.openGroup("outer");
      REQUIRE(outer.isValid());
      auto inner = outer.openGroup("inner");
      REQUIRE(inner.isValid());
      auto dataset = inner.openDataset("values");
      REQUIRE(dataset.getId() > 0);
      const std::array<int32, 6> source{4, 5, 6, 7, 8, 9};
      const auto result = dataset.writeSpanHyperslab<int32>({source.data(), source.size()}, selection.start, selection.count);
      CHECK(dataset.readAsVector<int32>() == std::vector<int32>{9, 9, 9, 9, 9, 9});
      CheckHyperslabErrorContext(result, path, -1012);
      CHECK(result.errors().front().message.find(selection.diagnostic) != std::string::npos);
    });
  }
}

TEST_CASE("HDF5 hyperslab reports an empty input and a native write failure", "[simplnx][HDF5][hyperslab]")
{
  const bool readOnly = GENERATE(false, true);
  DYNAMIC_SECTION((readOnly ? "native read-only failure" : "default-empty source span"))
  {
    WithHyperslabTestFile([&](const std::filesystem::path& path) {
      CreateHyperslabSentinelFile(path);
      auto file = readOnly ? HDF5::FileIO::ReadFile(path) : HDF5::FileIO::AppendFile(path);
      REQUIRE(file.isValid());
      auto outer = file.openGroup("outer");
      REQUIRE(outer.isValid());
      auto inner = outer.openGroup("inner");
      REQUIRE(inner.isValid());
      auto dataset = inner.openDataset("values");
      REQUIRE(dataset.getId() > 0);
      const std::array<int32, 2> source{4, 5};
      const nonstd::span<const int32> input = readOnly ? nonstd::span<const int32>{source.data(), source.size()} : nonstd::span<const int32>{};
      const auto result = dataset.writeSpanHyperslab<int32>(input, {1, 1}, {1, 2});
      CHECK(dataset.readAsVector<int32>() == std::vector<int32>{9, 9, 9, 9, 9, 9});
      CheckHyperslabErrorContext(result, path, readOnly ? -1014 : -1012);
      if(readOnly)
      {
        const auto& message = result.errors().front().message;
        CHECK(message.find("H5Dwrite") != std::string::npos);
        CHECK((message.find("status -1") != std::string::npos || message.find("error -1") != std::string::npos));
      }
    });
  }
}

TEST_CASE("HDF5 hyperslab empty and scalar selection contract", "[simplnx][HDF5][hyperslab]")
{
  SECTION("empty simple selection does not write, including read-only files")
  {
    const bool readOnly = GENERATE(false, true);
    const usize caseIndex = GENERATE(usize{0}, usize{1}, usize{2}, usize{3});
    const std::array<std::vector<uint64>, 4> starts{{{0, 1}, {1, 0}, {0, 0}, {2, 3}}};
    const std::array<std::vector<uint64>, 4> counts{{{0, 2}, {1, 0}, {0, 0}, {0, 0}}};
    DYNAMIC_SECTION("read-only " << readOnly << ", selection " << caseIndex)
    {
      WithHyperslabTestFile([&](const std::filesystem::path& path) {
        CreateHyperslabSentinelFile(path);
        auto file = readOnly ? HDF5::FileIO::ReadFile(path) : HDF5::FileIO::AppendFile(path);
        REQUIRE(file.isValid());
        auto outer = file.openGroup("outer");
        REQUIRE(outer.isValid());
        auto inner = outer.openGroup("inner");
        REQUIRE(inner.isValid());
        auto dataset = inner.openDataset("values");
        REQUIRE(dataset.getId() > 0);
        CHECK(dataset.writeSpanHyperslab<int32>({}, starts[caseIndex], counts[caseIndex]).valid());
        CHECK(dataset.readAsVector<int32>() == std::vector<int32>{9, 9, 9, 9, 9, 9});
      });
    }
  }

  SECTION("zero extent permits only an in-bounds empty selection")
  {
    WithHyperslabTestFile([&](const std::filesystem::path& path) {
      auto file = HDF5::FileIO::WriteFile(path);
      REQUIRE(file.isValid());
      const hid_t fileId = file.getId();
      CreateSpecialHyperslabDataset(fileId, H5S_SIMPLE, {0, 3});
      auto dataset = file.openDataset("values");
      REQUIRE(dataset.getId() > 0);
      CHECK(dataset.writeSpanHyperslab<int32>({}, {0, 1}, {0, 2}).valid());
      CHECK(dataset.writeSpanHyperslab<int32>({}, {1, 1}, {0, 2}).invalid());
      CHECK(dataset.getDimensions() == std::vector<usize>{0, 3});
    });
  }

  SECTION("zero count does not hide a rank mismatch")
  {
    WithHyperslabTestFile([&](const std::filesystem::path& path) {
      CreateHyperslabSentinelFile(path);
      auto file = HDF5::FileIO::AppendFile(path);
      REQUIRE(file.isValid());
      auto outer = file.openGroup("outer");
      REQUIRE(outer.isValid());
      auto inner = outer.openGroup("inner");
      REQUIRE(inner.isValid());
      auto dataset = inner.openDataset("values");
      REQUIRE(dataset.getId() > 0);
      const auto result = dataset.writeSpanHyperslab<int32>({}, {0}, {0, 0});
      CHECK(dataset.readAsVector<int32>() == std::vector<int32>{9, 9, 9, 9, 9, 9});
      CheckHyperslabErrorContext(result, path, -1012);
      CHECK(result.errors().front().message.find("rank") != std::string::npos);
    });
  }

  SECTION("scalar hyperslabs fail while full scalar writes remain valid")
  {
    WithHyperslabTestFile([&](const std::filesystem::path& path) {
      auto file = HDF5::FileIO::WriteFile(path);
      REQUIRE(file.isValid());
      auto dataset = file.createDataset("values");
      const std::array<int32, 1> sentinel{9};
      REQUIRE(dataset.writeSpan<int32>({}, {sentinel.data(), sentinel.size()}).valid());
      const hid_t datasetId = dataset.getId();
      REQUIRE(datasetId > 0);
      {
        std::lock_guard<std::mutex> apiLock(HDF5::Support::ApiLock());
        const hid_t spaceId = H5Dget_space(datasetId);
        REQUIRE(spaceId >= 0);
        auto spaceGuard = MakeScopeGuard([spaceId]() noexcept { H5Sclose(spaceId); });
        REQUIRE(H5Sget_simple_extent_type(spaceId) == H5S_SCALAR);
      }
      const std::array<int32, 1> source{4};
      const auto result = dataset.writeSpanHyperslab<int32>({source.data(), source.size()}, {}, {});
      CHECK(dataset.readAsVector<int32>() == std::vector<int32>{9});
      REQUIRE(result.invalid());
      REQUIRE_FALSE(result.errors().empty());
      CHECK(result.errors().front().message.find("scalar") != std::string::npos);
      REQUIRE(dataset.writeSpan<int32>({}, {source.data(), source.size()}).valid());
      CHECK(dataset.readAsVector<int32>() == std::vector<int32>{4});
    });
  }

  SECTION("null dataspaces do not become empty simple selections")
  {
    WithHyperslabTestFile([&](const std::filesystem::path& path) {
      auto file = HDF5::FileIO::WriteFile(path);
      REQUIRE(file.isValid());
      const hid_t fileId = file.getId();
      CreateSpecialHyperslabDataset(fileId, H5S_NULL, {});
      auto dataset = file.openDataset("values");
      REQUIRE(dataset.getId() > 0);
      const auto result = dataset.writeSpanHyperslab<int32>({}, {}, {});
      REQUIRE(result.invalid());
      REQUIRE_FALSE(result.errors().empty());
      CHECK(result.errors().front().message.find("null") != std::string::npos);
    });
  }
}

TEST_CASE("HDF5 hyperslab writes through an unlinked open dataset", "[simplnx][HDF5][hyperslab]")
{
  WithHyperslabTestFile([&](const std::filesystem::path& path) {
    CreateHyperslabSentinelFile(path);
    auto file = HDF5::FileIO::AppendFile(path);
    REQUIRE(file.isValid());
    auto outer = file.openGroup("outer");
    REQUIRE(outer.isValid());
    auto inner = outer.openGroup("inner");
    REQUIRE(inner.isValid());
    auto dataset = inner.openDataset("values");
    const hid_t parentId = inner.getId();
    const hid_t datasetId = dataset.getId();
    REQUIRE(parentId > 0);
    REQUIRE(datasetId > 0);

    // The open wrapper retains the dataset after its final link is removed.
    {
      std::lock_guard<std::mutex> apiLock(HDF5::Support::ApiLock());
      REQUIRE(H5Ldelete(parentId, "values", H5P_DEFAULT) >= 0);
      REQUIRE(H5Lexists(parentId, "values", H5P_DEFAULT) == 0);
      REQUIRE(H5Iis_valid(datasetId) > 0);
      REQUIRE(H5Iget_name(datasetId, nullptr, 0) == 0);
    }
    REQUIRE(dataset.isValid());

    const std::array<int32, 2> source{4, 5};
    Result<> writeResult;
    REQUIRE_NOTHROW(writeResult = dataset.writeSpanHyperslab<int32>({source.data(), source.size()}, {1, 1}, {1, 2}));
    REQUIRE(writeResult.valid());
    std::array<int32, 6> actual{};
    {
      std::lock_guard<std::mutex> apiLock(HDF5::Support::ApiLock());
      REQUIRE(H5Dread(datasetId, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, actual.data()) >= 0);
    }
    CHECK(actual == std::array<int32, 6>{9, 9, 9, 9, 4, 5});

    const std::array<int32, 2> rejectedSource{7, 8};
    Result<> rejectedResult;
    REQUIRE_NOTHROW(rejectedResult = dataset.writeSpanHyperslab<int32>({rejectedSource.data(), rejectedSource.size()}, {1, 1, 0}, {1, 2}));
    REQUIRE(rejectedResult.invalid());
    REQUIRE(rejectedResult.errors().size() == 1);
    const auto& error = rejectedResult.errors().front();
    CHECK(error.code == -1012);
    CHECK(error.message.find("values") != std::string::npos);
    CHECK(error.message.find(path.string()) != std::string::npos);
    CHECK(error.message.find("rank") != std::string::npos);
    {
      std::lock_guard<std::mutex> apiLock(HDF5::Support::ApiLock());
      REQUIRE(H5Dread(datasetId, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, actual.data()) >= 0);
    }
    CHECK(actual == std::array<int32, 6>{9, 9, 9, 9, 4, 5});
  });
}

namespace
{
// Sparse creation must succeed before a test can claim to reach an overflow guard.
void CreateSparseHyperslabDataset(hid_t parentId, const std::vector<hsize_t>& dimensions)
{
  const auto rank = static_cast<int>(dimensions.size());
  const std::vector<hsize_t> chunks(dimensions.size(), 1);
  std::lock_guard<std::mutex> apiLock(HDF5::Support::ApiLock());
  const hid_t spaceId = H5Screate_simple(rank, dimensions.data(), nullptr);
  REQUIRE(spaceId >= 0);
  auto spaceGuard = MakeScopeGuard([spaceId]() noexcept { H5Sclose(spaceId); });
  REQUIRE(H5Sget_simple_extent_type(spaceId) == H5S_SIMPLE);
  REQUIRE(H5Sget_simple_extent_ndims(spaceId) == rank);
  std::vector<hsize_t> actual(dimensions.size());
  REQUIRE(H5Sget_simple_extent_dims(spaceId, actual.data(), nullptr) == rank);
  REQUIRE(actual == dimensions);
  const hid_t propertiesId = H5Pcreate(H5P_DATASET_CREATE);
  REQUIRE(propertiesId >= 0);
  auto propertiesGuard = MakeScopeGuard([propertiesId]() noexcept { H5Pclose(propertiesId); });
  REQUIRE(H5Pset_chunk(propertiesId, rank, chunks.data()) >= 0);
  REQUIRE(H5Pset_alloc_time(propertiesId, H5D_ALLOC_TIME_LATE) >= 0);
  REQUIRE(H5Pset_fill_time(propertiesId, H5D_FILL_TIME_NEVER) >= 0);
  const hid_t datasetId = H5Dcreate2(parentId, "values", H5T_NATIVE_UINT8, spaceId, H5P_DEFAULT, propertiesId, H5P_DEFAULT);
  REQUIRE(datasetId >= 0);
  auto datasetGuard = MakeScopeGuard([datasetId]() noexcept { H5Dclose(datasetId); });
  REQUIRE(H5Dget_storage_size(datasetId) == 0);
  const hid_t storedSpaceId = H5Dget_space(datasetId);
  REQUIRE(storedSpaceId >= 0);
  auto storedSpaceGuard = MakeScopeGuard([storedSpaceId]() noexcept { H5Sclose(storedSpaceId); });
  REQUIRE(H5Sget_simple_extent_type(storedSpaceId) == H5S_SIMPLE);
  REQUIRE(H5Sget_simple_extent_ndims(storedSpaceId) == rank);
  REQUIRE(H5Sget_simple_extent_dims(storedSpaceId, actual.data(), nullptr) == rank);
  REQUIRE(actual == dimensions);
}

void RequireSparseHyperslabStorage(hid_t datasetId)
{
  std::lock_guard<std::mutex> apiLock(HDF5::Support::ApiLock());
  REQUIRE(H5Iis_valid(datasetId) > 0);
  REQUIRE(H5Dget_storage_size(datasetId) == 0);
}
} // namespace

TEST_CASE("HDF5 hyperslab sparse overflow fixture candidates", "[simplnx][HDF5][hyperslab][hyperslab-overflow-candidate]")
{
  const auto caseIndex = GENERATE(0, 1, 2);
  DYNAMIC_SECTION("overflow fixture " << caseIndex)
  {
    WithHyperslabTestFile([&](const std::filesystem::path& path) {
      {
        auto file = HDF5::FileIO::WriteFile(path);
        REQUIRE(file.isValid());
        const hid_t fileId = file.getId();
        const std::vector<uint64> byteOverflowDimensions = sizeof(usize) >= 8 ? std::vector<uint64>{2147483648ULL, 2147483648ULL} : std::vector<uint64>{65536, 8192};
        const std::vector<uint64> dimensions = caseIndex == 0 ? std::vector<uint64>{4294967296ULL, 4294967296ULL} :
                                               caseIndex == 1 ? byteOverflowDimensions :
                                                                std::vector<uint64>{4294967296ULL, 4294967296ULL, 0};
        std::vector<hsize_t> nativeDimensions;
        nativeDimensions.reserve(dimensions.size());
        for(const uint64 dimension : dimensions)
        {
          REQUIRE(dimension <= std::numeric_limits<hsize_t>::max());
          nativeDimensions.push_back(static_cast<hsize_t>(dimension));
        }
        CreateSparseHyperslabDataset(fileId, nativeDimensions);
        auto dataset = file.openDataset("values");
        const hid_t datasetId = dataset.getId();
        REQUIRE(datasetId > 0);
        RequireSparseHyperslabStorage(datasetId);
        const std::vector<uint64> start(dimensions.size(), 0);
        Result<> result;
        if(caseIndex == 0)
        {
          const std::array<uint8, 1> input{4};
          result = dataset.writeSpanHyperslab<uint8>({input.data(), input.size()}, start, dimensions);
        }
        else if(caseIndex == 1)
        {
          const std::array<uint64, 1> input{4};
          result = dataset.writeSpanHyperslab<uint64>({input.data(), input.size()}, start, dimensions);
        }
        else
        {
          result = dataset.writeSpanHyperslab<uint8>({}, start, dimensions);
        }
        RequireSparseHyperslabStorage(datasetId);
        if(caseIndex == 2)
        {
          CHECK(result.valid());
        }
        else
        {
          REQUIRE(result.invalid());
          REQUIRE(result.errors().size() == 1);
          CHECK(result.errors().front().code == -1012);
          CHECK(result.errors().front().message.find(caseIndex == 0 ? "selected element count overflow" : "selected byte count overflow") != std::string::npos);
          CHECK(result.errors().front().message.find(path.string()) != std::string::npos);
        }
      }
      // A closed sparse fixture must remain a small metadata file.
      CHECK(std::filesystem::file_size(path) < 1024ULL * 1024ULL);
    });
  }
}
