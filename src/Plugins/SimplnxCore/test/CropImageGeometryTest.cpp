#include "SimplnxCore/Filters/Algorithms/CropImageGeometry.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numeric>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
inline constexpr StringLiteral k_DataContainer("DataContainer2");
inline constexpr usize k_CropScratchBytes = 1024 * 1024;

/**
 * @class CropProbeStore
 * @brief Records crop transfer sizes and injects transfer behavior.
 * @tparam T Stored value type.
 *
 * Each fixture gives this store to one array task. The test reads
 * the recorded
 * state after the crop executor joins all tasks.
 */
template <typename T>
class CropProbeStore : public DataStore<T>
{
public:
  /**
   * @brief Creates a probe with resident or fallback behavior.
   * @param tupleShape Tuple dimensions.
   * @param componentShape Component dimensions.
   * @param resident True to report
   * in-memory storage.
   * @param initValue Initial value for all elements.
   */
  CropProbeStore(ShapeType tupleShape, ShapeType componentShape, bool resident, T initValue = T{})
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_Resident(resident)
  {
  }

  /**
   * @brief Reports the storage route that the crop must use.
   * @return InMemory for direct-copy fixtures, or OutOfCore for bulk fixtures.
   */
  IDataStore::StoreType getStoreType() const override
  {
    return m_Resident ? IDataStore::StoreType::InMemory : IDataStore::StoreType::OutOfCore;
  }

  /**
   * @brief Records and performs one bulk read.
   * @param startIndex First flat value index.
   * @param buffer Receives the values.
   * @return The injected error or the DataStore read
   * result.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++readCalls;
    readBytes.push_back(buffer.size() * sizeof(T));
    largestReadBytes = (std::max)(largestReadBytes, readBytes.back());
    if(failRead)
    {
      return MakeErrorResult(-95100, "Injected crop source read failure.");
    }
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  /**
   * @brief Records and performs one bulk write.
   * @param startIndex First flat value index.
   * @param buffer Supplies the values.
   * @return The injected error or the DataStore write
   * result.
   */
  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++writeCalls;
    writeBytes.push_back(buffer.size() * sizeof(T));
    largestWriteBytes = (std::max)(largestWriteBytes, writeBytes.back());
    if(failWrite)
    {
      return MakeErrorResult(-95101, "Injected crop destination write failure.");
    }
    auto result = DataStore<T>::copyFromBuffer(startIndex, buffer);
    if(result.valid() && cancelAfterWrite != nullptr)
    {
      cancelAfterWrite->store(true);
    }
    return result;
  }

  bool failRead = false;
  bool failWrite = false;
  std::atomic_bool* cancelAfterWrite = nullptr;
  mutable usize readCalls = 0;
  usize writeCalls = 0;
  mutable usize largestReadBytes = 0;
  usize largestWriteBytes = 0;
  mutable std::vector<usize> readBytes;
  std::vector<usize> writeBytes;

private:
  bool m_Resident = true;
};

/**
 * @brief Creates source and destination image geometries for a direct crop test.
 * @tparam T Stored value type.
 * @param sourceStore Source probe store.
 * @param destinationStore Destination
 * probe store.
 * @param sourceDimensions Source XYZ dimensions.
 * @param cropDimensions Destination XYZ dimensions.
 * @return Data structure that owns both test arrays.
 */
template <typename T>
DataStructure CreateCropProbeData(const std::shared_ptr<CropProbeStore<T>>& sourceStore, const std::shared_ptr<CropProbeStore<T>>& destinationStore, const SizeVec3& sourceDimensions,
                                  const SizeVec3& cropDimensions)
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());

  auto* sourceGeom = ImageGeom::Create(dataStructure, "Source");
  auto* destinationGeom = ImageGeom::Create(dataStructure, "Destination");
  REQUIRE(sourceGeom != nullptr);
  REQUIRE(destinationGeom != nullptr);
  sourceGeom->setDimensions(sourceDimensions);
  destinationGeom->setDimensions(cropDimensions);
  sourceGeom->setOrigin({0.0F, 0.0F, 0.0F});
  destinationGeom->setOrigin({0.0F, 0.0F, 0.0F});
  sourceGeom->setSpacing({1.0F, 1.0F, 1.0F});
  destinationGeom->setSpacing({1.0F, 1.0F, 1.0F});

  auto* sourceCellData = AttributeMatrix::Create(dataStructure, "Cell Data", sourceStore->getTupleShape(), sourceGeom->getId());
  auto* destinationCellData = AttributeMatrix::Create(dataStructure, "Cell Data", destinationStore->getTupleShape(), destinationGeom->getId());
  REQUIRE(sourceCellData != nullptr);
  REQUIRE(destinationCellData != nullptr);
  sourceGeom->setCellData(*sourceCellData);
  destinationGeom->setCellData(*destinationCellData);
  REQUIRE(DataArray<T>::Create(dataStructure, "Values", sourceStore, sourceCellData->getId()) != nullptr);
  REQUIRE(DataArray<T>::Create(dataStructure, "Values", destinationStore, destinationCellData->getId()) != nullptr);
  return dataStructure;
}

