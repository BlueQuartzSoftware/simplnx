#include "simplnx/Common/Extent.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
constexpr int32 k_ExtentReadFailure = -91010;
constexpr int32 k_SourceReadFailure = -91011;
constexpr int32 k_DestinationWriteFailure = -91012;
constexpr int32 k_ExtentWarningCode = -91013;

class TrackingDataStore final : public DataStore<int32>
{
public:
  using DataStore<int32>::DataStore;

  mutable usize m_BulkReadCalls = 0;
  usize m_BulkWriteCalls = 0;
  mutable usize m_ExtentReadCalls = 0;
  mutable usize m_MaxReadValues = 0;
  usize m_MaxWriteValues = 0;
  std::optional<usize> m_FailBulkReadCall;
  std::optional<usize> m_FailBulkWriteCall;
  std::optional<usize> m_FailExtentReadCall;
  bool m_AddExtentWarning = false;

  [[nodiscard]] Result<> copyIntoBuffer(usize startIndex, nonstd::span<int32> buffer) const override
  {
    ++m_BulkReadCalls;
    m_MaxReadValues = (std::max)(m_MaxReadValues, buffer.size());
    if(m_FailBulkReadCall == m_BulkReadCalls)
    {
      return MakeErrorResult(k_SourceReadFailure, "TrackingDataStore injected source bulk-read failure.");
    }
    return DataStore<int32>::copyIntoBuffer(startIndex, buffer);
  }

  [[nodiscard]] Result<> copyFromBuffer(usize startIndex, nonstd::span<const int32> buffer) override
  {
    ++m_BulkWriteCalls;
    m_MaxWriteValues = (std::max)(m_MaxWriteValues, buffer.size());
    if(m_FailBulkWriteCall == m_BulkWriteCalls)
    {
      return MakeErrorResult(k_DestinationWriteFailure, "TrackingDataStore injected destination bulk-write failure.");
    }
    return DataStore<int32>::copyFromBuffer(startIndex, buffer);
  }

  [[nodiscard]] Result<> readExtentIntoBuffer(const Extent& extent, nonstd::span<int32> destination) const override
  {
    ++m_ExtentReadCalls;
    if(m_FailExtentReadCall == m_ExtentReadCalls)
    {
      return MakeErrorResult(k_ExtentReadFailure, "TrackingDataStore injected extent read failure.");
    }

    Result<> result = DataStore<int32>::readExtentIntoBuffer(extent, destination);
    if(result.valid() && m_AddExtentWarning)
    {
      result.warnings().push_back(Warning{k_ExtentWarningCode, "TrackingDataStore extent-read warning."});
    }
    return result;
  }
};

void RequireErrorCode(const Result<>& result, int32 errorCode)
{
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors()[0].code == errorCode);
}

template <typename T>
void RequireAllValues(const T* values, usize count, T expectedValue)
{
  REQUIRE(std::all_of(values, values + count, [expectedValue](T value) { return value == expectedValue; }));
}

std::vector<int32> Expected3DExtentValues(const Extent& extent, const ShapeType& tupleShape, usize numComponents)
{
  std::vector<int32> values;
  for(uint64 z = extent.min[0]; z <= extent.max[0]; z += extent.stride[0])
  {
    for(uint64 y = extent.min[1]; y <= extent.max[1]; y += extent.stride[1])
    {
      for(uint64 x = extent.min[2]; x <= extent.max[2]; x += extent.stride[2])
      {
        const usize tupleIndex = static_cast<usize>((z * tupleShape[1] + y) * tupleShape[2] + x);
        for(usize component = 0; component < numComponents; ++component)
        {
          values.push_back(static_cast<int32>(tupleIndex * 10 + component));
        }
      }
    }
  }
  return values;
}

void FillTupleValues(DataStore<int32>& store)
{
  for(usize tupleIndex = 0; tupleIndex < store.getNumberOfTuples(); ++tupleIndex)
  {
    for(usize component = 0; component < store.getNumberOfComponents(); ++component)
    {
      store.setValue(tupleIndex * store.getNumberOfComponents() + component, static_cast<int32>(tupleIndex * 10 + component));
    }
  }
}
} // namespace

