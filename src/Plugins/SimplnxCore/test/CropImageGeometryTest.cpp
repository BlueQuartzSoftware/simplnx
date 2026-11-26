#include "SimplnxCore/Filters/Algorithms/CropImageGeometry.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <typeinfo>
#include <utility>
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
  : DataStore<T>(std::move(tupleShape), std::move(componentShape), initValue)
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
    ++read_calls;
    read_bytes.push_back(buffer.size() * sizeof(T));
    largest_read_bytes = (std::max)(largest_read_bytes, read_bytes.back());
    if(fail_read)
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
    ++write_calls;
    write_bytes.push_back(buffer.size() * sizeof(T));
    largest_write_bytes = (std::max)(largest_write_bytes, write_bytes.back());
    if(fail_write)
    {
      return MakeErrorResult(-95101, "Injected crop destination write failure.");
    }
    auto result = DataStore<T>::copyFromBuffer(startIndex, buffer);
    if(result.valid() && cancel_after_write != nullptr)
    {
      cancel_after_write->store(true);
    }
    return result;
  }

  bool fail_read = false;
  bool fail_write = false;
  std::atomic_bool* cancel_after_write = nullptr;
  mutable usize read_calls = 0;
  usize write_calls = 0;
  mutable usize largest_read_bytes = 0;
  usize largest_write_bytes = 0;
  mutable std::vector<usize> read_bytes;
  std::vector<usize> write_bytes;

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

  auto* sourceGeomPtr = ImageGeom::Create(dataStructure, "Source");
  auto* destinationGeomPtr = ImageGeom::Create(dataStructure, "Destination");
  REQUIRE(sourceGeomPtr != nullptr);
  REQUIRE(destinationGeomPtr != nullptr);
  sourceGeomPtr->setDimensions(sourceDimensions);
  destinationGeomPtr->setDimensions(cropDimensions);
  REQUIRE(sourceGeomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  REQUIRE(destinationGeomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  REQUIRE(sourceGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  REQUIRE(destinationGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());

  auto* sourceCellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", sourceStore->getTupleShape(), sourceGeomPtr->getId());
  auto* destinationCellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", destinationStore->getTupleShape(), destinationGeomPtr->getId());
  REQUIRE(sourceCellDataPtr != nullptr);
  REQUIRE(destinationCellDataPtr != nullptr);
  sourceGeomPtr->setCellData(*sourceCellDataPtr);
  destinationGeomPtr->setCellData(*destinationCellDataPtr);
  REQUIRE(DataArray<T>::Create(dataStructure, "Values", sourceStore, sourceCellDataPtr->getId()) != nullptr);
  REQUIRE(DataArray<T>::Create(dataStructure, "Values", destinationStore, destinationCellDataPtr->getId()) != nullptr);
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
  DataGroup* topLevelGroupPtr = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanDataPtr = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroupPtr->getId());

  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, scanDataPtr->getId());
  REQUIRE(imageGeomPtr->setSpacing({0.25f, 0.55f, 1.86}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 20.0f, 66.0f}).valid());
  SizeVec3 imageGeomDims = {40, 60, 80};
  imageGeomPtr->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  auto imageDimsArray = imageGeomDims.toArray();
  ShapeType cellDataDims{imageDimsArray.crbegin(), imageDimsArray.crend()};
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, ImageGeom::k_CellAttributeMatrixName, cellDataDims, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);

  Int32Array* phasesDataPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", cellDataDims, {1}, cellDataPtr->getId());

  return dataStructure;
}

/**
 * @brief Creates four columns of features and independent Feature and Ensemble data.
 * @return Image geometry with FeatureId x + 1 at each cell (x, y, 0).
 */
DataStructure CreateFeatureCropFixture()
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 4, 1});
  REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());

  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1, 4, 4}, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);
  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", cellDataPtr->getShape(), {1}, cellDataPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  for(usize y = 0; y < 4; ++y)
  {
    for(usize x = 0; x < 4; ++x)
    {
      (*featureIdsPtr)[y * 4 + x] = static_cast<int32>(x + 1);
    }
  }

  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, "Cell Feature Data", ShapeType{5}, imageGeomPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  auto* numElementsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumElements", {5}, {1}, featureDataPtr->getId());
  REQUIRE(numElementsPtr != nullptr);
  numElementsPtr->fill(4);
  (*numElementsPtr)[0] = 0;
  auto* neighborsPtr = NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{5}, featureDataPtr->getId());
  REQUIRE(neighborsPtr != nullptr);
  neighborsPtr->setLists({{}, {2}, {1, 3}, {2, 4}, {3}});
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Names", ShapeType{5}, {"Background", "One", "Two", "Three", "Four"}, featureDataPtr->getId()) != nullptr);

  auto* ensembleDataPtr = AttributeMatrix::Create(dataStructure, "Cell Ensemble Data", ShapeType{2}, imageGeomPtr->getId());
  REQUIRE(ensembleDataPtr != nullptr);
  auto* crystalStructuresPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleDataPtr->getId());
  REQUIRE(crystalStructuresPtr != nullptr);
  (*crystalStructuresPtr)[0] = 999;
  (*crystalStructuresPtr)[1] = 1;
  return dataStructure;
}

/**
 * @brief Selects the last two columns of the feature fixture using inclusive voxel bounds.
 * @param clear Whether to clear the destination Feature Attribute Matrix.
 * @param renumber Stored renumber setting, effective only when clear is true.
 * @return Arguments that preserve the source and create the Cropped geometry.
 */