/**
 * @brief Creates direct-executor input values for inclusive voxel bounds.
 * @param xMin Minimum X index.
 * @param xMax Maximum X index.
 * @param yMin Minimum Y index.
 * @param yMax Maximum Y
 * index.
 * @param zMin Minimum Z index.
 * @param zMax Maximum Z index.
 * @return Crop input values for the test geometries.
 */
CropImageGeometryInputValues CreateCropInputValues(uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin, uint64 zMax)
{
  CropImageGeometryInputValues values{};
  values.InputImageGeometryPath = DataPath({"Source"});
  values.OutputImageGeometryPath = DataPath({"Destination"});
  values.XMin = xMin;
  values.XMax = xMax;
  values.YMin = yMin;
  values.YMax = yMax;
  values.ZMin = zMin;
  values.ZMax = zMax;
  return values;
}

/**
 * @brief Runs the crop executor with no message callback.
 * @param dataStructure Contains the test geometries.
 * @param values Supplies the crop bounds and paths.
 * @param shouldCancel
 * Supplies the cancellation state.
 * @return Crop execution result.
 */
Result<> RunCrop(DataStructure& dataStructure, CropImageGeometryInputValues& values, const std::atomic_bool& shouldCancel)
{
  const IFilter::MessageHandler messageHandler{};
  return CropImageGeometry(dataStructure, messageHandler, shouldCancel, &values)();
}

/**
 * @brief Fills a probe with a repeating flat-index pattern.
 * @tparam T Stored value type.
 * @param store Receives the values.
 * @param modulus Controls the repeating pattern.
 */
template <typename T>
void FillProbeStore(CropProbeStore<T>& store, usize modulus)
{
  for(usize valueIndex = 0; valueIndex < store.getSize(); ++valueIndex)
  {
    store.data()[valueIndex] = static_cast<T>(valueIndex % modulus);
  }
}

struct CompareDataArrayFunctor
{
  template <typename T>
  void operator()(const IDataArray& left, const IDataArray& right, usize start = 0)
  {
    UnitTest::CompareDataArrays<T>(left, right, start);
  }
};

DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroup->getId());

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.55f, 1.86});
  imageGeom->setOrigin({0.0f, 20.0f, 66.0f});
  SizeVec3 imageGeomDims = {40, 60, 80};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  auto imageDimsArray = imageGeomDims.toArray();
  ShapeType cellDataDims{imageDimsArray.crbegin(), imageDimsArray.crend()};
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, ImageGeom::k_CellAttributeMatrixName, cellDataDims, imageGeom->getId());
  imageGeom->setCellData(*cellDataPtr);

  Int32Array* phases_data = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", cellDataDims, {1}, cellDataPtr->getId());

  return dataStructure;
}
} // namespace