TEMPLATE_TEST_CASE("DataStore bulk transfers validate ranges for resident scalar and boolean stores", "[simplnx][DataStore][DataStoreBulk]", int32, bool)
{
  DataStore<TestType> store(ShapeType{4}, ShapeType{2}, TestType{});
  for(usize valueIndex = 0; valueIndex < store.getSize(); ++valueIndex)
  {
    if constexpr(std::is_same_v<TestType, bool>)
    {
      store.setValue(valueIndex, (valueIndex % 2) == 0);
    }
    else
    {
      store.setValue(valueIndex, static_cast<TestType>(valueIndex * 10));
    }
  }

  std::array<TestType, 3> readValues = {TestType{}, TestType{}, TestType{}};
  REQUIRE(store.copyIntoBuffer(2, nonstd::span<TestType>(readValues.data(), readValues.size())).valid());
  if constexpr(std::is_same_v<TestType, bool>)
  {
    REQUIRE(readValues == std::array<bool, 3>{true, false, true});
  }
  else
  {
    REQUIRE(readValues == std::array<int32, 3>{20, 30, 40});
  }

  std::array<TestType, 0> emptyValues = {};
  REQUIRE(store.copyIntoBuffer(store.getSize(), nonstd::span<TestType>(emptyValues.data(), emptyValues.size())).valid());
  REQUIRE(store.copyFromBuffer(store.getSize(), nonstd::span<const TestType>(emptyValues.data(), emptyValues.size())).valid());

  const TestType poisonValue = std::is_same_v<TestType, bool> ? true : static_cast<TestType>(-777);
  std::array<TestType, 2> poisonRead = {poisonValue, poisonValue};
  RequireErrorCode(store.copyIntoBuffer(store.getSize() + 1, nonstd::span<TestType>(poisonRead.data(), poisonRead.size())), -6030);
  REQUIRE(poisonRead == std::array<TestType, 2>{poisonValue, poisonValue});
  RequireErrorCode(store.copyIntoBuffer(store.getSize() - 1, nonstd::span<TestType>(poisonRead.data(), poisonRead.size())), -6030);
  REQUIRE(poisonRead == std::array<TestType, 2>{poisonValue, poisonValue});
  RequireErrorCode(store.copyIntoBuffer((std::numeric_limits<usize>::max)(), nonstd::span<TestType>(poisonRead.data(), poisonRead.size())), -6030);
  REQUIRE(poisonRead == std::array<TestType, 2>{poisonValue, poisonValue});

  const std::array<TestType, 2> writeValues = {TestType{}, TestType{}};
  const std::array<TestType, 8> originalValues = {
      store.getValue(0), store.getValue(1), store.getValue(2), store.getValue(3), store.getValue(4), store.getValue(5), store.getValue(6), store.getValue(7),
  };
  RequireErrorCode(store.copyFromBuffer(store.getSize() + 1, nonstd::span<const TestType>(writeValues.data(), writeValues.size())), -6031);
  RequireErrorCode(store.copyFromBuffer(store.getSize() - 1, nonstd::span<const TestType>(writeValues.data(), writeValues.size())), -6031);
  RequireErrorCode(store.copyFromBuffer((std::numeric_limits<usize>::max)(), nonstd::span<const TestType>(writeValues.data(), writeValues.size())), -6031);
  for(usize valueIndex = 0; valueIndex < store.getSize(); ++valueIndex)
  {
    REQUIRE(store.getValue(valueIndex) == originalValues[valueIndex]);
  }
}