Arguments MakeFeatureCropArgs(bool clear, bool renumber)
{
  Arguments args;
  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{2, 0, 0}));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{3, 3, 0}));
  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Cropped"})));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(clear));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(renumber));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Feature Data"})));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "FeatureIds"})));
  return args;
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
  CHECK(sourceStore->read_calls == 0);
  CHECK(destinationStore->write_calls == 0);
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
  REQUIRE(sourceStore->read_calls >= 4);
  REQUIRE(destinationStore->write_calls >= 4);
  CHECK(sourceStore->largest_read_bytes <= k_CropScratchBytes);
  CHECK(destinationStore->largest_write_bytes <= k_CropScratchBytes);
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
  REQUIRE(sourceStore->read_bytes == std::vector<usize>{k_AlignedSegmentBytes, k_TailBytes});
  REQUIRE(destinationStore->write_bytes == sourceStore->read_bytes);
  for(const usize transferBytes : sourceStore->read_bytes)
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
  REQUIRE(sourceStore->read_calls >= 4);
  REQUIRE(destinationStore->write_calls >= 4);
  CHECK(sourceStore->largest_read_bytes <= k_CropScratchBytes);
  CHECK(destinationStore->largest_write_bytes <= k_CropScratchBytes);
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
  REQUIRE(sourceStore->read_calls == 1);
  REQUIRE(destinationStore->write_calls == 1);
  CHECK(sourceStore->read_bytes.front() + destinationStore->write_bytes.front() <= k_CropScratchBytes);
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
    sourceStore->fail_read = true;
    auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{4, 3, 2}, SizeVec3{2, 1, 2});
    auto values = CreateCropInputValues(1, 2, 1, 1, 0, 1);
    const std::atomic_bool shouldCancel = false;

    const auto result = RunCrop(dataStructure, values, shouldCancel);

    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -95100);
    CHECK(sourceStore->read_calls == 1);
    CHECK(destinationStore->write_calls == 0);
    CHECK(std::all_of(destinationStore->data(), destinationStore->data() + destinationStore->getSize(), [](int32 value) { return value == -1; }));
  }

  SECTION("destination write")
  {
    auto sourceStore = std::make_shared<CropProbeStore<int32>>(ShapeType{33, 3, 4}, ShapeType{1}, false);
    auto destinationStore = std::make_shared<CropProbeStore<int32>>(ShapeType{33, 1, 2}, ShapeType{1}, false, 7);
    FillProbeStore(*sourceStore, 17);
    destinationStore->fail_write = true;
    auto dataStructure = CreateCropProbeData(sourceStore, destinationStore, SizeVec3{4, 3, 33}, SizeVec3{2, 1, 33});
    auto values = CreateCropInputValues(1, 2, 1, 1, 0, 32);
    const std::atomic_bool shouldCancel = false;

    const auto result = RunCrop(dataStructure, values, shouldCancel);

    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -95101);
    CHECK(sourceStore->read_calls == 1);
    CHECK(destinationStore->write_calls == 1);
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
  CHECK(sourceStore->read_calls == 0);
  CHECK(destinationStore->write_calls == 0);
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
  REQUIRE(sourceStore->read_bytes == std::vector<usize>{k_CropScratchBytes, sizeof(int32)});
  REQUIRE(destinationStore->write_bytes == std::vector<usize>{k_CropScratchBytes, sizeof(int32)});
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
  destinationStore->cancel_after_write = &shouldCancel;

  const auto result = RunCrop(dataStructure, values, shouldCancel);

  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(shouldCancel.load());
  CHECK(sourceStore->read_calls == 1);
  CHECK(destinationStore->write_calls == 1);
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
  CHECK(sourceStore->read_calls == 0);
  CHECK(destinationStore->write_calls == 0);
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

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Clear ON, renumber OFF preflight", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  REQUIRE(AttributeMatrix::Create(dataStructure, "Other Feature Data", {5}, dataStructure.getId(DataPath({"Image"}))) != nullptr);
  CropImageGeometryFilter filter;
  auto result = filter.preflight(dataStructure, MakeFeatureCropArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  REQUIRE(result.outputActions.warnings().size() == 1);
  const auto& warning = result.outputActions.warnings().front();
  CHECK(warning.code == -4041);
  CHECK(warning.message == "These Attribute Matrices are copied to the cropped geometry unchanged:\n"
                           "Image/Cell Ensemble Data\n"
                           "Image/Other Feature Data\n"
                           "If one holds Feature data, its values describe the original geometry. Recompute them. Ensemble arrays retain their stored values. "
                           "Recompute any Ensemble statistics that depend on the geometry's cells or elements.");
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  const auto it = std::find_if(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; });
  REQUIRE(it != result.outputValues.end());
  CHECK(it->value.find("Image/Cell Feature Data/NumElements") != std::string::npos);
  CHECK(it->value.find("Image/Cell Feature Data/NeighborList") != std::string::npos);
  CHECK(it->value.find("Image/Cell Feature Data/Names") != std::string::npos);
  CHECK(it->value.find("Ensemble") == std::string::npos);
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Invalidated NeighborLists" || v.name == "Feature Data Not Cleared"; }));
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Clear OFF preflight", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  CropImageGeometryFilter filter;
  auto result = filter.preflight(dataStructure, MakeFeatureCropArgs(false, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  REQUIRE(result.outputActions.warnings().size() == 1);
  const auto& warning = result.outputActions.warnings().front();
  CHECK(warning.code == -4041);
  CHECK(warning.message.find("\nImage/Cell Feature Data\n") != std::string::npos);
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  CHECK(std::any_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Feature Data Not Cleared"; }));
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; }));
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Clear OFF ignores stored renumber", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  CropImageGeometryFilter filter;
  Arguments args = MakeFeatureCropArgs(false, true);
  args.insertOrAssign(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "DoesNotExist"})));
  args.insertOrAssign(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"DoesNotExist"})));
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  REQUIRE(result.outputActions.warnings().size() == 1);
  CHECK(result.outputActions.warnings().front().code == -4041);
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: StrictFeatureIds preflight", "[SimplnxCore][CropImageGeometryFilter][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(DataPath({"Image"}));
  REQUIRE(imageGeomPtr != nullptr);
  const auto* cellDataPtr = imageGeomPtr->getCellData();
  REQUIRE(cellDataPtr != nullptr);
  auto* otherDataPtr = AttributeMatrix::Create(dataStructure, "Other Cell Data", cellDataPtr->getShape(), imageGeomPtr->getId());
  REQUIRE(otherDataPtr != nullptr);
  auto* misplacedIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "MisplacedFeatureIds", otherDataPtr->getShape(), {1}, otherDataPtr->getId());
  REQUIRE(misplacedIdsPtr != nullptr);
  misplacedIdsPtr->fill(1);

  CropImageGeometryFilter filter;
  Arguments args = MakeFeatureCropArgs(true, true);
  args.insertOrAssign(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Other Cell Data", "MisplacedFeatureIds"})));

  SECTION("Renumber ON rejects Feature Ids in another Attribute Matrix")
  {
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors().front().code == -50564);
    CHECK(result.outputActions.errors().front().message ==
          "**Cell Feature Ids** DataArray 'Image/Other Cell Data/MisplacedFeatureIds' must be in the Cell Attribute Matrix 'Image/Cell Data' of the selected Image Geometry 'Image'. "
          "Select the DataArray from that Attribute Matrix.");
  }
  SECTION("Renumber OFF accepts Feature Ids in another Attribute Matrix")
  {
    args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Renumber ON accepts Feature Ids in the Cell Attribute Matrix")
  {
    args.insertOrAssign(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "FeatureIds"})));
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Clear ON Feature AM errors", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  // An Attribute Matrix that is not a child of "Image".
  REQUIRE(AttributeMatrix::Create(dataStructure, "Elsewhere", std::vector<usize>{5}) != nullptr);
  CropImageGeometryFilter filter;

  Arguments args = MakeFeatureCropArgs(true, false);
  args.insertOrAssign(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Elsewhere"})));
  auto notChild = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(notChild.outputActions);
  REQUIRE(notChild.outputActions.errors().size() == 1);
  CHECK(notChild.outputActions.errors().front().code == -50563);
  CHECK(notChild.outputActions.errors().front().message ==
        "The Attribute Matrix 'Elsewhere' selected for **Feature Attribute Matrix** must be a direct child of the selected geometry 'Image' and must not be its Cell Attribute Matrix 'Image/Cell "
        "Data'. Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.");

  args.insertOrAssign(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data"})));
  auto isCellAm = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(isCellAm.outputActions);
  REQUIRE(isCellAm.outputActions.errors().size() == 1);
  CHECK(isCellAm.outputActions.errors().front().code == -50563);
  CHECK(isCellAm.outputActions.errors().front().message ==
        "The Attribute Matrix 'Image/Cell Data' selected for **Feature Attribute Matrix** must be a direct child of the selected geometry 'Image' and must not be its Cell Attribute Matrix "
        "'Image/Cell Data'. Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.");

  args.insertOrAssign(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Image", "Missing"})));
  auto missing = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(missing.outputActions);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Clear ON, renumber OFF execute", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  CropImageGeometryFilter filter;
  auto result = filter.execute(dataStructure, MakeFeatureCropArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Cell Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Cell Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 0);
  // The cell FeatureIds keep their original values (3 and 4 for x = 2 and 3).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Cropped", "Cell Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Cropped", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {3, 4, 3, 4, 3, 4, 3, 4};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  // The Ensemble AM is copied unchanged.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  // The source geometry is untouched (debugging route).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})));
  CHECK(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})).getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Clear ON, renumber ON execute", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  CropImageGeometryFilter filter;
  auto result = filter.execute(dataStructure, MakeFeatureCropArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Cell Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Cell Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{3}); // features 3 and 4 remain, plus row 0
  CHECK(am.getSize() == 0);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Cropped", "Cell Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Cropped", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 2, 1, 2, 1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Clear OFF execute copies Feature AM unchanged", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  CropImageGeometryFilter filter;
  const bool renumber = GENERATE(false, true);
  CAPTURE(renumber);
  auto result = filter.execute(dataStructure, MakeFeatureCropArgs(false, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Cropped", "Cell Feature Data", "NumElements"})));
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(DataPath({"Cropped", "Cell Feature Data", "NumElements"}));
  REQUIRE(numElements.getNumberOfTuples() == 5);
  const std::array<int32, 5> expectedNumElements = {0, 4, 4, 4, 4};
  for(usize i = 0; i < expectedNumElements.size(); ++i)
  {
    CHECK(numElements[i] == expectedNumElements[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Cell Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Cell Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 3);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Cropped", "Cell Feature Data", "NeighborList"})));
  const auto& neighbors = dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Cropped", "Cell Feature Data", "NeighborList"}));
  const std::vector<std::vector<int32>> expectedNeighbors = {{}, {2}, {1, 3}, {2, 4}, {3}};
  REQUIRE(neighbors.getNumberOfTuples() == expectedNeighbors.size());
  for(usize i = 0; i < expectedNeighbors.size(); ++i)
  {
    CHECK(neighbors.getList(static_cast<int32>(i)) == expectedNeighbors[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(DataPath({"Cropped", "Cell Feature Data", "Names"})));
  const auto& names = dataStructure.getDataRefAs<StringArray>(DataPath({"Cropped", "Cell Feature Data", "Names"}));
  const std::array<std::string, 5> expectedNames = {"Background", "One", "Two", "Three", "Four"};
  REQUIRE(names.getNumberOfTuples() == expectedNames.size());
  for(usize i = 0; i < expectedNames.size(); ++i)
  {
    CHECK(names[i] == expectedNames[i]);
  }
  // The stored renumber=true is ignored.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Cropped", "Cell Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Cropped", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {3, 4, 3, 4, 3, 4, 3, 4};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Clear ON in place", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  CropImageGeometryFilter filter;
  Arguments args = MakeFeatureCropArgs(true, true);
  args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"})));
  const auto& am = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"}));
  CHECK(am.getSize() == 0);
  CHECK(am.getShape() == std::vector<usize>{3});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image"})));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image"}));
  CHECK(imageGeom.getDimensions() == SizeVec3(2, 4, 1));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "FeatureIds"})));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Image", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  const std::array<int32, 8> expectedIds = {1, 2, 1, 2, 1, 2, 1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  CHECK(dataStructure.getData(DataPath({"Cropped"})) == nullptr);
  CHECK(dataStructure.getData(DataPath({".Image"})) == nullptr);
  CHECK(dataStructure.getData(DataPath({".cropped_image_geometry"})) == nullptr);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(DataPath({"Image", "Cell Ensemble Data", "CrystalStructures"})));
  const auto& xtal = dataStructure.getDataRefAs<UInt32Array>(DataPath({"Image", "Cell Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Instantiate)", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> kMinVector{0, 0, 0};
  const std::vector<uint64> kMaxVector{0, 0, 0};

  //  static constexpr bool k_UpdateOrigin = false;
  const DataPath kImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath kNewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath kFeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});

  CropImageGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  // args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
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

  std::vector<uint64> kMinVector{0, 0, 0};
  std::vector<uint64> kMaxVector{500, 20, 30};

  //  static constexpr bool k_UpdateOrigin = false;
  const DataPath kImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath kNewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath kFeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});
  DataStructure dataStructure = CreateDataStructure();

  CropImageGeometryFilter filter;
  Arguments args;

  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  // args.insertOrAssign(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insertOrAssign(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insertOrAssign(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  auto preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5553);

  kMaxVector = {20, 500, 0};
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5554);

  kMaxVector = {1, 1, 500};
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -5555);

  kMinVector = {10, 10, 10};
  kMaxVector = {1, 20, 20};
  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -4011);

  kMinVector = {10, 10, 10};
  kMaxVector = {20, 1, 20};
  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  // Preflight the filter and check result
  preflightResult = filter.preflight(dataStructure, args);
  preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == -4012);

  kMinVector = {10, 10, 10};
  kMaxVector = {20, 20, 1};
  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
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

  const std::vector<uint64> kMinVector{10, 15, 0};
  const std::vector<uint64> kMaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 3; i++)
  {
    REQUIRE(newDimensions[i] == (kMaxVector[i] - kMinVector[i] + 1));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - XY", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> kMinVector{10, 15, 0};
  const std::vector<uint64> kMaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 2; i++)
  {
    REQUIRE(newDimensions[i] == (kMaxVector[i] - kMinVector[i] + 1));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - XZ", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> kMinVector{10, 15, 0};
  const std::vector<uint64> kMaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == (kMaxVector[0] - kMinVector[0] + 1));
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == (kMaxVector[2] - kMinVector[2] + 1));

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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - YZ", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> kMinVector{10, 15, 0};
  const std::vector<uint64> kMaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == (kMaxVector[1] - kMinVector[1] + 1));
  REQUIRE(newDimensions[2] == (kMaxVector[2] - kMinVector[2] + 1));

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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - X", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> kMinVector{10, 15, 0};
  const std::vector<uint64> kMaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == (kMaxVector[0] - kMinVector[0] + 1));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - Y", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> kMinVector{10, 15, 0};
  const std::vector<uint64> kMaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == (kMaxVector[1] - kMinVector[1] + 1));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter(Execute_Filter) - Z", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<uint64> kMinVector{10, 15, 0};
  const std::vector<uint64> kMaxVector{60, 40, 50};

  // static constexpr bool k_UpdateOrigin = false;
  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(kMaxVector));
  //  args.insert(CropImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  REQUIRE(newDimensions[2] == (kMaxVector[2] - kMinVector[2] + 1));

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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> kMinVector{-5, 57.5, 30};
  const std::vector<float64> kMaxVector{20, 70, 55};

  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(kMaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

  //    const auto oldDimensions = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();
  //    const auto oldOrigin = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getOrigin();
  //    const auto oldSpacing = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath).getSpacing();
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 3; i++)
  {
    auto min = static_cast<uint64>(std::floor((kMinVector[i] - origin[i]) / spacing[i]));
    auto max = static_cast<uint64>(std::floor((kMaxVector[i] - origin[i]) / spacing[i]));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Reject NaN Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const usize axis = GENERATE(0, 1, 2);
  CAPTURE(axis);
  std::vector<float64> minCoord{-5, 57.5, 30};
  std::vector<float64> maxCoord{20, 70, 55};
  SECTION("NaN minimum")
  {
    minCoord[axis] = std::numeric_limits<float64>::quiet_NaN();
  }
  SECTION("NaN maximum")
  {
    maxCoord[axis] = std::numeric_limits<float64>::quiet_NaN();
  }

  const DataPath imageGeomPath({k_DataContainer});
  const DataPath newImageGeomPath({"7_0_Cropped_ImageGeom"});
  const DataPath featureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath cellFeatureAMPath({k_DataContainer, Constants::k_CellFeatureData});
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");
  const auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  Arguments args;
  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(minCoord));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(maxCoord));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(newImageGeomPath));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(cellFeatureAMPath));
  args.insert(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropImageGeometryFilter::k_CropXDim_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_CropYDim_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_CropZDim_Key, std::make_any<bool>(true));

  CropImageGeometryFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  const auto& errors = preflightResult.outputActions.errors();
  REQUIRE(std::any_of(errors.begin(), errors.end(), [](const auto& error) { return error.code == -50561; }));
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop XY Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> kMinVector{-5, 57.5, 30};
  const std::vector<float64> kMaxVector{20, 70, 55};

  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(kMaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 2; i++)
  {
    auto min = static_cast<uint64>(std::floor((kMinVector[i] - origin[i]) / spacing[i]));
    auto max = static_cast<uint64>(std::floor((kMaxVector[i] - origin[i]) / spacing[i]));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop XZ Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> kMinVector{-5, 57.5, 30};
  const std::vector<float64> kMaxVector{20, 70, 55};

  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(kMaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  {
    auto min = static_cast<uint64>(std::floor((kMinVector[0] - origin[0]) / spacing[0]));
    auto max = static_cast<uint64>(std::floor((kMaxVector[0] - origin[0]) / spacing[0]));
    REQUIRE(newDimensions[0] == (max - min + 1));
  }
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  {
    auto min = static_cast<uint64>(std::floor((kMinVector[2] - origin[2]) / spacing[2]));
    auto max = static_cast<uint64>(std::floor((kMaxVector[2] - origin[2]) / spacing[2]));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop YZ Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> kMinVector{-5, 57.5, 30};
  const std::vector<float64> kMaxVector{20, 70, 55};

  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(kMaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  for(usize i = 1; i < 3; i++)
  {
    auto min = static_cast<uint64>(std::floor((kMinVector[i] - origin[i]) / spacing[i]));
    auto max = static_cast<uint64>(std::floor((kMaxVector[i] - origin[i]) / spacing[i]));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop X Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> kMinVector{-5, 57.5, 30};
  const std::vector<float64> kMaxVector{20, 70, 55};

  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(kMaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  {
    auto min = static_cast<uint64>(std::floor((kMinVector[0] - origin[0]) / spacing[0]));
    auto max = static_cast<uint64>(std::floor((kMaxVector[0] - origin[0]) / spacing[0]));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop Y Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> kMinVector{-5, 57.5, 30};
  const std::vector<float64> kMaxVector{20, 70, 55};

  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(kMaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  {
    auto min = static_cast<uint64>(std::floor((kMinVector[1] - origin[1]) / spacing[1]));
    auto max = static_cast<uint64>(std::floor((kMaxVector[1] - origin[1]) / spacing[1]));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Crop Z Physical Bounds", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float64> kMinVector{-5, 57.5, 30};
  const std::vector<float64> kMaxVector{20, 70, 55};

  const DataPath kImageGeomPath({k_DataContainer});
  const DataPath kNewImageGeomPath({"7_0_Cropped_ImageGeom"});
  DataPath destCellDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellData);
  static constexpr bool k_RenumberFeatures = true;
  const DataPath kFeatureIdsPath({k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath kCellFeatureAmPath({k_DataContainer, Constants::k_CellFeatureData});
  DataPath kDestCellFeatureDataPath = kNewImageGeomPath.createChildPath(Constants::k_CellFeatureData);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  CropImageGeometryFilter filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float64>>(kMinVector));
  args.insert(CropImageGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float64>>(kMaxVector));
  args.insert(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(kNewImageGeomPath));
  args.insert(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(true));
  args.insert(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insert(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(kCellFeatureAmPath));
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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  auto& newImageGeom = dataStructure.getDataRefAs<ImageGeom>(kNewImageGeomPath);
  auto origin = imageGeom.getOrigin();
  auto spacing = imageGeom.getSpacing();
  auto newDimensions = newImageGeom.getDimensions();

  REQUIRE(newDimensions[0] == imageGeom.getNumXCells());
  REQUIRE(newDimensions[1] == imageGeom.getNumYCells());
  {
    auto min = static_cast<uint64>(std::floor((kMinVector[2] - origin[2]) / spacing[2]));
    auto max = static_cast<uint64>(std::floor((kMaxVector[2] - origin[2]) / spacing[2]));
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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath));
  const auto& exemplarFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto& calculatedFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(kDestCellFeatureDataPath);
  CHECK(calculatedFeatureAM.getNumberOfTuples() == exemplarFeatureAM.getNumberOfTuples());
  CHECK(calculatedFeatureAM.getSize() == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CropImageGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CropImageGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CropImageGeometryFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<CropImageGeometryFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      // Complex type (UInt64ToVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (UInt64ToVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<bool>(CropImageGeometryFilter::k_UsePhysicalBounds_Key) == false);
      CHECK(args.value<bool>(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key) == false);
      CHECK(args.value<DataPath>(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<bool>(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key) == true);
      CHECK(args.value<bool>(CropImageGeometryFilter::k_RenumberFeatures_Key) == true);
      CHECK(args.value<DataPath>(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(CropImageGeometryFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
    }
  }
}

TEST_CASE("SimplnxCore::CropImageGeometry: Child names containing the geometry name are kept", "[SimplnxCore][CropImageGeometry]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath srcGeomPath({"Geom"});
  const DataPath destGeomPath({"Geom Out"});
  auto* geomPtr = ImageGeom::Create(dataStructure, "Geom");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({2, 2, 2});
  REQUIRE(geomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  REQUIRE(geomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* elementAmPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 2, 2}, geomPtr->getId());
  REQUIRE(elementAmPtr != nullptr);
  geomPtr->setCellData(*elementAmPtr);
  auto* dataPtr =
      Int32Array::Create(dataStructure, "Data", DataStoreUtilities::CreateDataStore<int32>(dataStructure, elementAmPtr->getDataPaths().front().createChildPath("Data"), elementAmPtr->getShape(), {1}),
                         elementAmPtr->getId());
  REQUIRE(dataPtr != nullptr);
  dataPtr->fill(7);
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "Geom Feature Data", {2}, geomPtr->getId());
  REQUIRE(featureAmPtr != nullptr);
  auto* valuesPtr = Int32Array::Create(dataStructure, "Geom Values",
                                       DataStoreUtilities::CreateDataStore<int32>(dataStructure, featureAmPtr->getDataPaths().front().createChildPath("Geom Values"), {2}, {1}), featureAmPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  valuesPtr->fill(42);
  CropImageGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(destGeomPath));
  args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{1, 1, 1}));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const DataPath copiedAmPath = destGeomPath.createChildPath("Geom Feature Data");
  const DataPath copiedValuesPath = copiedAmPath.createChildPath("Geom Values");
  const DataPath renamedAmPath = destGeomPath.createChildPath("Geom Out Feature Data");

  SECTION("Declared recursive paths keep child names")
  {
    const CopyDataObjectAction* copyActionPtr = nullptr;
    for(const auto& action : preflightResult.outputActions.value().actions)
    {
      const auto* candidatePtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
      if(candidatePtr != nullptr && candidatePtr->path() == srcGeomPath.createChildPath("Geom Feature Data"))
      {
        copyActionPtr = candidatePtr;
        break;
      }
    }
    REQUIRE(copyActionPtr != nullptr);
    const auto createdPaths = copyActionPtr->getAllCreatedPaths();
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), copiedValuesPath) != createdPaths.end());
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), renamedAmPath.createChildPath("Geom Out Values")) == createdPaths.end());
  }
  SECTION("Copied objects keep child names")
  {
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataAs<AttributeMatrix>(copiedAmPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Int32Array>(copiedValuesPath) != nullptr);
    REQUIRE_FALSE(dataStructure.containsData(renamedAmPath));
    REQUIRE(dataStructure.getDataAs<Int32Array>(destGeomPath.createChildPath("Cell Data").createChildPath("Data")) != nullptr);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: PhysicalCrop_MaxAtBound", "[SimplnxCore][CropImageGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath sourcePath({"Source"});
  const DataPath croppedPath({"Cropped"});
  CreateImageGeometryAction geometryAction(sourcePath, {5, 4, 3}, {0.5F, -1.0F, 2.0F}, {1.0F, 2.0F, 0.5F}, "Cell Data");
  auto geometryResult = geometryAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(geometryResult);
  CropImageGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, sourcePath);
  args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, croppedPath);
  args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, false);
  args.insertOrAssign(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, false);
  args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, false);
  args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, true);
  args.insertOrAssign(CropImageGeometryFilter::k_MinCoord_Key, std::vector<float64>{1.5, 1.0, 2.5});
  args.insertOrAssign(CropImageGeometryFilter::k_MaxCoord_Key, std::vector<float64>{5.5, 7.0, 3.5});
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(croppedPath));
  const auto& croppedGeom = dataStructure.getDataRefAs<ImageGeom>(croppedPath);
  REQUIRE(croppedGeom.getDimensions() == SizeVec3{4, 3, 2});
  REQUIRE(croppedGeom.getOrigin() == FloatVec3{1.5F, 1.0F, 2.5F});
  REQUIRE(croppedGeom.getSpacing() == FloatVec3{1.0F, 2.0F, 0.5F});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometry: RenumberFeatures negative destination id names the destination array", "[SimplnxCore][CropImageGeometryFilter][RenumberFeatures]")
{
  // Guards destination-specific diagnostics for negative Feature Ids.
  DataStructure dataStructure;
  auto* destPtr = DataGroup::Create(dataStructure, "Dest");
  REQUIRE(destPtr != nullptr);
  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{3}, destPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{4}, destPtr->getId());
  REQUIRE(vertexDataPtr != nullptr);
  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", ShapeType{4}, ShapeType{1}, vertexDataPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  (*featureIdsPtr)[0] = 0;
  (*featureIdsPtr)[1] = 1;
  (*featureIdsPtr)[2] = -1;
  (*featureIdsPtr)[3] = 2;

  const DataPath destFeatureAttributeMatrixPath({"Dest", "Feature Data"});
  const DataPath featureIdsArrayPath({"Source", "Vertex Data", "FeatureIds"});
  const DataPath destFeatureIdsPath({"Dest", "Vertex Data", "FeatureIds"});
  IFilter::MessageHandler messageHandler{};
  const std::atomic_bool shouldCancel = false;

  const auto result = Sampling::RenumberFeatures(dataStructure, destFeatureAttributeMatrixPath, featureIdsArrayPath, destFeatureIdsPath, messageHandler, shouldCancel);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == -605);
  CHECK(result.errors().front().message.find(destFeatureIdsPath.toString()) != std::string::npos);
  CHECK(result.errors().front().message.find("resampled") == std::string::npos);
}