TEMPLATE_TEST_CASE("SimplnxCore::CropImageGeometry: resident stores avoid slab transfers", "[SimplnxCore][CropImageGeometryFilter][CropScratch]", bool, int32, float32)
{
  UnitTest::LoadPlugins();
  const usize components = GENERATE(usize{1}, usize{3});
  auto sourceStore = std::make_shared<CropProbeStore<TestType>>(ShapeType{3, 512, 1024}, ShapeType{components}, true);
  auto destinationStore = std::make_shared<CropProbeStore<TestType>>(ShapeType{3, 1, 1}, ShapeType{components}, true);
  FillProbeStore(*sourceStore, 71);
  auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{1024, 512, 3}, SizeVec3{1, 1, 3});
  auto values = CreateCropInputValues(19, 19, 37, 37, 0, 2);
  const std::atomic_bool shouldCancel = false;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(sourceStore->readCalls == 0);
  CHECK(destinationStore->writeCalls == 0);
  for(usize zIndex = 0; zIndex < 3; ++zIndex)
  {
    for(usize componentIndex = 0; componentIndex < components; ++componentIndex)
    {
      const usize sourceValueIndex = ((zIndex * 512 + 37) * 1024 + 19) * components + componentIndex;
      const usize destinationValueIndex = zIndex * components + componentIndex;
      CHECK(destinationStore->data()[destinationValueIndex] == static_cast<TestType>(sourceValueIndex % 71));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometry: fallback bounds wide selected rows", "[SimplnxCore][CropImageGeometryFilter][CropScratch]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_SourceX = 300000;
  constexpr usize k_CropX = 299999;
  auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{2, 1, k_SourceX}, ShapeType{1}, false);
  auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{2, 1, k_CropX}, ShapeType{1}, false);
  FillProbeStore(*sourceStore, 101);
  auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{k_SourceX, 1, 2}, SizeVec3{k_CropX, 1, 2});
  auto values = CreateCropInputValues(1, k_SourceX - 1, 0, 0, 0, 1);
  const std::atomic_bool shouldCancel = false;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(sourceStore->readCalls >= 4);
  REQUIRE(destinationStore->writeCalls >= 4);
  CHECK(sourceStore->largestReadBytes <= k_CropScratchBytes);
  CHECK(destinationStore->largestWriteBytes <= k_CropScratchBytes);
  usize mismatchIndex = destinationStore->getSize();
  for(usize destinationValueIndex = 0; destinationValueIndex < destinationStore->getSize(); ++destinationValueIndex)
  {
    const usize zIndex = destinationValueIndex / k_CropX;
    const usize xIndex = destinationValueIndex % k_CropX;
    const usize sourceValueIndex = zIndex * k_SourceX + xIndex + 1;
    if(destinationStore->data()[destinationValueIndex] != static_cast<int32>(sourceValueIndex % 101))
    {
      mismatchIndex = destinationValueIndex;
      break;
    }
  }
  INFO("First mismatched output value index: " << mismatchIndex);
  REQUIRE(mismatchIndex == destinationStore->getSize());
}

TEST_CASE("SimplnxCore::CropImageGeometry: fallback aligns ordinary segments to complete tuples", "[SimplnxCore][CropImageGeometryFilter][CropScratch]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_Components = 3;
  constexpr usize k_CropX = 100000;
  constexpr usize k_SourceX = k_CropX + 2;
  constexpr usize k_TupleBytes = k_Components * sizeof(int32);
  constexpr usize k_AlignedSegmentBytes = (k_CropScratchBytes / k_TupleBytes) * k_TupleBytes;
  constexpr usize k_TailBytes = (k_CropX * k_TupleBytes) - k_AlignedSegmentBytes;
  auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{1, 1, k_SourceX}, ShapeType{k_Components}, false);
  auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{1, 1, k_CropX}, ShapeType{k_Components}, false);
  FillProbeStore(*sourceStore, 109);
  auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{k_SourceX, 1, 1}, SizeVec3{k_CropX, 1, 1});
  auto values = CreateCropInputValues(1, k_CropX, 0, 0, 0, 0);
  const std::atomic_bool shouldCancel = false;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(sourceStore->readBytes == std::vector<usize>{k_AlignedSegmentBytes, k_TailBytes});
  REQUIRE(destinationStore->writeBytes == sourceStore->readBytes);
  for(const usize transferBytes : sourceStore->readBytes)
  {
    CHECK(transferBytes <= k_CropScratchBytes);
    CHECK(transferBytes % k_TupleBytes == 0);
  }
  usize mismatchIndex = destinationStore->getSize();
  for(usize destinationValueIndex = 0; destinationValueIndex < destinationStore->getSize(); ++destinationValueIndex)
  {
    const usize destinationTuple = destinationValueIndex / k_Components;
    const usize componentIndex = destinationValueIndex % k_Components;
    const usize sourceValueIndex = (destinationTuple + 1) * k_Components + componentIndex;
    if(destinationStore->data()[destinationValueIndex] != static_cast<int32>(sourceValueIndex % 109))
    {
      mismatchIndex = destinationValueIndex;
      break;
    }
  }
  INFO("First mismatched output value index: " << mismatchIndex);
  REQUIRE(mismatchIndex == destinationStore->getSize());
}

