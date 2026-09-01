#include "simplnx/UnitTest/HDF5DatasetProbe.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <catch2/catch.hpp>

#include <nonstd/span.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <numeric>
#include <thread>
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