TEST_CASE("DataStore extent APIs preserve resident layouts and outside values", "[simplnx][DataStore][DataStoreBulk]")
{
  const ShapeType tupleShape = {2, 3, 4};
  constexpr usize k_NumComponents = 2;
  DataStore<int32> store(tupleShape, ShapeType{k_NumComponents}, 0);
  FillTupleValues(store);

  const Extent contiguousExtent({0, 1, 1}, {1, 2, 3});
  const std::vector<int32> expectedContiguous = Expected3DExtentValues(contiguousExtent, tupleShape, k_NumComponents);
  Result<std::vector<int32>> allocatedResult = store.readExtent(contiguousExtent);
  REQUIRE(allocatedResult.valid());
  REQUIRE(allocatedResult.value() == expectedContiguous);

  std::vector<int32> callerValues(expectedContiguous.size(), -777);
  REQUIRE(store.readExtentIntoBuffer(contiguousExtent, nonstd::span<int32>(callerValues.data(), callerValues.size())).valid());
  REQUIRE(callerValues == expectedContiguous);

  const std::array<Extent, 2> allocatedExtents = {contiguousExtent, Extent({1, 0, 0}, {1, 1, 1})};
  Result<std::vector<std::vector<int32>>> allocatedBatch = store.readExtents(nonstd::span<const Extent>(allocatedExtents.data(), allocatedExtents.size()));
  REQUIRE(allocatedBatch.valid());
  REQUIRE(allocatedBatch.value().size() == allocatedExtents.size());
  REQUIRE(allocatedBatch.value()[0] == expectedContiguous);
  REQUIRE(allocatedBatch.value()[1] == Expected3DExtentValues(allocatedExtents[1], tupleShape, k_NumComponents));

  const Extent xStridedExtent({0, 0, 0}, {1, 2, 3}, {1, 2, 2});
  const std::vector<int32> expectedXStrided = Expected3DExtentValues(xStridedExtent, tupleShape, k_NumComponents);
  std::vector<int32> xStridedValues(expectedXStrided.size(), -777);
  REQUIRE(store.readExtentIntoBuffer(xStridedExtent, nonstd::span<int32>(xStridedValues.data(), xStridedValues.size())).valid());
  REQUIRE(xStridedValues == expectedXStrided);

  DataStore<int32> contiguousWrite(tupleShape, ShapeType{k_NumComponents}, -999);
  std::vector<int32> contiguousInput(expectedContiguous.size());
  for(usize valueIndex = 0; valueIndex < contiguousInput.size(); ++valueIndex)
  {
    contiguousInput[valueIndex] = static_cast<int32>(1000 + valueIndex);
  }
  REQUIRE(contiguousWrite.writeExtent(contiguousExtent, nonstd::span<const int32>(contiguousInput.data(), contiguousInput.size())).valid());
  for(usize tupleIndex = 0; tupleIndex < contiguousWrite.getNumberOfTuples(); ++tupleIndex)
  {
    const bool inExtent = tupleIndex / 12 <= 1 && tupleIndex % 12 / 4 >= 1 && tupleIndex % 4 >= 1;
    for(usize component = 0; component < k_NumComponents; ++component)
    {
      const usize valueIndex = tupleIndex * k_NumComponents + component;
      if(inExtent)
      {
        const usize outputTuple = ((tupleIndex / 12) * 2 + (tupleIndex % 12 / 4 - 1)) * 3 + (tupleIndex % 4 - 1);
        REQUIRE(contiguousWrite.getValue(valueIndex) == contiguousInput[outputTuple * k_NumComponents + component]);
      }
      else
      {
        REQUIRE(contiguousWrite.getValue(valueIndex) == -999);
      }
    }
  }

  DataStore<int32> stridedWrite(tupleShape, ShapeType{k_NumComponents}, -999);
  std::vector<int32> stridedInput(expectedXStrided.size());
  for(usize valueIndex = 0; valueIndex < stridedInput.size(); ++valueIndex)
  {
    stridedInput[valueIndex] = static_cast<int32>(2000 + valueIndex);
  }
  REQUIRE(stridedWrite.writeExtent(xStridedExtent, nonstd::span<const int32>(stridedInput.data(), stridedInput.size())).valid());
  for(usize tupleIndex = 0; tupleIndex < stridedWrite.getNumberOfTuples(); ++tupleIndex)
  {
    const usize z = tupleIndex / 12;
    const usize y = tupleIndex % 12 / 4;
    const usize x = tupleIndex % 4;
    const bool inExtent = y % 2 == 0 && x % 2 == 0;
    for(usize component = 0; component < k_NumComponents; ++component)
    {
      const usize valueIndex = tupleIndex * k_NumComponents + component;
      if(inExtent)
      {
        const usize outputTuple = (z * 2 + y / 2) * 2 + x / 2;
        REQUIRE(stridedWrite.getValue(valueIndex) == stridedInput[outputTuple * k_NumComponents + component]);
      }
      else
      {
        REQUIRE(stridedWrite.getValue(valueIndex) == -999);
      }
    }
  }

  DataStore<int32> oneDimensional(ShapeType{5}, ShapeType{k_NumComponents}, 0);
  FillTupleValues(oneDimensional);
  const Extent oneDimensionalExtent({1}, {4}, {2});
  std::array<int32, 4> oneDimensionalValues = {-777, -777, -777, -777};
  REQUIRE(oneDimensional.readExtentIntoBuffer(oneDimensionalExtent, nonstd::span<int32>(oneDimensionalValues.data(), oneDimensionalValues.size())).valid());
  REQUIRE(oneDimensionalValues == std::array<int32, 4>{10, 11, 30, 31});

  DataStore<bool> booleanStore(ShapeType{1, 2, 4}, ShapeType{2}, false);
  for(usize valueIndex = 0; valueIndex < booleanStore.getSize(); ++valueIndex)
  {
    booleanStore.setValue(valueIndex, (valueIndex % 3) == 0);
  }
  const Extent booleanExtent({0, 0, 0}, {0, 1, 3}, {1, 1, 2});
  Result<std::vector<bool>> booleanRead = booleanStore.readExtent(booleanExtent);
  REQUIRE(booleanRead.valid());
  REQUIRE(booleanRead.value() == std::vector<bool>{true, false, false, false, false, true, true, false});

  const Extent booleanContiguousExtent({0, 0, 0}, {0, 0, 1});
  std::array<bool, 4> booleanContiguousValues = {false, false, false, false};
  REQUIRE(booleanStore.readExtentIntoBuffer(booleanContiguousExtent, nonstd::span<bool>(booleanContiguousValues.data(), booleanContiguousValues.size())).valid());
  REQUIRE(booleanContiguousValues == std::array<bool, 4>{true, false, false, true});

  std::array<bool, 8> booleanXStridedValues = {true, true, true, true, true, true, true, true};
  REQUIRE(booleanStore.readExtentIntoBuffer(booleanExtent, nonstd::span<bool>(booleanXStridedValues.data(), booleanXStridedValues.size())).valid());
  REQUIRE(booleanXStridedValues == std::array<bool, 8>{true, false, false, false, false, true, true, false});

  const std::array<bool, 8> booleanWriteValues = {true, true, false, false, true, false, true, false};
  REQUIRE(booleanStore.writeExtent(booleanExtent, nonstd::span<const bool>(booleanWriteValues.data(), booleanWriteValues.size())).valid());
  for(usize tupleIndex = 0; tupleIndex < booleanStore.getNumberOfTuples(); ++tupleIndex)
  {
    const usize x = tupleIndex % 4;
    const bool written = x == 0 || x == 2;
    for(usize component = 0; component < booleanStore.getNumberOfComponents(); ++component)
    {
      const usize valueIndex = tupleIndex * booleanStore.getNumberOfComponents() + component;
      if(written)
      {
        const usize outputTuple = tupleIndex / 4 * 2 + x / 2;
        REQUIRE(booleanStore.getValue(valueIndex) == booleanWriteValues[outputTuple * 2 + component]);
      }
      else
      {
        REQUIRE(booleanStore.getValue(valueIndex) == ((valueIndex % 3) == 0));
      }
    }
  }
}