TEST_CASE("SimplnxCore::CropImageGeometry: fallback segments tuples wider than the buffer", "[SimplnxCore][CropImageGeometryFilter][CropScratch]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_Components = k_CropScratchBytes + 1;
  auto sourceStore = std::make_shared<CropProbeStore<uint8>>(ShapeType{2, 1, 2}, ShapeType{k_Components}, false);
  auto destinationStore = std::make_shared<CropProbeStore<uint8>>(ShapeType{2, 1, 1}, ShapeType{k_Components}, false);
  FillProbeStore(*sourceStore, 251);
  auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{2, 1, 2}, SizeVec3{1, 1, 2});
  auto values = CreateCropInputValues(1, 1, 0, 0, 0, 1);
  const std::atomic_bool shouldCancel = false;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(sourceStore->readCalls >= 4);
  REQUIRE(destinationStore->writeCalls >= 4);
  CHECK(sourceStore->largestReadBytes <= k_CropScratchBytes);
  CHECK(destinationStore->largestWriteBytes <= k_CropScratchBytes);
  usize mismatchIndex = destinationStore->getSize();
  for(usize destinationValueIndex = 0; destinationValueIndex < destinationStore->getSize(); ++destinationValueIndex)
  {
    const usize zIndex = destinationValueIndex / k_Components;
    const usize componentIndex = destinationValueIndex % k_Components;
    const usize sourceValueIndex = (zIndex * 2 + 1) * k_Components + componentIndex;
    if(destinationStore->data()[destinationValueIndex] != static_cast<uint8>(sourceValueIndex % 251))
    {
      mismatchIndex = destinationValueIndex;
      break;
    }
  }
  INFO("First mismatched output value index: " << mismatchIndex);
  REQUIRE(mismatchIndex == destinationStore->getSize());
}

TEST_CASE("SimplnxCore::CropImageGeometry: fallback keeps a small slab transfer", "[SimplnxCore][CropImageGeometryFilter][CropScratch]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_Components = 3;
  auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{3, 6, 8}, ShapeType{k_Components}, false);
  auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{3, 3, 5}, ShapeType{k_Components}, false);
  FillProbeStore(*sourceStore, 113);
  auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{8, 6, 3}, SizeVec3{5, 3, 3});
  auto values = CreateCropInputValues(1, 5, 2, 4, 0, 2);
  const std::atomic_bool shouldCancel = false;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(sourceStore->readCalls == 1);
  REQUIRE(destinationStore->writeCalls == 1);
  CHECK(sourceStore->readBytes.front() + destinationStore->writeBytes.front() <= k_CropScratchBytes);
  for(usize zIndex = 0; zIndex < 3; ++zIndex)
  {
    for(usize yIndex = 0; yIndex < 3; ++yIndex)
    {
      for(usize xIndex = 0; xIndex < 5; ++xIndex)
      {
        for(usize componentIndex = 0; componentIndex < k_Components; ++componentIndex)
        {
          const usize sourceValueIndex = (((zIndex * 6 + yIndex + 2) * 8 + xIndex + 1) * k_Components) + componentIndex;
          const usize destinationValueIndex = (((zIndex * 3 + yIndex) * 5 + xIndex) * k_Components) + componentIndex;
          CHECK(destinationStore->data()[destinationValueIndex] == static_cast<int32>(sourceValueIndex % 113));
        }
      }
    }
  }
}