TEST_CASE("SimplnxCore::CropImageGeometry: RenumberFeatures scans destination Feature Ids in batches", "[SimplnxCore][CropImageGeometryFilter][RenumberFeatures]")
{
  constexpr usize k_BatchSize = 40000;
  constexpr usize k_NegativeIdIndex = k_BatchSize + 2;
  constexpr usize k_TotalIds = k_NegativeIdIndex + 1;
  DataStructure dataStructure;
  const DataPath featureAmPath({"Feature Data"});
  const DataPath featureIdsPath({"FeatureIds"});
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, featureAmPath.getTargetName(), ShapeType{2});
  REQUIRE(featureAmPtr != nullptr);
  auto store = std::make_shared<CropProbeStore<int32>>(ShapeType{k_TotalIds}, ShapeType{1}, false, 1);
  auto* featureIdsPtr = Int32Array::Create(dataStructure, featureIdsPath.getTargetName(), store);
  REQUIRE(featureIdsPtr != nullptr);

  SECTION("Preserves the first bulk-read diagnostic")
  {
    store->fail_read = true;
    const auto result = Sampling::RenumberFeatures(dataStructure, featureAmPath, featureIdsPath, featureIdsPath, {});
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -95100);
    CHECK(result.errors().front().message == "Injected crop source read failure.");
    CHECK(store->read_calls == 1);
    CHECK(store->write_calls == 0);
  }

  SECTION("Reports the global index in the second batch")
  {
    (*featureIdsPtr)[k_NegativeIdIndex] = -7;
    const auto result = Sampling::RenumberFeatures(dataStructure, featureAmPath, featureIdsPath, featureIdsPath, {});
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -605);
    CHECK(result.errors().front().message == "Feature Ids values must be zero or greater. Destination Feature Ids array 'FeatureIds' has the negative value -7 at index 40002.");
    CHECK(store->read_bytes == std::vector<usize>{k_BatchSize * sizeof(int32), (k_TotalIds - k_BatchSize) * sizeof(int32)});
    CHECK(store->write_calls == 0);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometry: RenumberFeatures missing destination Feature Ids throws", "[SimplnxCore][CropImageGeometryFilter][RenumberFeatures]")
{
  // Guards exception handling for a missing destination Feature Ids array.
  DataStructure dataStructure;
  auto* destPtr = DataGroup::Create(dataStructure, "Dest");
  REQUIRE(destPtr != nullptr);
  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{3}, destPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{4}, destPtr->getId());
  REQUIRE(vertexDataPtr != nullptr);
  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", ShapeType{4}, ShapeType{1}, vertexDataPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  (*featureIdsPtr)[0] = 0;
  (*featureIdsPtr)[1] = 1;
  (*featureIdsPtr)[2] = -1;
  (*featureIdsPtr)[3] = 2;

  const DataPath destFeatureAttributeMatrixPath({"Dest", "Feature Data"});
  const DataPath featureIdsArrayPath({"Source", "Vertex Data", "FeatureIds"});
  const DataPath destFeatureIdsPath({"Dest", "Vertex Data", "Missing"});
  IFilter::MessageHandler messageHandler{};
  const std::atomic_bool shouldCancel = false;

  REQUIRE_THROWS_AS(Sampling::RenumberFeatures(dataStructure, destFeatureAttributeMatrixPath, featureIdsArrayPath, destFeatureIdsPath, messageHandler, shouldCancel), std::out_of_range);
}

