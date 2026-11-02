#include "simplnx/Utilities/ImageProcessing/AxisProjectionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ProjectionReducers.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;
namespace projection_detail = nx::core::ImageProcessing::detail;

namespace
{
// Deliberately NON-CUBIC so a transposed/swapped-axis bug cannot pass: X even, Y odd, Z even.
constexpr usize k_X = 4;
constexpr usize k_Y = 5;
constexpr usize k_Z = 6;
constexpr usize k_Volume = k_X * k_Y * k_Z; // 120

// simplnx flat index for input voxel (x,y,z): X fastest-moving.
usize FlatIndex(usize x, usize y, usize z)
{
  return z * (k_Y * k_X) + y * k_X + x;
}

// Independent (div/mod-based) reference for the output-slot mapping the engine must reproduce.
// projDim collapses that spatial axis to size 1; the slot is the flat index in the collapsed dims.
usize OutputSlot(usize x, usize y, usize z, usize projDim)
{
  const usize ox = (projDim == 0) ? 0 : x;
  const usize oy = (projDim == 1) ? 0 : y;
  const usize oz = (projDim == 2) ? 0 : z;
  const usize odx = (projDim == 0) ? 1 : k_X;
  const usize ody = (projDim == 1) ? 1 : k_Y;
  return oz * (ody * odx) + oy * odx + ox;
}

usize NumSlots(usize projDim)
{
  const usize collapsed = (projDim == 0) ? k_X : (projDim == 1) ? k_Y : k_Z;
  return k_Volume / collapsed;
}

template <class T>
DataStore<T> MakeInputStore(const std::vector<T>& values)
{
  // tuple shape is slowest->fastest == {Z, Y, X}
  DataStore<T> store(ShapeType{k_Z, k_Y, k_X}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < values.size(); ++i)
  {
    store.setValue(i, values[i]);
  }
  return store;
}

// Gather each output slot's "pencil" of input values. Order within a pencil is irrelevant because
// max / mean / median are all order-independent, so this is a clean independent oracle.
template <class T>
std::vector<std::vector<T>> GatherPencils(const std::vector<T>& in, usize projDim)
{
  std::vector<std::vector<T>> pencils(NumSlots(projDim));
  for(usize z = 0; z < k_Z; ++z)
  {
    for(usize y = 0; y < k_Y; ++y)
    {
      for(usize x = 0; x < k_X; ++x)
      {
        pencils[OutputSlot(x, y, z, projDim)].push_back(in[FlatIndex(x, y, z)]);
      }
    }
  }
  return pencils;
}

template <class TIn, class TOut, class ReduceFn>
std::vector<TOut> RunProjection(const std::vector<TIn>& input, usize projDim, ReduceFn reduce)
{
  DataStore<TIn> inStore = MakeInputStore(input);
  const usize slots = NumSlots(projDim);
  DataStore<TOut> outStore(ShapeType{slots}, ShapeType{1}, static_cast<TOut>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  Result<> result = ApplyAxisProjection<TIn, TOut>(inStore, outStore, SizeVec3{k_X, k_Y, k_Z}, projDim, reduce, shouldCancel, messageHandler);
  REQUIRE(result.valid());

  std::vector<TOut> out(slots);
  for(usize i = 0; i < slots; ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

template <class T>
class ProjectionTransferStore : public DataStore<T>
{
public:
  ProjectionTransferStore(const ShapeType& tupleShape, T initialValue, bool outOfCore = true)
  : DataStore<T>(tupleShape, ShapeType{1}, initialValue)
  , m_OutOfCore(outOfCore)
  {
  }

  IDataStore::StoreType getStoreType() const override
  {
    return m_OutOfCore ? IDataStore::StoreType::OutOfCore : IDataStore::StoreType::InMemory;
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    ++m_FlatReadCount;
    m_MaxReadValues = std::max(m_MaxReadValues, buffer.size());
    if(m_FailReadCall != 0 && m_ReadCount == m_FailReadCall)
    {
      return MakeErrorResult(m_ReadErrorCode, "Injected axis-projection input read failure");
    }
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(result.valid() && m_CancelAfterReadCall != 0 && m_ReadCount == m_CancelAfterReadCall && m_ShouldCancel != nullptr)
    {
      *m_ShouldCancel = true;
    }
    return result;
  }

  Result<std::vector<T>> readExtent(const Extent& extent) const override
  {
    ++m_ReadCount;
    ++m_ExtentReadCount;
    if(m_FailReadCall != 0 && m_ReadCount == m_FailReadCall)
    {
      return MakeErrorResult<std::vector<T>>(m_ReadErrorCode, "Injected axis-projection extent read failure");
    }
    Result<std::vector<T>> readResult = DataStore<T>::readExtent(extent);
    if(readResult.invalid())
    {
      return readResult;
    }
    m_MaxReadValues = std::max(m_MaxReadValues, readResult.value().size());
    if(m_CancelAfterReadCall != 0 && m_ReadCount == m_CancelAfterReadCall && m_ShouldCancel != nullptr)
    {
      *m_ShouldCancel = true;
    }
    return readResult;
  }

  Result<std::vector<std::vector<T>>> readExtents(nonstd::span<const Extent> extents) const override
  {
    ++m_ReadCount;
    ++m_ExtentFamilyReadCount;
    if(m_FailReadCall != 0 && m_ReadCount == m_FailReadCall)
    {
      return MakeErrorResult<std::vector<std::vector<T>>>(m_ReadErrorCode, "Injected axis-projection extent read failure");
    }
    Result<std::vector<std::vector<T>>> readResult = DataStore<T>::readExtents(extents);
    if(readResult.invalid())
    {
      return readResult;
    }
    for(const auto& extentValues : readResult.value())
    {
      m_MaxReadValues = std::max(m_MaxReadValues, extentValues.size());
    }
    if(m_CancelAfterReadCall != 0 && m_ReadCount == m_CancelAfterReadCall && m_ShouldCancel != nullptr)
    {
      *m_ShouldCancel = true;
    }
    return readResult;
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++m_WriteCount;
    m_MaxWriteValues = std::max(m_MaxWriteValues, buffer.size());
    if(m_FailWriteCall != 0 && m_WriteCount == m_FailWriteCall)
    {
      return MakeErrorResult(m_WriteErrorCode, "Injected axis-projection output write failure");
    }
    Result<> result = DataStore<T>::copyFromBuffer(startIndex, buffer);
    if(result.valid())
    {
      m_WrittenValues += buffer.size();
    }
    return result;
  }

  void resetTransferCounts()
  {
    m_ReadCount = 0;
    m_FlatReadCount = 0;
    m_ExtentReadCount = 0;
    m_ExtentFamilyReadCount = 0;
    m_WriteCount = 0;
    m_MaxReadValues = 0;
    m_MaxWriteValues = 0;
    m_WrittenValues = 0;
  }

  void cancelAfterRead(usize readCall, std::atomic_bool& shouldCancel)
  {
    m_CancelAfterReadCall = readCall;
    m_ShouldCancel = &shouldCancel;
  }

  void failRead(usize readCall, int32 errorCode)
  {
    m_FailReadCall = readCall;
    m_ReadErrorCode = errorCode;
  }

  void failWrite(usize writeCall, int32 errorCode)
  {
    m_FailWriteCall = writeCall;
    m_WriteErrorCode = errorCode;
  }

  usize maxReadValues() const noexcept
  {
    return m_MaxReadValues;
  }

  usize maxWriteValues() const noexcept
  {
    return m_MaxWriteValues;
  }

  usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

  usize flatReadCount() const noexcept
  {
    return m_FlatReadCount;
  }

  usize extentReadCount() const noexcept
  {
    return m_ExtentReadCount;
  }

  usize extentFamilyReadCount() const noexcept
  {
    return m_ExtentFamilyReadCount;
  }

private:
  bool m_OutOfCore = true;
  mutable usize m_ReadCount = 0;
  mutable usize m_FlatReadCount = 0;
  mutable usize m_ExtentReadCount = 0;
  mutable usize m_ExtentFamilyReadCount = 0;
  usize m_WriteCount = 0;
  mutable usize m_MaxReadValues = 0;
  usize m_MaxWriteValues = 0;
  usize m_WrittenValues = 0;
  usize m_CancelAfterReadCall = 0;
  std::atomic_bool* m_ShouldCancel = nullptr;
  usize m_FailReadCall = 0;
  usize m_FailWriteCall = 0;
  int32 m_ReadErrorCode = -9901;
  int32 m_WriteErrorCode = -9902;
};

struct FakeExternalSortControl
{
  bool failCreate = false;
  bool failAppend = false;
  bool failFinish = false;
  bool failRead = false;
  bool shortRead = false;
  bool failTransposeCreate = false;
  bool failTransposeRead = false;
  bool failTransposeWrite = false;
  bool wrongRecordCount = false;
  usize createCount = 0;
  usize maxAppendRecords = 0;
};

class FakeExternalSort : public IExternalSort
{
public:
  FakeExternalSort(ExternalSortConfig config, std::shared_ptr<FakeExternalSortControl> control)
  : m_Config(std::move(config))
  , m_Control(std::move(control))
  {
  }

  Result<> append(uint64 recordCount, nonstd::span<const std::byte> records, const std::atomic_bool& shouldCancel, const ExternalSortProgressCallback& progressCallback) override
  {
    if(shouldCancel)
    {
      return MakeErrorResult(-9910, "Fake external sort append cancelled");
    }
    if(m_Control->failAppend)
    {
      return MakeErrorResult(-9911, "Injected external sort append failure");
    }
    if(recordCount > m_Config.maxRecordsPerBatch || records.size() != recordCount * m_Config.recordSize)
    {
      return MakeErrorResult(-9912, "Fake external sort received an invalid append batch");
    }
    m_Control->maxAppendRecords = std::max(m_Control->maxAppendRecords, static_cast<usize>(recordCount));
    for(uint64 recordIndex = 0; recordIndex < recordCount; ++recordIndex)
    {
      const std::byte* recordStart = records.data() + recordIndex * m_Config.recordSize;
      m_Records.emplace_back(recordStart, recordStart + m_Config.recordSize);
    }
    if(progressCallback)
    {
      progressCallback(recordCount, recordCount);
    }
    return {};
  }

  Result<> finish(const std::atomic_bool& shouldCancel, const ExternalSortProgressCallback& progressCallback) override
  {
    if(shouldCancel)
    {
      return MakeErrorResult(-9913, "Fake external sort finish cancelled");
    }
    if(m_Control->failFinish)
    {
      return MakeErrorResult(-9914, "Injected external sort finish failure");
    }
    std::stable_sort(m_Records.begin(), m_Records.end(), [&](const std::vector<std::byte>& left, const std::vector<std::byte>& right) {
      return m_Config.compare(nonstd::span<const std::byte>(left.data(), left.size()), nonstd::span<const std::byte>(right.data(), right.size())) < 0;
    });
    m_Finished = true;
    if(progressCallback)
    {
      progressCallback(m_Records.size(), m_Records.size());
    }
    return {};
  }

  Result<uint64> read(uint64 recordOffset, uint64 recordCount, nonstd::span<std::byte> records, const std::atomic_bool& shouldCancel) const override
  {
    if(shouldCancel)
    {
      return MakeErrorResult<uint64>(-9915, "Fake external sort read cancelled");
    }
    if(m_Control->failRead)
    {
      return MakeErrorResult<uint64>(-9916, "Injected external sort read failure");
    }
    if(!m_Finished || recordOffset >= m_Records.size() || recordCount > m_Config.maxRecordsPerBatch || records.size() != recordCount * m_Config.recordSize)
    {
      return MakeErrorResult<uint64>(-9917, "Fake external sort received an invalid read");
    }
    if(m_Control->shortRead)
    {
      return {0};
    }
    const uint64 available = std::min<uint64>(recordCount, m_Records.size() - recordOffset);
    for(uint64 recordIndex = 0; recordIndex < available; ++recordIndex)
    {
      std::memcpy(records.data() + recordIndex * m_Config.recordSize, m_Records[recordOffset + recordIndex].data(), m_Config.recordSize);
    }
    return {available};
  }

  uint64 recordCount() const override
  {
    return m_Records.size() + (m_Control->wrongRecordCount ? 1 : 0);
  }

private:
  ExternalSortConfig m_Config;
  std::shared_ptr<FakeExternalSortControl> m_Control;
  std::vector<std::vector<std::byte>> m_Records;
  bool m_Finished = false;
};

projection_detail::AxisProjection2DServices<int32> MakeFakeProjectionServices(const std::shared_ptr<FakeExternalSortControl>& control, std::shared_ptr<ProjectionTransferStore<int32>>& transposeStore)
{
  projection_detail::AxisProjection2DServices<int32> services;
  services.createExternalSort = [control](const ExternalSortConfig& config) -> Result<std::unique_ptr<IExternalSort>> {
    if(control->failCreate)
    {
      return MakeErrorResult<std::unique_ptr<IExternalSort>>(-9920, "Injected external sort creation failure");
    }
    ++control->createCount;
    std::unique_ptr<IExternalSort> sorter = std::make_unique<FakeExternalSort>(config, control);
    return {std::move(sorter)};
  };
  services.createTransposeStore = [control, &transposeStore](usize valueCount, usize) -> Result<std::unique_ptr<projection_detail::AxisProjectionWorkStore<int32>>> {
    if(control->failTransposeCreate)
    {
      return MakeErrorResult<std::unique_ptr<projection_detail::AxisProjectionWorkStore<int32>>>(-9921, "Injected transpose-store creation failure");
    }
    transposeStore = std::make_shared<ProjectionTransferStore<int32>>(ShapeType{valueCount}, int32{}, false);
    if(control->failTransposeRead)
    {
      transposeStore->failRead(1, -9922);
    }
    if(control->failTransposeWrite)
    {
      transposeStore->failWrite(1, -9923);
    }
    std::shared_ptr<AbstractDataStore<int32>> abstractStore = transposeStore;
    return {std::make_unique<projection_detail::AxisProjectionWorkStore<int32>>(std::move(abstractStore))};
  };
  return services;
}

template <class TIn, class TOut, class ReduceFn>
void RequireBoundedAssociativeMatchesResident(const SizeVec3& dims, usize projDim, ReduceFn reduce, usize targetBytes, projection_detail::AxisProjection2DRoute expectedRoute)
{
  const usize volume = dims[0] * dims[1] * dims[2];
  const usize outputValues = volume / dims[projDim];
  const ShapeType inputShape = {dims[2], dims[1], dims[0]};
  const ShapeType outputShape = {outputValues};

  ProjectionTransferStore<TIn> inputStore(inputShape, TIn{});
  DataStore<TIn> residentInput(inputShape, ShapeType{1}, TIn{});
  for(usize index = 0; index < volume; ++index)
  {
    const TIn value = static_cast<TIn>(static_cast<int32>((index * 7) % 23) - 11);
    inputStore.setValue(index, value);
    residentInput.setValue(index, value);
  }
  inputStore.resetTransferCounts();

  constexpr TOut k_Poison = static_cast<TOut>(-12345);
  ProjectionTransferStore<TOut> boundedOutput(outputShape, k_Poison);
  DataStore<TOut> residentOutput(outputShape, ShapeType{1}, k_Poison);
  const auto planResult = projection_detail::CreateAxisProjection2DPlan<TIn, TOut, ReduceFn>(dims, projDim, targetBytes);
  REQUIRE(planResult.valid());
  const auto& plan = planResult.value();
  REQUIRE(plan.route == expectedRoute);

  const auto residentShape = projection_detail::ValidateAxisProjection(residentInput, residentOutput, dims, projDim);
  REQUIRE(residentShape.valid());
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> residentResult = projection_detail::ApplyAxisProjectionResident(residentInput, residentOutput, residentShape.value(), projDim, reduce, shouldCancel, messageHandler);
  REQUIRE(residentResult.valid());

  const auto boundedShape = projection_detail::ValidateAxisProjection(inputStore, boundedOutput, dims, projDim);
  REQUIRE(boundedShape.valid());
  const Result<> boundedResult = projection_detail::ApplyAxisProjection2D(inputStore, boundedOutput, boundedShape.value(), projDim, reduce, shouldCancel, targetBytes);
  REQUIRE(boundedResult.valid());

  std::vector<TOut> actual(outputValues);
  std::vector<TOut> expected(outputValues);
  REQUIRE(boundedOutput.copyIntoBuffer(0, nonstd::span<TOut>(actual.data(), actual.size())).valid());
  REQUIRE(residentOutput.copyIntoBuffer(0, nonstd::span<TOut>(expected.data(), expected.size())).valid());
  REQUIRE(actual == expected);
  REQUIRE(inputStore.maxReadValues() <= plan.inputValues);
  REQUIRE(boundedOutput.maxWriteValues() <= plan.outputValues);
  REQUIRE(boundedOutput.writtenValues() == outputValues);
  REQUIRE(plan.residentBytes <= targetBytes);
}

void RequireBoundedMedianMatchesResident(const SizeVec3& dims, usize projDim, usize targetBytes, projection_detail::AxisProjection2DRoute expectedRoute,
                                         const projection_detail::AxisProjection2DServices<int32>* services = nullptr)
{
  const usize volume = dims[0] * dims[1] * dims[2];
  const usize outputValues = volume / dims[projDim];
  const ShapeType inputShape = {dims[2], dims[1], dims[0]};
  const ShapeType outputShape = {outputValues};

  ProjectionTransferStore<int32> inputStore(inputShape, int32{});
  DataStore<int32> residentInput(inputShape, ShapeType{1}, int32{});
  std::vector<int32> input(volume);
  for(usize index = 0; index < volume; ++index)
  {
    input[index] = static_cast<int32>((index * 11 + 3) % 29);
    inputStore.setValue(index, input[index]);
    residentInput.setValue(index, input[index]);
  }
  inputStore.resetTransferCounts();

  constexpr int32 k_Poison = -12345;
  ProjectionTransferStore<int32> boundedOutput(outputShape, k_Poison);
  DataStore<int32> residentOutput(outputShape, ShapeType{1}, k_Poison);
  const auto planResult = projection_detail::CreateAxisProjection2DPlan<int32, int32, MedianReduce>(dims, projDim, targetBytes);
  REQUIRE(planResult.valid());
  const auto& plan = planResult.value();
  REQUIRE(plan.route == expectedRoute);

  const auto residentShape = projection_detail::ValidateAxisProjection(residentInput, residentOutput, dims, projDim);
  const auto boundedShape = projection_detail::ValidateAxisProjection(inputStore, boundedOutput, dims, projDim);
  REQUIRE(residentShape.valid());
  REQUIRE(boundedShape.valid());
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  REQUIRE(projection_detail::ApplyAxisProjectionResident(residentInput, residentOutput, residentShape.value(), projDim, MedianReduce{}, shouldCancel, messageHandler).valid());
  REQUIRE(projection_detail::ApplyAxisProjection2D(inputStore, boundedOutput, boundedShape.value(), projDim, MedianReduce{}, shouldCancel, targetBytes, services).valid());

  std::vector<std::vector<int32>> pencils(outputValues);
  for(usize z = 0; z < dims[2]; ++z)
  {
    for(usize y = 0; y < dims[1]; ++y)
    {
      for(usize x = 0; x < dims[0]; ++x)
      {
        const usize inputIndex = z * dims[1] * dims[0] + y * dims[0] + x;
        const usize outputSlot = projDim == 0 ? z * dims[1] + y : (projDim == 1 ? z * dims[0] + x : y * dims[0] + x);
        pencils[outputSlot].push_back(input[inputIndex]);
      }
    }
  }
  for(usize slot = 0; slot < outputValues; ++slot)
  {
    std::sort(pencils[slot].begin(), pencils[slot].end());
    const int32 expected = pencils[slot][pencils[slot].size() / 2];
    REQUIRE(boundedOutput.getValue(slot) == expected);
    REQUIRE(boundedOutput.getValue(slot) == residentOutput.getValue(slot));
  }
  REQUIRE(inputStore.maxReadValues() <= plan.inputValues);
  REQUIRE(boundedOutput.maxWriteValues() <= plan.outputValues);
  REQUIRE(boundedOutput.writtenValues() == outputValues);
  REQUIRE(plan.residentBytes <= targetBytes);
  if(expectedRoute == projection_detail::AxisProjection2DRoute::MedianYTiles)
  {
    REQUIRE(plan.stateSlots > 0);
    REQUIRE(plan.stateSlots <= 8);
    REQUIRE(plan.pencilValues == plan.stateSlots * dims[1]);
    REQUIRE(inputStore.flatReadCount() == 0);
    REQUIRE(inputStore.extentReadCount() == (dims[0] + plan.tileColumns - 1) / plan.tileColumns);
    REQUIRE(inputStore.extentFamilyReadCount() == 0);
  }
}

Result<> RunExternalMedianWithServices(usize projDim, const projection_detail::AxisProjection2DServices<int32>* services)
{
  const SizeVec3 dims = projDim == 0 ? SizeVec3{17, 3, 1} : SizeVec3{5, 17, 1};
  const usize volume = dims[0] * dims[1];
  const usize outputValues = volume / dims[projDim];
  ProjectionTransferStore<int32> inputStore(ShapeType{1, dims[1], dims[0]}, int32{});
  ProjectionTransferStore<int32> outputStore(ShapeType{outputValues}, -12345);
  for(usize index = 0; index < volume; ++index)
  {
    inputStore.setValue(index, static_cast<int32>((index * 11 + 3) % 29));
  }
  inputStore.resetTransferCounts();
  auto shapeResult = projection_detail::ValidateAxisProjection(inputStore, outputStore, dims, projDim);
  if(shapeResult.invalid())
  {
    return ConvertResult(std::move(shapeResult));
  }
  std::atomic_bool shouldCancel{false};
  constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 60;
  return projection_detail::ApplyAxisProjection2D(inputStore, outputStore, shapeResult.value(), projDim, MedianReduce{}, shouldCancel, k_Target, services);
}
} // namespace

TEST_CASE("ImageProcessing::AxisProjectionEngine: Max/Mean/Median on non-cubic image, all axes", "[ImageProcessing][AxisProjectionEngine]")
{
  const usize projDim = GENERATE(usize{0}, usize{1}, usize{2});

  // Ramp: value == flat index (exact in int32 and float32 for a 120-voxel image).
  std::vector<int32> ramp(k_Volume);
  for(usize i = 0; i < k_Volume; ++i)
  {
    ramp[i] = static_cast<int32>(i);
  }

  // Non-monotonic pattern so the median is a non-trivial choice among the pencil values.
  std::vector<int32> nonMono(k_Volume);
  for(usize i = 0; i < k_Volume; ++i)
  {
    nonMono[i] = static_cast<int32>((i * 7) % 53);
  }

  DYNAMIC_SECTION("projDim=" << projDim << " Max")
  {
    const auto pencils = GatherPencils(ramp, projDim);
    const auto actual = RunProjection<int32, int32>(ramp, projDim, MaxReduce{});
    REQUIRE(actual.size() == pencils.size());
    for(usize s = 0; s < pencils.size(); ++s)
    {
      const int32 expected = *std::max_element(pencils[s].begin(), pencils[s].end());
      REQUIRE(actual[s] == expected);
    }
  }

  DYNAMIC_SECTION("projDim=" << projDim << " Mean")
  {
    const auto pencils = GatherPencils(ramp, projDim);
    const auto actual = RunProjection<int32, float32>(ramp, projDim, MeanReduce{});
    REQUIRE(actual.size() == pencils.size());
    for(usize s = 0; s < pencils.size(); ++s)
    {
      float64 sum = 0.0;
      for(int32 v : pencils[s])
      {
        sum += static_cast<float64>(v);
      }
      const float64 expected = sum / static_cast<float64>(pencils[s].size());
      REQUIRE(static_cast<float64>(actual[s]) == Approx(expected).margin(1.0e-4));
    }
  }

  DYNAMIC_SECTION("projDim=" << projDim << " Median")
  {
    const auto pencils = GatherPencils(nonMono, projDim);
    const auto actual = RunProjection<int32, int32>(nonMono, projDim, MedianReduce{});
    REQUIRE(actual.size() == pencils.size());
    for(usize s = 0; s < pencils.size(); ++s)
    {
      // Same convention as the Phase-2 MedianImageFilter: nth_element at size()/2 (upper-middle for
      // even counts). X=4 and Z=6 are even extents, so projDim 0 and 2 exercise that tie behavior.
      std::vector<int32> sorted = pencils[s];
      std::sort(sorted.begin(), sorted.end());
      const int32 expected = sorted[sorted.size() / 2];
      REQUIRE(actual[s] == expected);
    }
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: projection preflight defers full-overwrite output initialization", "[ImageProcessing][AxisProjectionEngine]")
{
  const auto buildInput = [](DataStructure& dataStructure) {
    auto* imageGeom = ImageGeom::Create(dataStructure, "Image Geometry");
    REQUIRE(imageGeom != nullptr);
    imageGeom->setDimensions({k_X, k_Y, k_Z});
    auto* cellData = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{k_Z, k_Y, k_X}, imageGeom->getId());
    REQUIRE(cellData != nullptr);
    imageGeom->setCellData(*cellData);
    auto store = std::make_shared<DataStore<uint8>>(ShapeType{k_Z, k_Y, k_X}, ShapeType{1}, uint8{0});
    auto* input = DataArray<uint8>::Create(dataStructure, "Input", store, cellData->getId());
    REQUIRE(input != nullptr);
    return DataPath({"Image Geometry", "CellData", "Input"});
  };

  const auto requireOutputAction = [](const Result<OutputActions>& preflightResult, const DataPath& outputPath) -> const CreateArrayAction& {
    REQUIRE(preflightResult.valid());
    REQUIRE(preflightResult.value().actions.size() == 2);
    const auto* action = dynamic_cast<const CreateArrayAction*>(preflightResult.value().actions[1].get());
    REQUIRE(action != nullptr);
    REQUIRE(action->path() == outputPath);
    REQUIRE(action->initializationMode() == DataStoreInitializationMode::DeferredZeroFill);
    return *action;
  };

  SECTION("SameAsInput creates a new geometry")
  {
    DataStructure dataStructure;
    const DataPath inputPath = buildInput(dataStructure);
    const DataPath outputPath({"Projected Same", "CellData", "Same Output"});
    auto preflightResult = PreflightAxisProjection<ProjectionScalar, SameAsInput>(dataStructure, DataPath({"Image Geometry"}), inputPath, /*projDim=*/0,
                                                                                  /*performInPlace=*/false, "Projected Same", "Same Output");
    requireOutputAction(preflightResult, outputPath);

    const Result<> applyResult = preflightResult.value().applyAll(dataStructure, IDataAction::Mode::Execute);
    REQUIRE(applyResult.valid());
    const auto* output = dataStructure.getDataAs<DataArray<uint8>>(outputPath);
    REQUIRE(output != nullptr);
  }

  SECTION("AlwaysFloat64 creates the temporary in-place geometry")
  {
    DataStructure dataStructure;
    const DataPath inputPath = buildInput(dataStructure);
    const DataPath actionPath({k_ProjectionTempGeomName, "CellData", "Float Output"});
    auto preflightResult = PreflightAxisProjection<ProjectionScalar, AlwaysFloat64>(dataStructure, DataPath({"Image Geometry"}), inputPath, /*projDim=*/1,
                                                                                    /*performInPlace=*/true, "Ignored Projected Geometry", "Float Output");
    requireOutputAction(preflightResult, actionPath);

    const Result<> applyResult = preflightResult.value().applyAll(dataStructure, IDataAction::Mode::Execute);
    REQUIRE(applyResult.valid());
    const auto* output = dataStructure.getDataAs<DataArray<float64>>(DataPath({"Image Geometry", "CellData", "Float Output"}));
    REQUIRE(output != nullptr);
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: InCore direct path avoids bulk input staging", "[ImageProcessing][AxisProjectionEngine]")
{
  const usize projDim = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(projDim);
  const ShapeType inputShape{k_Z, k_Y, k_X};
  std::vector<int32> input(k_Volume);
  for(usize index = 0; index < input.size(); ++index)
  {
    input[index] = static_cast<int32>((index * 7) % 53);
  }

  const auto requireDirect = [&]<class TOut>(auto reduce) {
    ProjectionTransferStore<int32> inputStore(inputShape, int32{0}, false);
    for(usize index = 0; index < input.size(); ++index)
    {
      inputStore.setValue(index, input[index]);
    }
    ProjectionTransferStore<TOut> outputStore(ShapeType{NumSlots(projDim)}, TOut{}, false);
    inputStore.resetTransferCounts();
    outputStore.resetTransferCounts();
    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};

    REQUIRE(ApplyAxisProjection<int32, TOut>(inputStore, outputStore, SizeVec3{k_X, k_Y, k_Z}, projDim, reduce, shouldCancel, messageHandler).valid());
    REQUIRE(inputStore.flatReadCount() == 0);
    REQUIRE(outputStore.writtenValues() == NumSlots(projDim));
  };

  SECTION("maximum")
  {
    requireDirect.template operator()<int32>(MaxReduce{});
  }
  SECTION("mean")
  {
    requireDirect.template operator()<float32>(MeanReduce{});
  }
  SECTION("median")
  {
    requireDirect.template operator()<int32>(MedianReduce{});
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: pre-cancelled projection leaves output unchanged", "[ImageProcessing][AxisProjectionEngine]")
{
  const usize projDim = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(projDim);

  std::vector<int32> input(k_Volume);
  for(usize i = 0; i < k_Volume; ++i)
  {
    input[i] = static_cast<int32>((i * 7) % 53);
  }
  constexpr int32 k_Poison = -12345;
  const auto requirePreserved = [&](auto reduce) {
    DataStore<int32> inStore = MakeInputStore(input);
    DataStore<int32> outStore(ShapeType{NumSlots(projDim)}, ShapeType{1}, k_Poison);
    std::atomic_bool shouldCancel{true};
    IFilter::MessageHandler messageHandler{};

    const Result<> result = ApplyAxisProjection<int32, int32>(inStore, outStore, SizeVec3{k_X, k_Y, k_Z}, projDim, reduce, shouldCancel, messageHandler);
    REQUIRE(result.valid());
    for(usize i = 0; i < outStore.getSize(); ++i)
    {
      REQUIRE(outStore.getValue(i) == k_Poison);
    }
  };

  SECTION("non-associative median")
  {
    requirePreserved(MedianReduce{});
  }
  SECTION("associative mean")
  {
    requirePreserved(MeanReduce{});
  }
  SECTION("associative binary")
  {
    requirePreserved(BinaryReduce{1.0, 0.0});
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: rejects out-of-range projDim", "[ImageProcessing][AxisProjectionEngine]")
{
  std::vector<int32> ramp(k_Volume);
  for(usize i = 0; i < k_Volume; ++i)
  {
    ramp[i] = static_cast<int32>(i);
  }
  DataStore<int32> inStore = MakeInputStore(ramp);
  DataStore<int32> outStore(ShapeType{k_Volume}, ShapeType{1}, 0);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  // projDim == 3 is out of range; the engine must fail cleanly rather than perform an unchecked
  // Vec3 OOB read.
  const Result<> result = ApplyAxisProjection<int32, int32>(inStore, outStore, SizeVec3{k_X, k_Y, k_Z}, /*projDim=*/3, MaxReduce{}, shouldCancel, messageHandler);
  REQUIRE(result.invalid());
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: median pencil rejects a non-3D input tuple shape", "[ImageProcessing][AxisProjectionEngine]")
{
  // The median (pencil) path reads a 3-axis extent via readExtent, which returns EMPTY when the store's tuple
  // shape is not 3-dimensional. Rather than silently emit a zero image and report success, the engine must fail
  // cleanly. A normal image cell array is always {Z,Y,X}, so this exercises the defensive guard directly.
  DataStore<int32> inStore(ShapeType{k_Volume}, ShapeType{1}, 0); // rank-1 tuple shape (not 3)
  DataStore<int32> outStore(ShapeType{NumSlots(2)}, ShapeType{1}, 0);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = ApplyAxisProjection<int32, int32>(inStore, outStore, SizeVec3{k_X, k_Y, k_Z}, /*projDim=*/2, MedianReduce{}, shouldCancel, messageHandler);
  REQUIRE(result.invalid());
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: checked true-2-D plans", "[ImageProcessing][AxisProjectionEngine]")
{
  constexpr usize k_MiB = 1024 * 1024;
  constexpr usize k_SmallTarget = 96 * 1024;

  SECTION("associative X rows and overwide chunks")
  {
    const auto rows = projection_detail::CreateAxisProjection2DPlan<float32, float64, MeanReduce>(SizeVec3{4097, 17, 1}, 0, k_MiB);
    REQUIRE(rows.valid());
    REQUIRE(rows.value().route == projection_detail::AxisProjection2DRoute::AssociativeXRows);
    REQUIRE(rows.value().blockRows > 1);
    REQUIRE(rows.value().residentBytes <= k_MiB);

    const auto chunks = projection_detail::CreateAxisProjection2DPlan<float32, float64, MeanReduce>(SizeVec3{100000, 3, 1}, 0, k_SmallTarget);
    REQUIRE(chunks.valid());
    REQUIRE(chunks.value().route == projection_detail::AxisProjection2DRoute::AssociativeXChunks);
    REQUIRE(chunks.value().inputValues < 100000);
    REQUIRE(chunks.value().residentBytes <= k_SmallTarget);
  }

  SECTION("associative Y rows and X tiles")
  {
    const auto rows = projection_detail::CreateAxisProjection2DPlan<float32, float64, StdDevReduce>(SizeVec3{257, 4097, 1}, 1, k_MiB);
    REQUIRE(rows.valid());
    REQUIRE(rows.value().route == projection_detail::AxisProjection2DRoute::AssociativeYRows);
    REQUIRE(rows.value().blockRows > 1);
    REQUIRE(rows.value().residentBytes <= k_MiB);

    const auto tiles = projection_detail::CreateAxisProjection2DPlan<float32, float64, StdDevReduce>(SizeVec3{4097, 4097, 1}, 1, k_SmallTarget);
    REQUIRE(tiles.valid());
    REQUIRE(tiles.value().route == projection_detail::AxisProjection2DRoute::AssociativeYTiles);
    REQUIRE(tiles.value().tileColumns > 0);
    REQUIRE(tiles.value().tileColumns < 4097);
    REQUIRE(tiles.value().residentBytes <= k_SmallTarget);
  }

  SECTION("fitting and external Median pencils")
  {
    const auto xRows = projection_detail::CreateAxisProjection2DPlan<float32, float32, MedianReduce>(SizeVec3{19, 17, 1}, 0, k_MiB);
    REQUIRE(xRows.valid());
    REQUIRE(xRows.value().route == projection_detail::AxisProjection2DRoute::MedianXRows);
    REQUIRE(xRows.value().blockRows == 17);

    const auto yTiles = projection_detail::CreateAxisProjection2DPlan<float32, float32, MedianReduce>(SizeVec3{17, 19, 1}, 1, k_MiB);
    REQUIRE(yTiles.valid());
    REQUIRE(yTiles.value().route == projection_detail::AxisProjection2DRoute::MedianYTiles);
    REQUIRE(yTiles.value().tileColumns == 17);

    const auto xExternal = projection_detail::CreateAxisProjection2DPlan<float32, float32, MedianReduce>(SizeVec3{262145, 3, 1}, 0, k_MiB);
    REQUIRE(xExternal.valid());
    REQUIRE(xExternal.value().route == projection_detail::AxisProjection2DRoute::MedianXExternal);
    REQUIRE(xExternal.value().sortBatchValues > 0);
    REQUIRE(xExternal.value().residentBytes <= k_MiB);

    const auto yExternal = projection_detail::CreateAxisProjection2DPlan<float32, float32, MedianReduce>(SizeVec3{17, 262145, 1}, 1, k_MiB);
    REQUIRE(yExternal.valid());
    REQUIRE(yExternal.value().route == projection_detail::AxisProjection2DRoute::MedianYExternal);
    REQUIRE(yExternal.value().sortBatchValues > 0);
    REQUIRE(yExternal.value().blockRows > 0);
    REQUIRE(yExternal.value().tileColumns > 0);
    REQUIRE(yExternal.value().residentBytes <= k_MiB);

    const usize sortAndOutputBytes = yExternal.value().sortBatchValues * (sizeof(float32) + sizeof(uint64)) + yExternal.value().outputValues * sizeof(float32);
    REQUIRE(sortAndOutputBytes + projection_detail::k_AxisProjection2DMetadataBytes <= k_MiB);
  }

  SECTION("singleton Z, empty image, and rejected plans")
  {
    const auto singleton = projection_detail::CreateAxisProjection2DPlan<int16, float64, SumReduce>(SizeVec3{100000, 3, 1}, 2, k_SmallTarget);
    REQUIRE(singleton.valid());
    REQUIRE(singleton.value().route == projection_detail::AxisProjection2DRoute::SingletonZ);
    REQUIRE(singleton.value().inputValues < 300000);
    REQUIRE(singleton.value().residentBytes <= k_SmallTarget);

    const auto empty = projection_detail::CreateAxisProjection2DPlan<float32, float32, MedianReduce>(SizeVec3{0, 7, 1}, 1, 1);
    REQUIRE(empty.valid());
    REQUIRE(empty.value().residentBytes == 0);

    REQUIRE(projection_detail::CreateAxisProjection2DPlan<float32, float32, MaxReduce>(SizeVec3{7, 7, 1}, 3, k_MiB).invalid());
    REQUIRE(projection_detail::CreateAxisProjection2DPlan<float32, float32, MaxReduce>(SizeVec3{7, 7, 2}, 1, k_MiB).invalid());
    REQUIRE(projection_detail::CreateAxisProjection2DPlan<float32, float32, MaxReduce>(SizeVec3{7, 7, 1}, 1, projection_detail::k_AxisProjection2DMetadataBytes).invalid());
    REQUIRE(projection_detail::CreateAxisProjection2DPlan<float32, float32, MaxReduce>(SizeVec3{std::numeric_limits<usize>::max(), 2, 1}, 1, k_MiB).invalid());
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: associative bounded true-2-D axes", "[ImageProcessing][AxisProjectionEngine]")
{
  SECTION("X complete-row batches")
  {
    constexpr usize k_RowBytes = 7 * sizeof(int32) + sizeof(MaxReduce::State<int32>) + sizeof(int32);
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 2 * k_RowBytes;
    RequireBoundedAssociativeMatchesResident<int32, int32>(SizeVec3{7, 5, 1}, 0, MaxReduce{}, k_Target, projection_detail::AxisProjection2DRoute::AssociativeXRows);
  }

  SECTION("X overwide chunks")
  {
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + sizeof(MinReduce::State<int32>) + sizeof(int32) + 3 * sizeof(int32);
    RequireBoundedAssociativeMatchesResident<int32, int32>(SizeVec3{17, 3, 1}, 0, MinReduce{}, k_Target, projection_detail::AxisProjection2DRoute::AssociativeXChunks);
  }

  SECTION("Y complete-row blocks")
  {
    constexpr usize k_FixedBytes = 5 * (sizeof(MeanReduce::State<float64>) + sizeof(float64));
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + k_FixedBytes + 2 * 5 * sizeof(int32);
    RequireBoundedAssociativeMatchesResident<int32, float64>(SizeVec3{5, 7, 1}, 1, MeanReduce{}, k_Target, projection_detail::AxisProjection2DRoute::AssociativeYRows);
  }

  SECTION("Y X tiles")
  {
    constexpr usize k_ValuesPerColumn = sizeof(StdDevReduce::State<float64>) + sizeof(int32) + sizeof(float64);
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 3 * k_ValuesPerColumn;
    RequireBoundedAssociativeMatchesResident<int32, float64>(SizeVec3{7, 5, 1}, 1, StdDevReduce{}, k_Target, projection_detail::AxisProjection2DRoute::AssociativeYTiles);
  }

  SECTION("Binary Y X tiles")
  {
    constexpr usize k_ValuesPerColumn = sizeof(BinaryReduce::State<int32>) + sizeof(int32) + sizeof(int32);
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 3 * k_ValuesPerColumn;
    RequireBoundedAssociativeMatchesResident<int32, int32>(SizeVec3{7, 5, 1}, 1, BinaryReduce{1.0, -4.0}, k_Target, projection_detail::AxisProjection2DRoute::AssociativeYTiles);
  }

  SECTION("singleton Z chunks")
  {
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 4 * (sizeof(int32) + sizeof(float64));
    RequireBoundedAssociativeMatchesResident<int32, float64>(SizeVec3{17, 3, 1}, 2, SumReduce{}, k_Target, projection_detail::AxisProjection2DRoute::SingletonZ);
  }

  SECTION("Y row blocks preserve Kahan input order")
  {
    constexpr usize k_DimX = 5;
    constexpr usize k_DimY = 4;
    const SizeVec3 dims{k_DimX, k_DimY, 1};
    constexpr std::array<float64, k_DimY> k_Sequence = {1.0e16, 1.0, -1.0e16, 3.0};
    ProjectionTransferStore<float64> inputStore(ShapeType{1, k_DimY, k_DimX}, 0.0);
    DataStore<float64> residentInput(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0.0);
    for(usize y = 0; y < k_DimY; ++y)
    {
      for(usize x = 0; x < k_DimX; ++x)
      {
        inputStore.setValue(y * k_DimX + x, k_Sequence[y]);
        residentInput.setValue(y * k_DimX + x, k_Sequence[y]);
      }
    }
    ProjectionTransferStore<float64> boundedOutput(ShapeType{k_DimX}, -12345.0);
    DataStore<float64> residentOutput(ShapeType{k_DimX}, ShapeType{1}, -12345.0);
    constexpr usize k_FixedBytes = k_DimX * (sizeof(SumReduce::State<float64>) + sizeof(float64));
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + k_FixedBytes + 2 * k_DimX * sizeof(float64);
    const auto plan = projection_detail::CreateAxisProjection2DPlan<float64, float64, SumReduce>(dims, 1, k_Target);
    REQUIRE(plan.valid());
    REQUIRE(plan.value().route == projection_detail::AxisProjection2DRoute::AssociativeYRows);
    REQUIRE(plan.value().blockRows == 2);

    const auto residentShape = projection_detail::ValidateAxisProjection(residentInput, residentOutput, dims, 1);
    const auto boundedShape = projection_detail::ValidateAxisProjection(inputStore, boundedOutput, dims, 1);
    REQUIRE(residentShape.valid());
    REQUIRE(boundedShape.valid());
    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    REQUIRE(projection_detail::ApplyAxisProjectionResident(residentInput, residentOutput, residentShape.value(), 1, SumReduce{}, shouldCancel, messageHandler).valid());
    REQUIRE(projection_detail::ApplyAxisProjection2D(inputStore, boundedOutput, boundedShape.value(), 1, SumReduce{}, shouldCancel, k_Target).valid());
    for(usize x = 0; x < k_DimX; ++x)
    {
      REQUIRE(boundedOutput.getValue(x) == residentOutput.getValue(x));
      REQUIRE(boundedOutput.getValue(x) == 3.0);
    }
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: associative bounded cancellation and I/O failures", "[ImageProcessing][AxisProjectionEngine]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 7;
  const SizeVec3 k_Dims{k_DimX, k_DimY, 1};
  constexpr usize k_OutputValues = k_DimX;
  constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + k_DimX * (sizeof(MeanReduce::State<float64>) + sizeof(float64)) + 2 * k_DimX * sizeof(int32);

  ProjectionTransferStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, int32{});
  for(usize index = 0; index < k_DimX * k_DimY; ++index)
  {
    inputStore.setValue(index, static_cast<int32>(index));
  }
  inputStore.resetTransferCounts();
  constexpr float64 k_Poison = -12345.0;
  ProjectionTransferStore<float64> outputStore(ShapeType{k_OutputValues}, k_Poison);
  const auto shapeResult = projection_detail::ValidateAxisProjection(inputStore, outputStore, k_Dims, 1);
  REQUIRE(shapeResult.valid());

  SECTION("pre-cancel preserves poison")
  {
    std::atomic_bool shouldCancel{true};
    const Result<> result = projection_detail::ApplyAxisProjection2D(inputStore, outputStore, shapeResult.value(), 1, MeanReduce{}, shouldCancel, k_Target);
    REQUIRE(result.valid());
    REQUIRE(outputStore.writtenValues() == 0);
    for(usize index = 0; index < k_OutputValues; ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }

  SECTION("cancel after input read preserves poison")
  {
    std::atomic_bool shouldCancel{false};
    inputStore.cancelAfterRead(1, shouldCancel);
    const Result<> result = projection_detail::ApplyAxisProjection2D(inputStore, outputStore, shapeResult.value(), 1, MeanReduce{}, shouldCancel, k_Target);
    REQUIRE(result.valid());
    REQUIRE(shouldCancel);
    REQUIRE(outputStore.writtenValues() == 0);
    for(usize index = 0; index < k_OutputValues; ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }

  SECTION("fitting Median cancel after row-tile read preserves poison")
  {
    const usize workerCount = projection_detail::AxisProjectionMedianWorkerCount(k_DimX);
    const usize k_MedianTarget = projection_detail::k_AxisProjection2DMetadataBytes + workerCount * k_DimY * sizeof(int32) + k_DimX * (k_DimY * sizeof(int32) + sizeof(float64));
    std::atomic_bool shouldCancel{false};
    inputStore.cancelAfterRead(1, shouldCancel);
    const Result<> result = projection_detail::ApplyAxisProjection2D(inputStore, outputStore, shapeResult.value(), 1, MedianReduce{}, shouldCancel, k_MedianTarget);
    REQUIRE(result.valid());
    REQUIRE(shouldCancel);
    REQUIRE(outputStore.writtenValues() == 0);
    for(usize index = 0; index < k_OutputValues; ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }

  SECTION("input failure propagates")
  {
    std::atomic_bool shouldCancel{false};
    inputStore.failRead(1, -9901);
    const Result<> result = projection_detail::ApplyAxisProjection2D(inputStore, outputStore, shapeResult.value(), 1, MeanReduce{}, shouldCancel, k_Target);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -9901);
    REQUIRE(outputStore.writtenValues() == 0);
  }

  SECTION("output failure propagates")
  {
    std::atomic_bool shouldCancel{false};
    outputStore.failWrite(1, -9902);
    const Result<> result = projection_detail::ApplyAxisProjection2D(inputStore, outputStore, shapeResult.value(), 1, MeanReduce{}, shouldCancel, k_Target);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -9902);
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: fitting Median bounded true-2-D axes", "[ImageProcessing][AxisProjectionEngine]")
{
  SECTION("even X pencils in multiple row batches")
  {
    constexpr usize k_RowBytes = 4 * sizeof(int32) + sizeof(int32);
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 2 * k_RowBytes;
    RequireBoundedMedianMatchesResident(SizeVec3{4, 5, 1}, 0, k_Target, projection_detail::AxisProjection2DRoute::MedianXRows);
  }

  SECTION("odd X pencils in multiple row batches")
  {
    constexpr usize k_RowBytes = 5 * sizeof(int32) + sizeof(int32);
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 2 * k_RowBytes;
    RequireBoundedMedianMatchesResident(SizeVec3{5, 5, 1}, 0, k_Target, projection_detail::AxisProjection2DRoute::MedianXRows);
  }

  SECTION("even Y pencils in column tiles with a short tail")
  {
    const usize workerCount = projection_detail::AxisProjectionMedianWorkerCount(7);
    const usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + workerCount * 4 * sizeof(int32) + 2 * (4 * sizeof(int32) + sizeof(int32));
    RequireBoundedMedianMatchesResident(SizeVec3{7, 4, 1}, 1, k_Target, projection_detail::AxisProjection2DRoute::MedianYTiles);
  }

  SECTION("odd Y pencils in column tiles with a short tail")
  {
    const usize workerCount = projection_detail::AxisProjectionMedianWorkerCount(7);
    const usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + workerCount * 5 * sizeof(int32) + 2 * (5 * sizeof(int32) + sizeof(int32));
    RequireBoundedMedianMatchesResident(SizeVec3{7, 5, 1}, 1, k_Target, projection_detail::AxisProjection2DRoute::MedianYTiles);
  }

  SECTION("singleton Z chunks")
  {
    constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 3 * (sizeof(int32) + sizeof(int32));
    RequireBoundedMedianMatchesResident(SizeVec3{7, 5, 1}, 2, k_Target, projection_detail::AxisProjection2DRoute::SingletonZ);
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: external Median long X and Y pencils", "[ImageProcessing][AxisProjectionEngine]")
{
  constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 60;

  SECTION("long contiguous X pencils")
  {
    auto control = std::make_shared<FakeExternalSortControl>();
    std::shared_ptr<ProjectionTransferStore<int32>> transposeStore;
    const auto services = MakeFakeProjectionServices(control, transposeStore);
    RequireBoundedMedianMatchesResident(SizeVec3{17, 3, 1}, 0, k_Target, projection_detail::AxisProjection2DRoute::MedianXExternal, &services);
    const auto plan = projection_detail::CreateAxisProjection2DPlan<int32, int32, MedianReduce>(SizeVec3{17, 3, 1}, 0, k_Target);
    REQUIRE(plan.valid());
    REQUIRE(control->createCount == 3);
    REQUIRE(control->maxAppendRecords <= plan.value().sortBatchValues);
    REQUIRE(transposeStore == nullptr);
  }

  SECTION("long strided Y pencils use one typed transpose")
  {
    auto control = std::make_shared<FakeExternalSortControl>();
    std::shared_ptr<ProjectionTransferStore<int32>> transposeStore;
    const auto services = MakeFakeProjectionServices(control, transposeStore);
    RequireBoundedMedianMatchesResident(SizeVec3{5, 17, 1}, 1, k_Target, projection_detail::AxisProjection2DRoute::MedianYExternal, &services);
    const auto plan = projection_detail::CreateAxisProjection2DPlan<int32, int32, MedianReduce>(SizeVec3{5, 17, 1}, 1, k_Target);
    REQUIRE(plan.valid());
    REQUIRE(control->createCount == 5);
    REQUIRE(control->maxAppendRecords <= plan.value().sortBatchValues);
    REQUIRE(transposeStore != nullptr);
    REQUIRE(transposeStore->getSize() == 5 * 17);
    for(usize x = 0; x < 5; ++x)
    {
      for(usize y = 0; y < 17; ++y)
      {
        const usize sourceIndex = y * 5 + x;
        const int32 expected = static_cast<int32>((sourceIndex * 11 + 3) % 29);
        REQUIRE(transposeStore->getValue(x * 17 + y) == expected);
      }
    }
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: external Median provider failures", "[ImageProcessing][AxisProjectionEngine]")
{
  auto control = std::make_shared<FakeExternalSortControl>();
  std::shared_ptr<ProjectionTransferStore<int32>> transposeStore;
  auto services = MakeFakeProjectionServices(control, transposeStore);

  SECTION("missing external-sort factory")
  {
    services.createExternalSort = {};
    const Result<> result = RunExternalMedianWithServices(0, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8746);
  }

  SECTION("sort creation failure")
  {
    control->failCreate = true;
    const Result<> result = RunExternalMedianWithServices(0, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8746);
  }

  SECTION("append failure")
  {
    control->failAppend = true;
    const Result<> result = RunExternalMedianWithServices(0, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8747);
  }

  SECTION("finish failure")
  {
    control->failFinish = true;
    const Result<> result = RunExternalMedianWithServices(0, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8748);
  }

  SECTION("read failure")
  {
    control->failRead = true;
    const Result<> result = RunExternalMedianWithServices(0, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8749);
  }

  SECTION("short read")
  {
    control->shortRead = true;
    const Result<> result = RunExternalMedianWithServices(0, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8749);
  }

  SECTION("record-count mismatch")
  {
    control->wrongRecordCount = true;
    const Result<> result = RunExternalMedianWithServices(0, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8749);
  }

  SECTION("transpose creation failure")
  {
    control->failTransposeCreate = true;
    const Result<> result = RunExternalMedianWithServices(1, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8750);
  }

  SECTION("transpose write failure")
  {
    control->failTransposeWrite = true;
    const Result<> result = RunExternalMedianWithServices(1, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -9923);
  }

  SECTION("transpose read failure")
  {
    control->failTransposeRead = true;
    const Result<> result = RunExternalMedianWithServices(1, &services);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -9922);
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: external record comparator is deterministic", "[ImageProcessing][AxisProjectionEngine]")
{
  const auto compare = [](float32 left, float32 right) {
    std::array<std::byte, sizeof(float32)> leftBytes{};
    std::array<std::byte, sizeof(float32)> rightBytes{};
    std::memcpy(leftBytes.data(), &left, sizeof(float32));
    std::memcpy(rightBytes.data(), &right, sizeof(float32));
    return projection_detail::CompareAxisProjectionRecords<float32>(leftBytes, rightBytes);
  };

  const float32 nan = std::numeric_limits<float32>::quiet_NaN();
  REQUIRE(compare(-1.0F, 2.0F) < 0);
  REQUIRE(compare(2.0F, -1.0F) > 0);
  REQUIRE(compare(2.0F, 2.0F) == 0);
  REQUIRE(compare(2.0F, nan) < 0);
  REQUIRE(compare(nan, 2.0F) > 0);
  REQUIRE(compare(nan, nan) == 0);
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: external Median cancellation preserves output", "[ImageProcessing][AxisProjectionEngine]")
{
  const SizeVec3 k_Dims{17, 3, 1};
  constexpr usize k_OutputValues = 3;
  constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 60;
  ProjectionTransferStore<int32> inputStore(ShapeType{1, 3, 17}, int32{});
  ProjectionTransferStore<int32> outputStore(ShapeType{k_OutputValues}, -12345);
  for(usize index = 0; index < 51; ++index)
  {
    inputStore.setValue(index, static_cast<int32>((index * 11 + 3) % 29));
  }
  inputStore.resetTransferCounts();
  std::atomic_bool shouldCancel{false};
  inputStore.cancelAfterRead(1, shouldCancel);
  const auto shape = projection_detail::ValidateAxisProjection(inputStore, outputStore, k_Dims, 0);
  REQUIRE(shape.valid());
  auto control = std::make_shared<FakeExternalSortControl>();
  std::shared_ptr<ProjectionTransferStore<int32>> transposeStore;
  const auto services = MakeFakeProjectionServices(control, transposeStore);
  const Result<> result = projection_detail::ApplyAxisProjection2D(inputStore, outputStore, shape.value(), 0, MedianReduce{}, shouldCancel, k_Target, &services);
  REQUIRE(result.valid());
  REQUIRE(shouldCancel);
  REQUIRE(outputStore.writtenValues() == 0);
  for(usize index = 0; index < k_OutputValues; ++index)
  {
    REQUIRE(outputStore.getValue(index) == -12345);
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: endpoint-aware true-2-D selection", "[ImageProcessing][AxisProjectionEngine]")
{
  const bool inputOutOfCore = GENERATE(false, true);
  const bool outputOutOfCore = GENERATE(false, true);
  CAPTURE(inputOutOfCore, outputOutOfCore);
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 5;
  const SizeVec3 dims{k_DimX, k_DimY, 1};
  ProjectionTransferStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, int32{}, inputOutOfCore);
  ProjectionTransferStore<float64> outputStore(ShapeType{k_DimX}, -12345.0, outputOutOfCore);
  for(usize index = 0; index < k_DimX * k_DimY; ++index)
  {
    inputStore.setValue(index, static_cast<int32>(index));
  }
  inputStore.resetTransferCounts();
  constexpr usize k_BytesPerColumn = sizeof(MeanReduce::State<float64>) + sizeof(int32) + sizeof(float64);
  constexpr usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + 3 * k_BytesPerColumn;
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> result = ApplyAxisProjection(inputStore, outputStore, dims, 1, MeanReduce{}, shouldCancel, messageHandler, k_Target);
  REQUIRE(result.valid());
  if(inputOutOfCore || outputOutOfCore)
  {
    const auto plan = projection_detail::CreateAxisProjection2DPlan<int32, float64, MeanReduce>(dims, 1, k_Target);
    REQUIRE(plan.valid());
    REQUIRE(inputStore.maxReadValues() <= plan.value().inputValues);
    REQUIRE(inputStore.maxReadValues() < k_DimX * k_DimY);
  }
  else
  {
    REQUIRE(inputStore.flatReadCount() == 0);
    REQUIRE(inputStore.maxReadValues() == 0);
  }
  for(usize x = 0; x < k_DimX; ++x)
  {
    REQUIRE(outputStore.getValue(x) == static_cast<float64>(2 * k_DimX + x));
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: endpoint-aware fitting Median selection", "[ImageProcessing][AxisProjectionEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 5;
  const SizeVec3 dims{k_DimX, k_DimY, 1};
  ProjectionTransferStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, int32{});
  ProjectionTransferStore<int32> outputStore(ShapeType{k_DimX}, -12345);
  std::vector<std::vector<int32>> pencils(k_DimX);
  for(usize y = 0; y < k_DimY; ++y)
  {
    for(usize x = 0; x < k_DimX; ++x)
    {
      const int32 value = static_cast<int32>(((y * k_DimX + x) * 11 + 3) % 29);
      inputStore.setValue(y * k_DimX + x, value);
      pencils[x].push_back(value);
    }
  }
  inputStore.resetTransferCounts();
  const usize workerCount = projection_detail::AxisProjectionMedianWorkerCount(k_DimX);
  const usize k_Target = projection_detail::k_AxisProjection2DMetadataBytes + workerCount * k_DimY * sizeof(int32) + 2 * (k_DimY * sizeof(int32) + sizeof(int32));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  REQUIRE(ApplyAxisProjection(inputStore, outputStore, dims, 1, MedianReduce{}, shouldCancel, messageHandler, k_Target).valid());
  const auto plan = projection_detail::CreateAxisProjection2DPlan<int32, int32, MedianReduce>(dims, 1, k_Target);
  REQUIRE(plan.valid());
  REQUIRE(inputStore.maxReadValues() <= plan.value().inputValues);
  REQUIRE(inputStore.maxReadValues() < k_DimX * k_DimY);
  for(usize x = 0; x < k_DimX; ++x)
  {
    std::sort(pencils[x].begin(), pencils[x].end());
    REQUIRE(outputStore.getValue(x) == pencils[x][k_DimY / 2]);
  }
}

TEST_CASE("ImageProcessing::AxisProjectionEngine: OOC-backed Z-greater-than-one stays resident", "[ImageProcessing][AxisProjectionEngine]")
{
  constexpr usize k_DimX = 4;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 2;
  constexpr usize k_TestVolume = k_DimX * k_DimY * k_DimZ;
  const SizeVec3 dims{k_DimX, k_DimY, k_DimZ};
  ProjectionTransferStore<int32> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, int32{});
  ProjectionTransferStore<int32> outputStore(ShapeType{k_DimX * k_DimZ}, -12345);
  for(usize index = 0; index < k_TestVolume; ++index)
  {
    inputStore.setValue(index, static_cast<int32>(index));
  }
  inputStore.resetTransferCounts();
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  REQUIRE(ApplyAxisProjection(inputStore, outputStore, dims, 1, MaxReduce{}, shouldCancel, messageHandler, projection_detail::k_AxisProjection2DMetadataBytes + 1).valid());
  REQUIRE(inputStore.maxReadValues() == k_TestVolume);
}