TEST_CASE("SimplnxCore::CropImageGeometry: fallback propagates bulk transfer errors", "[SimplnxCore][CropImageGeometryFilter][CropScratch]")
{
  UnitTest::LoadPlugins();

  SECTION("source read")
  {
    auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{2, 3, 4}, ShapeType{1}, false);
    auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{2, 1, 2}, ShapeType{1}, false, 7);
    FillProbeStore(*sourceStore, 17);
    sourceStore->failRead = true;
    auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{4, 3, 2}, SizeVec3{2, 1, 2});
    auto values = CreateCropInputValues(1, 2, 1, 1, 0, 1);
    const std::atomic_bool shouldCancel = false;

    const auto result = RunCrop(dataStructure, values, shouldCancel);

    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -95100);
    CHECK(sourceStore->readCalls == 1);
    CHECK(destinationStore->writeCalls == 0);
    CHECK(std::all_of(destinationStore->data(), destinationStore->data() + destinationStore->getSize(), [](int32 value) { return value == -1; }));
  }

  SECTION("destination write")
  {
    auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{33, 3, 4}, ShapeType{1}, false);
    auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{33, 1, 2}, ShapeType{1}, false, 7);
    FillProbeStore(*sourceStore, 17);
    destinationStore->failWrite = true;
    auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{4, 3, 33}, SizeVec3{2, 1, 33});
    auto values = CreateCropInputValues(1, 2, 1, 1, 0, 32);
    const std::atomic_bool shouldCancel = false;

    const auto result = RunCrop(dataStructure, values, shouldCancel);

    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -95101);
    CHECK(sourceStore->readCalls == 1);
    CHECK(destinationStore->writeCalls == 1);
    CHECK(std::all_of(destinationStore->data(), destinationStore->data() + destinationStore->getSize(), [](int32 value) { return value == -1; }));
  }
}

TEST_CASE("SimplnxCore::CropImageGeometry: pre-cancellation preserves destination values", "[SimplnxCore][CropImageGeometryFilter][CropScratch]")
{
  UnitTest::LoadPlugins();
  auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{2, 3, 4}, ShapeType{1}, false);
  auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{2, 1, 2}, ShapeType{1}, false, 7);
  FillProbeStore(*sourceStore, 17);
  auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{4, 3, 2}, SizeVec3{2, 1, 2});
  auto values = CreateCropInputValues(1, 2, 1, 1, 0, 1);
  const std::atomic_bool shouldCancel = true;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(sourceStore->readCalls == 0);
  CHECK(destinationStore->writeCalls == 0);
  CHECK(std::all_of(destinationStore->data(), destinationStore->data() + destinationStore->getSize(), [](int32 value) { return value == 7; }));
}

TEST_CASE("SimplnxCore::CropImageGeometry: fallback copies a one-value tail", "[SimplnxCore][CropImageGeometryFilter][CropScratch]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_CropX = (k_CropScratchBytes / sizeof(int32)) + 1;
  constexpr usize k_SourceX = k_CropX + 2;
  auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{1, 1, k_SourceX}, ShapeType{1}, false);
  auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{1, 1, k_CropX}, ShapeType{1}, false);
  FillProbeStore(*sourceStore, 127);
  auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{k_SourceX, 1, 1}, SizeVec3{k_CropX, 1, 1});
  auto values = CreateCropInputValues(1, k_CropX, 0, 0, 0, 0);
  const std::atomic_bool shouldCancel = false;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(sourceStore->readBytes == std::vector<usize>{k_CropScratchBytes, sizeof(int32)});
  REQUIRE(destinationStore->writeBytes == std::vector<usize>{k_CropScratchBytes, sizeof(int32)});
  CHECK(destinationStore->data()[k_CropX - 1] == static_cast<int32>(k_CropX % 127));
}

TEST_CASE("SimplnxCore::CropImageGeometry: fallback cancellation preserves completed segments", "[SimplnxCore][CropImageGeometryFilter][CropScratch]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_CropX = (k_CropScratchBytes / sizeof(int32)) + 1;
  constexpr usize k_SourceX = k_CropX + 2;
  auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{1, 1, k_SourceX}, ShapeType{1}, false);
  auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{1, 1, k_CropX}, ShapeType{1}, false, 7);
  FillProbeStore(*sourceStore, 127);
  auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{k_SourceX, 1, 1}, SizeVec3{k_CropX, 1, 1});
  auto values = CreateCropInputValues(1, k_CropX, 0, 0, 0, 0);
  std::atomic_bool shouldCancel = false;
  destinationStore->cancelAfterWrite = &shouldCancel;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(shouldCancel.load());
  CHECK(sourceStore->readCalls == 1);
  CHECK(destinationStore->writeCalls == 1);
  CHECK(destinationStore->data()[0] == 1);
  CHECK(destinationStore->data()[(k_CropScratchBytes / sizeof(int32)) - 1] == static_cast<int32>((k_CropScratchBytes / sizeof(int32)) % 127));
  CHECK(destinationStore->data()[k_CropX - 1] == -1);
}

