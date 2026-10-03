#pragma once

#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <system_error>

/**
 * @namespace nx::core::UnitTest::AvizoWriterFailure
 * @brief Provides source-read failure fixtures for the Avizo filters.
 */
namespace nx::core::UnitTest::AvizoWriterFailure
{
/**
 * @class SecondBlockFailureStore
 * @brief Reports an error after one complete Avizo source window.
 * @tparam BaseStore Supplies resident or real HDF5 storage.
 */
template <class BaseStore>
class SecondBlockFailureStore : public BaseStore
{
public:
  using BaseStore::BaseStore;

  /**
   * @brief Copies the first window and rejects the second read.
   * @param startIndex Zero-based source element offset.
   * @param buffer Receives the first window's feature IDs.
   * @return Named error for the second read; otherwise the base store's Result.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<int32> buffer) const override
  {
    if(readCalls < readOffsets.size())
    {
      readOffsets[readCalls] = startIndex;
      readCounts[readCalls] = buffer.size();
    }
    ++readCalls;
    if(readCalls == 2)
    {
      if(throwAllocation)
      {
        throw std::bad_alloc{};
      }
      return MakeErrorResult(-92561, "Injected Avizo second-block read failure");
    }
    Result<> result = BaseStore::copyIntoBuffer(startIndex, buffer);
    if(result.valid())
    {
      ++successfulReads;
      firstBufferHasExpectedValues = std::all_of(buffer.begin(), buffer.end(), [](int32 value) { return value == 7; });
    }
    return result;
  }

  bool throwAllocation = false;
  mutable bool firstBufferHasExpectedValues = false;
  mutable usize readCalls = 0;
  mutable usize successfulReads = 0;
  mutable std::array<usize, 2> readOffsets{};
  mutable std::array<usize, 2> readCounts{};
};

/**
 * @brief Checks the real filter's publication and cleanup after a later source-read error.
 * @tparam FilterType Selects the uniform or rectilinear Avizo filter.
 * @tparam StoreType Supplies the observed source store.
 * @param writeBinary Selects native binary feature IDs instead of ASCII.
 * @param existingDestination Selects an existing file with sentinel bytes.
 * @param store Supplies exactly two source windows.
 * @param throwAllocation Selects the filter allocation-error path on the second read.
 */
template <class FilterType, class StoreType = SecondBlockFailureStore<DataStore<int32>>>
void CheckSecondBlockFailure(bool writeBinary, bool existingDestination, std::shared_ptr<StoreType> store = std::make_shared<StoreType>(ShapeType{1, 1, 65537}, ShapeType{1}, std::optional<int32>{7}),
                             bool throwAllocation = false)
{
  namespace fs = std::filesystem;
  const auto outputBase = fs::path(unit_test::k_BinaryTestOutputDir.view());
  fs::create_directories(outputBase);
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const fs::path outputRoot = outputBase / ("AvizoReadFailure-" + std::to_string(stamp));
  REQUIRE(fs::create_directory(outputRoot));
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    try
    {
      std::error_code ignored;
      fs::remove_all(outputRoot, ignored);
    } catch(...)
    {
    }
  });
  const fs::path destination = outputRoot / "output.am";
  const std::string sentinel = "preserved destination";
  if(existingDestination)
  {
    std::ofstream output(destination, std::ios::binary);
    REQUIRE(output.is_open());
    output.write(sentinel.data(), static_cast<std::streamsize>(sentinel.size()));
    output.close();
    REQUIRE(output.good());
  }

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeom");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({65537, 1, 1});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellData = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 65537}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  store->throwAllocation = throwAllocation;
  auto* featureIds = Int32Array::Create(dataStructure, "FeatureIds", store, cellData->getId());
  REQUIRE(featureIds != nullptr);

  FilterType filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(FilterType::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(destination));
  args.insertOrAssign(FilterType::k_WriteBinaryFile_Key, std::make_any<bool>(writeBinary));
  args.insertOrAssign(FilterType::k_GeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom"})));
  args.insertOrAssign(FilterType::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom", "CellData", "FeatureIds"})));

  const auto executeResult = filter.execute(dataStructure, args);
  CHECK(store->readCalls == 2);
  CHECK(store->successfulReads == 1);
  CHECK(store->firstBufferHasExpectedValues);
  CHECK(store->readOffsets == std::array<usize, 2>{0, 65536});
  CHECK(store->readCounts == std::array<usize, 2>{65536, 1});
  REQUIRE(executeResult.result.invalid());
  bool foundReadError = false;
  for(const auto& error : executeResult.result.errors())
  {
    foundReadError = foundReadError || (throwAllocation ? error.code == -272 : (error.code == -92561 && error.message.find("Injected Avizo second-block read failure") != std::string::npos));
  }
  CHECK(foundReadError);

  if(existingDestination)
  {
    std::ifstream input(destination, std::ios::binary);
    REQUIRE(input.is_open());
    const std::string actual((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    CHECK(actual == sentinel);
  }
  else
  {
    CHECK_FALSE(fs::exists(destination));
  }

  usize entryCount = 0;
  for(const auto& entry : fs::directory_iterator(outputRoot))
  {
    ++entryCount;
    CHECK(entry.path() == destination);
  }
  CHECK(entryCount == (existingDestination ? usize{1} : usize{0}));
  std::error_code cleanupError;
  fs::remove_all(outputRoot, cleanupError);
  CHECK_FALSE(cleanupError);
  CHECK_FALSE(fs::exists(outputRoot));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace nx::core::UnitTest::AvizoWriterFailure