TEST_CASE("DataStore extent validation is atomic before backend reads", "[simplnx][DataStore][DataStoreBulk]")
{
  DataStore<int32> store(ShapeType{4}, ShapeType{2}, 0);
  FillTupleValues(store);
  const Extent validExtent({1}, {2});
  Extent zeroStrideExtent({1}, {2});
  zeroStrideExtent.stride[0] = 0;
  Extent reversedExtent({1}, {2});
  reversedExtent.min[0] = 3;
  reversedExtent.max[0] = 2;
  const Extent outOfBoundsExtent({3}, {4});
  const Extent wrongRankExtent({0, 0}, {1, 1});

  std::array<int32, 4> poisonedValues = {-777, -777, -777, -777};
  RequireErrorCode(store.readExtentIntoBuffer(wrongRankExtent, nonstd::span<int32>(poisonedValues.data(), poisonedValues.size())), -6034);
  RequireErrorCode(store.readExtentIntoBuffer(zeroStrideExtent, nonstd::span<int32>(poisonedValues.data(), poisonedValues.size())), -6034);
  RequireErrorCode(store.readExtentIntoBuffer(reversedExtent, nonstd::span<int32>(poisonedValues.data(), poisonedValues.size())), -6034);
  RequireErrorCode(store.readExtentIntoBuffer(outOfBoundsExtent, nonstd::span<int32>(poisonedValues.data(), poisonedValues.size())), -6034);
  RequireErrorCode(store.readExtentIntoBuffer(validExtent, nonstd::span<int32>(poisonedValues.data(), poisonedValues.size() - 1)), -6034);
  REQUIRE(poisonedValues == std::array<int32, 4>{-777, -777, -777, -777});

  const std::array<int32, 3> shortWriteValues = {1, 2, 3};
  RequireErrorCode(store.writeExtent(validExtent, nonstd::span<const int32>(shortWriteValues.data(), shortWriteValues.size())), -6034);
  RequireErrorCode(store.writeExtent(wrongRankExtent, nonstd::span<const int32>(shortWriteValues.data(), shortWriteValues.size())), -6034);
  RequireErrorCode(store.writeExtent(zeroStrideExtent, nonstd::span<const int32>(shortWriteValues.data(), shortWriteValues.size())), -6034);
  RequireErrorCode(store.writeExtent(reversedExtent, nonstd::span<const int32>(shortWriteValues.data(), shortWriteValues.size())), -6034);
  RequireErrorCode(store.writeExtent(outOfBoundsExtent, nonstd::span<const int32>(shortWriteValues.data(), shortWriteValues.size())), -6034);
  for(usize tupleIndex = 0; tupleIndex < store.getNumberOfTuples(); ++tupleIndex)
  {
    REQUIRE(store.getValue(tupleIndex * 2) == static_cast<int32>(tupleIndex * 10));
    REQUIRE(store.getValue(tupleIndex * 2 + 1) == static_cast<int32>(tupleIndex * 10 + 1));
  }
  DataStore<int32> unsupportedRankStore(ShapeType{2, 2}, ShapeType{1}, -999);
  const std::array<int32, 4> twoDimensionalWriteValues = {1, 2, 3, 4};
  RequireErrorCode(unsupportedRankStore.writeExtent(Extent({0, 0}, {1, 1}), nonstd::span<const int32>(twoDimensionalWriteValues.data(), twoDimensionalWriteValues.size())), -6037);
  RequireAllValues(unsupportedRankStore.data(), unsupportedRankStore.getSize(), -999);

  std::array<Extent, 2> batchExtents = {validExtent, outOfBoundsExtent};
  std::array<int32, 4> firstDestination = {-777, -777, -777, -777};
  std::array<int32, 4> secondDestination = {-777, -777, -777, -777};
  std::array<nonstd::span<int32>, 2> destinations = {
      nonstd::span<int32>(firstDestination.data(), firstDestination.size()),
      nonstd::span<int32>(secondDestination.data(), secondDestination.size()),
  };
  RequireErrorCode(store.readExtentsIntoBuffers(nonstd::span<const Extent>(batchExtents.data(), batchExtents.size()), nonstd::span<nonstd::span<int32>>(destinations.data(), destinations.size())),
                   -6034);
  REQUIRE(firstDestination == std::array<int32, 4>{-777, -777, -777, -777});
  REQUIRE(secondDestination == std::array<int32, 4>{-777, -777, -777, -777});

  REQUIRE(store.readExtentsIntoBuffers({}, {}).valid());

  TrackingDataStore trackingStore(ShapeType{4}, ShapeType{2}, 0);
  FillTupleValues(trackingStore);
  trackingStore.m_AddExtentWarning = true;
  const std::array<Extent, 3> warningExtents = {Extent({0}, {0}), Extent({1}, {1}), Extent({2}, {2})};
  std::array<int32, 2> firstWarningDestination = {-777, -777};
  std::array<int32, 2> secondWarningDestination = {-777, -777};
  std::array<int32, 2> thirdWarningDestination = {-777, -777};
  std::array<nonstd::span<int32>, 3> warningDestinations = {
      nonstd::span<int32>(firstWarningDestination.data(), firstWarningDestination.size()),
      nonstd::span<int32>(secondWarningDestination.data(), secondWarningDestination.size()),
      nonstd::span<int32>(thirdWarningDestination.data(), thirdWarningDestination.size()),
  };
  Result<> warningResult = trackingStore.readExtentsIntoBuffers(nonstd::span<const Extent>(warningExtents.data(), warningExtents.size()),
                                                                nonstd::span<nonstd::span<int32>>(warningDestinations.data(), warningDestinations.size()));
  REQUIRE(warningResult.valid());
  REQUIRE(warningResult.warnings().size() == warningExtents.size());
  REQUIRE(std::all_of(warningResult.warnings().begin(), warningResult.warnings().end(), [](const Warning& warning) { return warning.code == k_ExtentWarningCode; }));
  REQUIRE(firstWarningDestination == std::array<int32, 2>{0, 1});
  REQUIRE(secondWarningDestination == std::array<int32, 2>{10, 11});
  REQUIRE(thirdWarningDestination == std::array<int32, 2>{20, 21});

  trackingStore.m_ExtentReadCalls = 0;
  trackingStore.m_FailExtentReadCall = 2;
  firstWarningDestination.fill(-777);
  secondWarningDestination.fill(-777);
  thirdWarningDestination.fill(-777);
  Result<> backendFailure = trackingStore.readExtentsIntoBuffers(nonstd::span<const Extent>(warningExtents.data(), warningExtents.size()),
                                                                 nonstd::span<nonstd::span<int32>>(warningDestinations.data(), warningDestinations.size()));
  RequireErrorCode(backendFailure, k_ExtentReadFailure);
  REQUIRE(backendFailure.warnings().size() == 1);
  REQUIRE(backendFailure.warnings()[0].code == k_ExtentWarningCode);
  REQUIRE(trackingStore.m_ExtentReadCalls == 2);
  REQUIRE(firstWarningDestination == std::array<int32, 2>{0, 1});
  REQUIRE(secondWarningDestination == std::array<int32, 2>{-777, -777});
  REQUIRE(thirdWarningDestination == std::array<int32, 2>{-777, -777});
}