TEST_CASE("SimplnxCore::CropImageGeometry: rejects a store size that disagrees with the source geometry", "[SimplnxCore][CropImageGeometryFilter][CropScratch]")
{
  UnitTest::LoadPlugins();
  auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{2, 3, 3}, ShapeType{1}, false);
  auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{2, 1, 2}, ShapeType{1}, false);
  auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{4, 3, 2}, SizeVec3{2, 1, 2});
  auto values = CreateCropInputValues(1, 2, 1, 1, 0, 1);
  const std::atomic_bool shouldCancel = false;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == -953);
  CHECK(sourceStore->readCalls == 0);
  CHECK(destinationStore->writeCalls == 0);
}

TEST_CASE("SimplnxCore::CropImageGeometry: real OOC stores preserve cropped values", "[SimplnxCore][CropImageGeometryFilter][CropScratch][OOC]")
{
  UnitTest::LoadPlugins();
  auto& ioCollection = DataStoreUtilities::GetIOCollection();
  if(!ioCollection.hasDataStoreCreationFunction("HDF5-OOC"))
  {
#if SIMPLNX_TEST_ALGORITHM_PATH == 1
    FAIL("The OOC-only build did not register HDF5-OOC storage.");
#endif
    return;
  }

  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  DataStructure dataStructure;
  const DataPath sourceGeomPath({"Source"});
  const DataPath destinationGeomPath({"Destination"});
  const DataPath sourceArrayPath = sourceGeomPath.createChildPath("Cell Data").createChildPath("Values");
  const DataPath destinationArrayPath = destinationGeomPath.createChildPath("Cell Data").createChildPath("Values");
  CreateImageGeometryAction sourceGeometryAction(sourceGeomPath, {4, 3, 2}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}, "Cell Data");
  CreateImageGeometryAction destinationGeometryAction(destinationGeomPath, {2, 2, 2}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}, "Cell Data");
  auto sourceGeometryActionApplyResult = sourceGeometryAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(sourceGeometryActionApplyResult);
  auto destinationGeometryActionApplyResult = destinationGeometryAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(destinationGeometryActionApplyResult);
  CreateArrayAction sourceArrayAction(DataType::int32, {2, 3, 4}, {2}, sourceArrayPath);
  CreateArrayAction destinationArrayAction(DataType::int32, {2, 2, 2}, {2}, destinationArrayPath);
  auto sourceArrayActionApplyResult = sourceArrayAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(sourceArrayActionApplyResult);
  auto destinationArrayActionApplyResult = destinationArrayAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(destinationArrayActionApplyResult);

  auto& sourceArray = dataStructure.getDataRefAs<Int32Array>(sourceArrayPath);
  auto& destinationArray = dataStructure.getDataRefAs<Int32Array>(destinationArrayPath);
  REQUIRE(sourceArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(destinationArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(sourceArray.getDataFormat() == "HDF5-OOC");
  REQUIRE(destinationArray.getDataFormat() == "HDF5-OOC");
  std::array<int32, 48> sourceValues = {};
  std::iota(sourceValues.begin(), sourceValues.end(), 100);
  auto sourceArrayWriteResult = sourceArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(sourceValues.data(), sourceValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(sourceArrayWriteResult);
  auto values = CreateCropInputValues(1, 2, 1, 2, 0, 1);
  const std::atomic_bool shouldCancel = false;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  std::array<int32, 16> actualValues = {};
  auto destinationArrayReadResult = destinationArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actualValues.data(), actualValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(destinationArrayReadResult);
  constexpr std::array<int32, 16> k_ExpectedValues = {110, 111, 112, 113, 118, 119, 120, 121, 134, 135, 136, 137, 142, 143, 144, 145};
  REQUIRE(actualValues == k_ExpectedValues);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Instantiate)", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> k_MinVector{0, 0, 0};
  const std::vector<uint64> k_MaxVector{0, 0, 0};

  //  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});

  CropImageGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter Invalid Params", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  std::vector<uint64> k_MinVector{0, 0, 0};
  std::vector<uint64> k_MaxVector{500, 20, 30};

  //  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});
  DataStructure dataStructure = CreateDataStructure();

  CropImageGeometryFilter filter;
  Arguments args;

  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // args.insertOrAssign(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insertOrAssign(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  auto preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5553);

  k_MaxVector = {20, 500, 0};
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5554);

  k_MaxVector = {1, 1, 500};
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5555);

  k_MinVector = {10, 10, 10};
  k_MaxVector = {1, 20, 20};
  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -4011);

  k_MinVector = {10, 10, 10};
  k_MaxVector = {20, 1, 20};
  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -4012);

  k_MinVector = {10, 10, 10};
  k_MaxVector = {20, 20, 1};
  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -4013);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter)", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 3; i++)
  {
    REQUIRE(newDimensions[i] == (k_MaxVector[i] - k_MinVector[i] + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - XY", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 2; i++)
  {
    REQUIRE(newDimensions[i] == (k_MaxVector[i] - k_MinVector[i] + 1));
  }
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_XY_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - XZ", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == (k_MaxVector[0] - k_MinVector[0] + 1));
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == (k_MaxVector[2] - k_MinVector[2] + 1));

  DataPath exemplarGeoPath({"6_5_Cropped_XZ_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - YZ", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == (k_MaxVector[1] - k_MinVector[1] + 1));
  REQUIRE(newDimensions[2] == (k_MaxVector[2] - k_MinVector[2] + 1));

  DataPath exemplarGeoPath({"6_5_Cropped_YZ_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - X", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == (k_MaxVector[0] - k_MinVector[0] + 1));
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_X_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - Y", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == (k_MaxVector[1] - k_MinVector[1] + 1));
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_Y_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - Z", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> k_MinVector{10, 15, 0};
  const std::vector<uint64> k_MaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  {
    // Write out the DataStructure for later viewing/debugging
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == (k_MaxVector[2] - k_MinVector[2] + 1));

  DataPath exemplarGeoPath({"6_5_Cropped_Z_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 3; i++)
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[i] - origin[i]) / spacing[i]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[i] - origin[i]) / spacing[i]));
    REQUIRE(newDimensions[i] == (max - min + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop XY Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 2; i++)
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[i] - origin[i]) / spacing[i]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[i] - origin[i]) / spacing[i]));
    REQUIRE(newDimensions[i] == (max - min + 1));
  }
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_XY_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop XZ Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[0] - origin[0]) / spacing[0]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[0] - origin[0]) / spacing[0]));
    REQUIRE(newDimensions[0] == (max - min + 1));
  }
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[2] - origin[2]) / spacing[2]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[2] - origin[2]) / spacing[2]));
    REQUIRE(newDimensions[2] == (max - min + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_XZ_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop YZ Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  for(usize i = 1; i < 3; i++)
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[i] - origin[i]) / spacing[i]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[i] - origin[i]) / spacing[i]));
    REQUIRE(newDimensions[i] == (max - min + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_YZ_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop X Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[0] - origin[0]) / spacing[0]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[0] - origin[0]) / spacing[0]));
    REQUIRE(newDimensions[0] == (max - min + 1));
  }
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_X_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop Y Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[1] - origin[1]) / spacing[1]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[1] - origin[1]) / spacing[1]));
    REQUIRE(newDimensions[1] == (max - min + 1));
  }
  REQUIRE(newDimensions[2] == imageGeom.getNumZCells());

  DataPath exemplarGeoPath({"6_5_Cropped_Y_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop Z Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> k_MinVector{-5, 57.5, 30};
  const std::vector<float64> k_MaxVector{20, 70, 55};

  const DataPath k_ImageGeomPath({k_DataContainer});
  const DataPath k_NewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath k_DestCellFeatureDataPath = k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(k_MinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(k_MaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  {
    auto min = static_cast<uint64>(std::floor((k_MinVector[2] - origin[2]) / spacing[2]));
    auto max = static_cast<uint64>(std::floor((k_MaxVector[2] - origin[2]) / spacing[2]));
    REQUIRE(newDimensions[2] == (max - min + 1));
  }

  DataPath exemplarGeoPath({"6_5_Cropped_Z_ImageGeom"});
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_CellData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_CellFeatureData);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ::ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, k_DestCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CropImageGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CropImageGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CropImageGeometryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CropImageGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (UInt64ToVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (UInt64ToVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<bool>(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key) == false);
      CHECK(args.value<DataPath>(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<bool>(CropImageGeometryFilter::k_RenumberFeatures_Key) == true);
      CHECK(args.value<DataPath>(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
    }
  }
}