TEST_CASE("SimplnxCore::CropImageGeometry: RenumberFeatures wrong-type destination Feature Ids throws", "[SimplnxCore][CropImageGeometryFilter][RenumberFeatures]")
{
  // Guards exception handling for a destination Feature Ids array with the wrong type.
  DataStructure dataStructure;
  auto* destPtr = DataGroup::Create(dataStructure, "Dest");
  REQUIRE(destPtr != nullptr);
  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{3}, destPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{4}, destPtr->getId());
  REQUIRE(vertexDataPtr != nullptr);
  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", ShapeType{4}, ShapeType{1}, vertexDataPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  (*featureIdsPtr)[0] = 0;
  (*featureIdsPtr)[1] = 1;
  (*featureIdsPtr)[2] = -1;
  (*featureIdsPtr)[3] = 2;

  const DataPath destFeatureAttributeMatrixPath({"Dest", "Feature Data"});
  const DataPath featureIdsArrayPath({"Source", "Vertex Data", "FeatureIds"});
  auto* floatIdsPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "FloatIds", ShapeType{4}, ShapeType{1}, vertexDataPtr->getId());
  REQUIRE(floatIdsPtr != nullptr);

  const DataPath destFeatureIdsPath({"Dest", "Vertex Data", "FloatIds"});
  IFilter::MessageHandler messageHandler{};
  const std::atomic_bool shouldCancel = false;

  REQUIRE_THROWS_AS(Sampling::RenumberFeatures(dataStructure, destFeatureAttributeMatrixPath, featureIdsArrayPath, destFeatureIdsPath, messageHandler, shouldCancel), std::bad_cast);
}