TEST_CASE("DataStore copyFrom uses bounded pages and propagates page-two failures", "[simplnx][DataStore][DataStoreBulk]")
{
  constexpr usize k_PageElements = 1024 * 1024 / sizeof(int32);
  constexpr usize k_Components = 2;
  constexpr usize k_TuplesPerPage = k_PageElements / k_Components;
  constexpr usize k_TuplesToCopy = 2 * k_TuplesPerPage + 7;
  constexpr usize k_SourceTupleOffset = 3;
  constexpr usize k_DestinationTupleOffset = 5;
  constexpr int32 k_DestinationPoison = -777;
  constexpr usize k_SourceTuples = k_SourceTupleOffset + k_TuplesToCopy + 2;
  constexpr usize k_DestinationTuples = k_DestinationTupleOffset + k_TuplesToCopy + 3;
  constexpr usize k_CopyElements = k_TuplesToCopy * k_Components;
  constexpr usize k_SourceStart = k_SourceTupleOffset * k_Components;
  constexpr usize k_DestinationStart = k_DestinationTupleOffset * k_Components;

  TrackingDataStore source(ShapeType{k_SourceTuples}, ShapeType{k_Components}, 0);
  TrackingDataStore destination(ShapeType{k_DestinationTuples}, ShapeType{k_Components}, k_DestinationPoison);
  for(usize valueIndex = 0; valueIndex < source.getSize(); ++valueIndex)
  {
    source.setValue(valueIndex, static_cast<int32>(valueIndex * 3 + 1));
  }

  REQUIRE(destination.copyFrom(k_DestinationTupleOffset, source, k_SourceTupleOffset, k_TuplesToCopy).valid());
  REQUIRE(source.m_BulkReadCalls == 3);
  REQUIRE(destination.m_BulkWriteCalls == 3);
  REQUIRE(source.m_MaxReadValues == k_PageElements);
  REQUIRE(destination.m_MaxWriteValues == k_PageElements);
  REQUIRE(std::equal(source.data() + k_SourceStart, source.data() + k_SourceStart + k_CopyElements, destination.data() + k_DestinationStart));
  RequireAllValues(destination.data(), k_DestinationStart, k_DestinationPoison);
  RequireAllValues(destination.data() + k_DestinationStart + k_CopyElements, destination.getSize() - k_DestinationStart - k_CopyElements, k_DestinationPoison);

  DataStore<bool> boolSource(ShapeType{8}, ShapeType{2}, false);
  DataStore<bool> boolDestination(ShapeType{10}, ShapeType{2}, true);
  for(usize valueIndex = 0; valueIndex < boolSource.getSize(); ++valueIndex)
  {
    boolSource.setValue(valueIndex, (valueIndex % 3) == 0);
  }
  REQUIRE(boolDestination.copyFrom(1, boolSource, 2, 4).valid());
  for(usize valueIndex = 0; valueIndex < 8; ++valueIndex)
  {
    REQUIRE(boolDestination.getValue(2 + valueIndex) == boolSource.getValue(4 + valueIndex));
  }
  REQUIRE(boolDestination.getValue(0));
  REQUIRE(boolDestination.getValue(1));
  REQUIRE(boolDestination.getValue(10));
  REQUIRE(boolDestination.getValue(11));

  TrackingDataStore sourceFailure(ShapeType{k_SourceTuples}, ShapeType{k_Components}, 0);
  TrackingDataStore destinationAfterSourceFailure(ShapeType{k_DestinationTuples}, ShapeType{k_Components}, k_DestinationPoison);
  for(usize valueIndex = 0; valueIndex < sourceFailure.getSize(); ++valueIndex)
  {
    sourceFailure.setValue(valueIndex, static_cast<int32>(valueIndex * 3 + 1));
  }
  sourceFailure.m_FailBulkReadCall = 2;
  RequireErrorCode(destinationAfterSourceFailure.copyFrom(k_DestinationTupleOffset, sourceFailure, k_SourceTupleOffset, k_TuplesToCopy), k_SourceReadFailure);
  REQUIRE(sourceFailure.m_BulkReadCalls == 2);
  REQUIRE(destinationAfterSourceFailure.m_BulkWriteCalls == 1);
  REQUIRE(std::equal(sourceFailure.data() + k_SourceStart, sourceFailure.data() + k_SourceStart + k_PageElements, destinationAfterSourceFailure.data() + k_DestinationStart));
  RequireAllValues(destinationAfterSourceFailure.data() + k_DestinationStart + k_PageElements, k_CopyElements - k_PageElements, k_DestinationPoison);

  TrackingDataStore sourceBeforeDestinationFailure(ShapeType{k_SourceTuples}, ShapeType{k_Components}, 0);
  TrackingDataStore destinationFailure(ShapeType{k_DestinationTuples}, ShapeType{k_Components}, k_DestinationPoison);
  for(usize valueIndex = 0; valueIndex < sourceBeforeDestinationFailure.getSize(); ++valueIndex)
  {
    sourceBeforeDestinationFailure.setValue(valueIndex, static_cast<int32>(valueIndex * 3 + 1));
  }
  destinationFailure.m_FailBulkWriteCall = 2;
  RequireErrorCode(destinationFailure.copyFrom(k_DestinationTupleOffset, sourceBeforeDestinationFailure, k_SourceTupleOffset, k_TuplesToCopy), k_DestinationWriteFailure);
  REQUIRE(sourceBeforeDestinationFailure.m_BulkReadCalls == 2);
  REQUIRE(destinationFailure.m_BulkWriteCalls == 2);
  REQUIRE(std::equal(sourceBeforeDestinationFailure.data() + k_SourceStart, sourceBeforeDestinationFailure.data() + k_SourceStart + k_PageElements, destinationFailure.data() + k_DestinationStart));
  RequireAllValues(destinationFailure.data() + k_DestinationStart + k_PageElements, k_CopyElements - k_PageElements, k_DestinationPoison);

  DataStore<int32> mismatchedComponents(ShapeType{k_SourceTuples}, ShapeType{1}, 0);
  RequireErrorCode(destination.copyFrom(destination.getNumberOfTuples(), source, 0, 1), -14600);
  RequireErrorCode(destination.copyFrom(0, mismatchedComponents, 0, 1), -14601);
  RequireErrorCode(destination.copyFrom(destination.getNumberOfTuples() - 1, source, 0, 2), -14602);
  RequireErrorCode(destination.copyFrom(0, source, source.getNumberOfTuples() - 1, 2), -14603);
}

TEST_CASE("DataArray deepCopy does not insert a destination after a source bulk-read failure", "[simplnx][DataArray][DataStoreBulk]")
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto sourceStore = std::make_shared<TrackingDataStore>(ShapeType{4}, ShapeType{2}, 0);
  sourceStore->m_FailBulkReadCall = 1;
  auto* sourceArray = DataArray<int32>::Create(dataStructure, "Source", sourceStore);
  REQUIRE(sourceArray != nullptr);

  const DataPath copyPath({"Copy"});
  REQUIRE(sourceArray->deepCopy(copyPath) == nullptr);
  REQUIRE_FALSE(dataStructure.containsData(copyPath));
  REQUIRE(dataStructure.getDataAs<DataArray<int32>>(copyPath) == nullptr);
  REQUIRE(sourceStore->m_BulkReadCalls == 1);
}