TEST_CASE("SimplnxCore::CropImageGeometry: RenumberFeatures without a geometry removes unused features", "[SimplnxCore][CropImageGeometryFilter][RenumberFeatures]")
{
  // The geometry-independent overload must renumber Feature Ids and compact the Feature AM
  // with no geometry present. Feature 1 is not used, so it is removed and features 2 and 3 move down.
  DataStructure dataStructure;
  auto* destPtr = DataGroup::Create(dataStructure, "Dest");
  REQUIRE(destPtr != nullptr);
  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{4}, destPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  auto* featureValuesPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Values", ShapeType{4}, ShapeType{1}, featureDataPtr->getId());
  REQUIRE(featureValuesPtr != nullptr);
  const std::vector<int32> inputValues = {10, 11, 12, 13};
  for(usize i = 0; i < inputValues.size(); ++i)
  {
    (*featureValuesPtr)[i] = inputValues[i];
  }
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{5}, destPtr->getId());
  REQUIRE(vertexDataPtr != nullptr);
  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", ShapeType{5}, ShapeType{1}, vertexDataPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  const std::vector<int32> inputIds = {0, 2, 2, 0, 3};
  for(usize i = 0; i < inputIds.size(); ++i)
  {
    (*featureIdsPtr)[i] = inputIds[i];
  }

  const DataPath destFeatureAttributeMatrixPath({"Dest", "Feature Data"});
  const DataPath featureIdsArrayPath({"Source", "Vertex Data", "FeatureIds"});
  const DataPath destFeatureIdsPath({"Dest", "Vertex Data", "FeatureIds"});
  IFilter::MessageHandler messageHandler{};
  const std::atomic_bool shouldCancel = false;

  const auto result = Sampling::RenumberFeatures(dataStructure, destFeatureAttributeMatrixPath, featureIdsArrayPath, destFeatureIdsPath, messageHandler, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const std::vector<int32> expectedIds = {0, 1, 1, 0, 2};
  const auto& resultIds = dataStructure.getDataRefAs<Int32Array>(destFeatureIdsPath);
  REQUIRE(resultIds.getNumberOfTuples() == expectedIds.size());
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(resultIds[i] == expectedIds[i]);
  }

  const std::vector<int32> expectedValues = {10, 12, 13};
  REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(destFeatureAttributeMatrixPath).getNumberOfTuples() == expectedValues.size());
  const auto& resultValues = dataStructure.getDataRefAs<Int32Array>(destFeatureAttributeMatrixPath.createChildPath("Values"));
  REQUIRE(resultValues.getNumberOfTuples() == expectedValues.size());
  for(usize i = 0; i < expectedValues.size(); ++i)
  {
    CHECK(resultValues[i] == expectedValues[i]);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometry: RenumberFeatures image cell count mismatch returns -606", "[SimplnxCore][CropImageGeometryFilter][RenumberFeatures]")
{
  // The ImageGeom overload must reject a Feature Ids array whose tuple count differs from the
  // image cell count, and it must not change the Feature Ids or the Feature AM.
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 1});
  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{3}, imageGeomPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  auto* otherDataPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{3}, imageGeomPtr->getId());
  REQUIRE(otherDataPtr != nullptr);
  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", ShapeType{3}, ShapeType{1}, otherDataPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  const std::vector<int32> inputIds = {0, 2, 2};
  for(usize i = 0; i < inputIds.size(); ++i)
  {
    (*featureIdsPtr)[i] = inputIds[i];
  }

  const DataPath imagePath({"Image"});
  const DataPath destFeatureAttributeMatrixPath({"Image", "Feature Data"});
  const DataPath featureIdsArrayPath({"Source", "Other Data", "FeatureIds"});
  const DataPath destFeatureIdsPath({"Image", "Other Data", "FeatureIds"});
  IFilter::MessageHandler messageHandler{};
  const std::atomic_bool shouldCancel = false;

  const auto result = Sampling::RenumberFeatures(dataStructure, imagePath, destFeatureAttributeMatrixPath, featureIdsArrayPath, destFeatureIdsPath, messageHandler, shouldCancel);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == -606);
  CHECK(result.errors().front().message.find(destFeatureIdsPath.toString()) != std::string::npos);

  // Feature 1 is unused, so a renumber would have changed both of these.
  CHECK(dataStructure.getDataRefAs<AttributeMatrix>(destFeatureAttributeMatrixPath).getNumberOfTuples() == 3);
  const auto& resultIds = dataStructure.getDataRefAs<Int32Array>(destFeatureIdsPath);
  for(usize i = 0; i < inputIds.size(); ++i)
  {
    CHECK(resultIds[i] == inputIds[i]);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Feature AM nested alias regression", "[SimplnxCore][CropImageGeometryFilter][NestedFeatureAlias]")
{
  UnitTest::LoadPlugins();
  const bool clear = GENERATE(true, false);
  const bool renumber = GENERATE(false, true);
  auto ds = CreateFeatureCropFixture();
  const DataPath sourcePath({"Image"});
  const DataPath outputPath({"Cropped"});
  const auto featurePath = sourcePath.createChildPath("Cell Feature Data");
  const auto* featuresPtr = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(featuresPtr != nullptr);
  const auto featureId = featuresPtr->getId();
  const auto shape = featuresPtr->getShape();
  const auto countsPath = featurePath.createChildPath("NumElements");
  const auto& counts = ds.getDataRefAs<Int32Array>(countsPath);
  std::vector<int32> originalCounts(counts.getSize());
  for(usize i = 0; i < originalCounts.size(); ++i)
  {
    originalCounts[i] = counts[i];
  }
  auto* nestedPtr = DataGroup::Create(ds, "Misc", ds.getData(sourcePath)->getId());
  REQUIRE(nestedPtr != nullptr);
  auto* keepPtr = UnitTest::CreateTestDataArray<int32>(ds, "Keep", {1}, {1}, nestedPtr->getId());
  REQUIRE(keepPtr != nullptr);
  (*keepPtr)[0] = 123;
  REQUIRE(ds.setAdditionalParent(featureId, nestedPtr->getId()));
  auto* outsidePtr = DataGroup::Create(ds, "Outside");
  REQUIRE(outsidePtr != nullptr);
  REQUIRE(ds.setAdditionalParent(nestedPtr->getId(), outsidePtr->getId()));
  const auto parents = ds.getData(featurePath)->getParentIds();
  auto args = MakeFeatureCropArgs(clear, renumber);
  CropImageGeometryFilter filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  bool foundNestedCopy = false;
  const auto nestedOutputPath = outputPath.createChildPath("Misc").createChildPath("Cell Feature Data");
  for(const auto& action : preflight.outputActions.value().actions)
  {
    const auto* copyPtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
    if(copyPtr != nullptr && copyPtr->path() == sourcePath.createChildPath("Misc"))
    {
      foundNestedCopy = true;
      bool declaresFeature = false;
      for(const auto& created : copyPtr->getAllCreatedPaths())
      {
        if(created.rebase(nestedOutputPath, DataPath{}).has_value())
        {
          declaresFeature = true;
        }
      }
      CHECK(declaresFeature == !clear);
    }
  }
  REQUIRE(foundNestedCopy);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* outputFeaturesPtr = ds.getDataAs<AttributeMatrix>(outputPath.createChildPath("Cell Feature Data"));
  REQUIRE(outputFeaturesPtr != nullptr);
  CHECK(outputFeaturesPtr->getSize() == (clear ? 0 : 3));
  CHECK((ds.getData(nestedOutputPath) == nullptr) == clear);
  CHECK(ds.getDataRefAs<Int32Array>(outputPath.createChildPath("Misc").createChildPath("Keep"))[0] == 123);
  CHECK(ds.getData(featurePath)->getId() == featureId);
  CHECK(ds.getData(featurePath)->getParentIds() == parents);
  CHECK(ds.getDataRefAs<AttributeMatrix>(featurePath).getShape() == shape);
  CHECK(ds.getDataRefAs<AttributeMatrix>(featurePath).getSize() == 3);
  CHECK(ds.getData(DataPath({"Outside", "Misc", "Cell Feature Data"}))->getId() == featureId);
  for(usize i = 0; i < originalCounts.size(); ++i)
  {
    CHECK(ds.getDataRefAs<Int32Array>(countsPath)[i] == originalCounts[i]);
    if(!clear)
    {
      CHECK(ds.getDataRefAs<Int32Array>(nestedOutputPath.createChildPath("NumElements"))[i] == originalCounts[i]);
    }
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Feature AM selected through second parent", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateFeatureCropFixture();
  const bool renumber = GENERATE(false, true);
  auto* parentPtr = DataGroup::Create(dataStructure, "Alias");
  REQUIRE(parentPtr != nullptr);
  const auto& source = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image", "Cell Feature Data"}));
  REQUIRE(dataStructure.setAdditionalParent(source.getId(), parentPtr->getId()));
  auto args = MakeFeatureCropArgs(true, renumber);
  args.insertOrAssign(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Alias", "Cell Feature Data"})));
  CropImageGeometryFilter filter;
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto& output = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Cell Feature Data"}));
  CHECK(output.getSize() == 0);
  CHECK(output.getShape() == ShapeType{renumber ? usize{3} : usize{5}});
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(DataPath({"Cropped", "Cell Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 8);
  for(usize i = 0; i < 8; ++i)
  {
    CHECK(ids[i] == (renumber ? (i % 2 == 0 ? 1 : 2) : (i % 2 == 0 ? 3 : 4)));
  }
  CHECK(source.getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: SIMPL conversion Feature options", "[SimplnxCore][CropImageGeometryFilter][SIMPL]")
{
  const fs::path fixturePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test/simpl_conversion/6_5/CropImageGeometryFilter.json";
  std::ifstream stream(fixturePath);
  REQUIRE(stream.is_open());
  auto json = nlohmann::json::parse(stream).at("0");
  json["CellFeatureAttributeMatrixPath"] = {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Feature Data"}, {"Data Array Name", ""}};
  const bool renumber = GENERATE(true, false);
  json["RenumberFeatures"] = renumber ? 1 : 0;
  auto converted = CropImageGeometryFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(converted);
  CHECK(converted.value().value<bool>(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key) == renumber);
  CHECK(converted.value().value<bool>(CropImageGeometryFilter::k_RenumberFeatures_Key) == renumber);
  CHECK(converted.value().value<DataPath>(CropImageGeometryFilter::k_FeatureAttributeMatrixPath_Key) == DataPath({"Image", "Cell Feature Data"}));

  SECTION("Missing RenumberFeatures follows the standard conversion error")
  {
    json.erase("RenumberFeatures");
    auto missing = CropImageGeometryFilter::FromSIMPLJson(json);
    SIMPLNX_RESULT_REQUIRE_INVALID(missing);
    CHECK(missing.errors().front().code == -571);
  }
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: SIMPL conversion uses voxel bounds", "[SimplnxCore][CropImageGeometryFilter][SIMPL]")
{
  const nlohmann::json json = {
      {"Filter_Name", "CropImageGeometry"},
      {"Filter_Uuid", "{baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf}"},
      {"XMin", 2U},
      {"YMin", 1U},
      {"ZMin", 0U},
      {"XMax", 3U},
      {"YMax", 2U},
      {"ZMax", 0U},
      {"SaveAsNewDataContainer", 1U},
      {"NewDataContainerName", "Cropped"},
      {"RenumberFeatures", 1U},
      {"CellAttributeMatrixPath", {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Data"}, {"Data Array Name", ""}}},
      {"FeatureIdsArrayPath", {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Data"}, {"Data Array Name", "FeatureIds"}}},
      {"CellFeatureAttributeMatrixPath", {{"Data Container Name", "Image"}, {"Attribute Matrix Name", "Cell Feature Data"}, {"Data Array Name", ""}}},
  };
  auto converted = CropImageGeometryFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(converted);
  const auto& args = converted.value();
  CHECK(args.value<bool>(CropImageGeometryFilter::k_UsePhysicalBounds_Key) == false);
  CHECK(args.value<std::vector<uint64>>(CropImageGeometryFilter::k_MinVoxel_Key) == std::vector<uint64>{2, 1, 0});
  CHECK(args.value<std::vector<uint64>>(CropImageGeometryFilter::k_MaxVoxel_Key) == std::vector<uint64>{3, 2, 0});

  UnitTest::LoadPlugins();
  auto ds = CreateFeatureCropFixture();
  auto result = CropImageGeometryFilter().execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* croppedPtr = ds.getDataAs<ImageGeom>(DataPath({"Cropped"}));
  REQUIRE(croppedPtr != nullptr);
  CHECK(croppedPtr->getDimensions() == SizeVec3(2, 2, 1));
}

TEST_CASE("SimplnxCore::CropImageGeometryFilter: Reject another geometry element Feature AM", "[SimplnxCore][CropImageGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureCropFixture();
  const auto featurePath = DataPath({"Image", "Cell Feature Data"});
  const auto* featuresPtr = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(featuresPtr != nullptr);
  auto* otherPtr = VertexGeom::Create(ds, "OtherGeometry");
  REQUIRE(otherPtr != nullptr);
  auto* verticesPtr = UnitTest::CreateTestDataArray<float32>(ds, "Vertices", {featuresPtr->getNumberOfTuples()}, {3}, otherPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  otherPtr->setVertices(*verticesPtr);
  REQUIRE(ds.setAdditionalParent(featuresPtr->getId(), otherPtr->getId()));
  otherPtr->setVertexAttributeMatrix(*featuresPtr);
  CropImageGeometryFilter filter;
  auto result = filter.preflight(ds, MakeFeatureCropArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -67460);
  CHECK(result.outputActions.errors()[0].message.find(featurePath.toString()) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find("OtherGeometry") != std::string::npos);
  CHECK(otherPtr->getVertexAttributeMatrix() == featuresPtr);
  CHECK(featuresPtr->getSize() > 0);
}
