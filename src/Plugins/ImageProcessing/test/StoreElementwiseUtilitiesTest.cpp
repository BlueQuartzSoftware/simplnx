#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"

#include <catch2/catch.hpp>

#include <atomic>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
template <class T>
DataStore<T> MakeStore(const std::vector<T>& values)
{
  DataStore<T> store(ShapeType{1, 1, values.size()}, ShapeType{1}, T{0});
  for(usize index = 0; index < values.size(); ++index)
  {
    store.setValue(index, values[index]);
  }
  return store;
}

template <class T>
class TransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_ReadValues += buffer.size();
    m_Reads.emplace_back(startIndex, buffer.size());
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_WrittenValues += buffer.size();
    m_Writes.emplace_back(startIndex, buffer.size());
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize readValues() const noexcept
  {
    return m_ReadValues;
  }

  usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

  const std::vector<std::pair<usize, usize>>& reads() const noexcept
  {
    return m_Reads;
  }

  const std::vector<std::pair<usize, usize>>& writes() const noexcept
  {
    return m_Writes;
  }

private:
  mutable usize m_ReadValues = 0;
  usize m_WrittenValues = 0;
  mutable std::vector<std::pair<usize, usize>> m_Reads;
  std::vector<std::pair<usize, usize>> m_Writes;
};

template <class T>
void SetValues(AbstractDataStore<T>& store, const std::vector<T>& values)
{
  for(usize index = 0; index < values.size(); ++index)
  {
    store.setValue(index, values[index]);
  }
}

template <class T>
std::vector<T> ReadAll(const AbstractDataStore<T>& store)
{
  std::vector<T> values(store.getSize());
  for(usize index = 0; index < values.size(); ++index)
  {
    values[index] = store.getValue(index);
  }
  return values;
}

template <class T>
void MakeSubtractValues(usize size, std::vector<T>& aValues, std::vector<T>& bValues)
{
  aValues.resize(size);
  bValues.resize(size);
  uint32 state = 0x12345678U;
  for(usize index = 0; index < size; ++index)
  {
    state = state * 1664525U + 1013904223U;
    if constexpr(std::is_floating_point_v<T>)
    {
      aValues[index] = static_cast<T>(static_cast<int32>(state % 2001U) - 1000) / T{17};
      state = state * 1664525U + 1013904223U;
      bValues[index] = static_cast<T>(static_cast<int32>(state % 2001U) - 1000) / T{19};
    }
    else
    {
      aValues[index] = static_cast<T>(state >> 16U);
      state = state * 1664525U + 1013904223U;
      bValues[index] = static_cast<T>(state >> 16U);
    }
  }
  if(size > 0)
  {
    aValues[0] = T{3};
    bValues[0] = T{5};
  }
  if constexpr(std::is_floating_point_v<T>)
  {
    if(size > 1)
    {
      aValues[1] = T{-7.5};
      bValues[1] = T{2.25};
    }
  }
}

template <class T>
std::vector<T> SubtractOracle(const std::vector<T>& aValues, const std::vector<T>& bValues)
{
  std::vector<T> expected(aValues.size());
  for(usize index = 0; index < expected.size(); ++index)
  {
    expected[index] = static_cast<T>(aValues[index] - bValues[index]);
  }
  return expected;
}
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::StoreElementwise: SubtractStores matches the serial difference on resident stores", "[ImageProcessing][StoreElementwise]", uint8, int16, float32)
{
  using T = TestType;
  std::atomic_bool shouldCancel{false};

  for(const usize size : {usize{0}, usize{1}, usize{7}, usize{100003}})
  {
    std::vector<T> aValues;
    std::vector<T> bValues;
    MakeSubtractValues(size, aValues, bValues);
    const std::vector<T> expected = SubtractOracle(aValues, bValues);

    DYNAMIC_SECTION("distinct output, size " << size)
    {
      DataStore<T> aStore = MakeStore(aValues);
      DataStore<T> bStore = MakeStore(bValues);
      DataStore<T> outStore(ShapeType{1, 1, size}, ShapeType{1}, T{77});

      const Result<> result = SubtractStores<T>(aStore, bStore, outStore, shouldCancel);
      REQUIRE(result.valid());
      REQUIRE(ReadAll<T>(outStore) == expected);
    }

    DYNAMIC_SECTION("output aliases a, size " << size)
    {
      DataStore<T> aStore = MakeStore(aValues);
      DataStore<T> bStore = MakeStore(bValues);

      const Result<> result = SubtractStores<T>(aStore, bStore, aStore, shouldCancel);
      REQUIRE(result.valid());
      REQUIRE(ReadAll<T>(aStore) == expected);
    }

    DYNAMIC_SECTION("output aliases b, size " << size)
    {
      DataStore<T> aStore = MakeStore(aValues);
      DataStore<T> bStore = MakeStore(bValues);

      const Result<> result = SubtractStores<T>(aStore, bStore, bStore, shouldCancel);
      REQUIRE(result.valid());
      REQUIRE(ReadAll<T>(bStore) == expected);
    }
  }
}

TEST_CASE("ImageProcessing::StoreElementwise: SubtractStores chunked path crosses chunk boundaries", "[ImageProcessing][StoreElementwise]")
{
  constexpr usize k_ChunkValues = 8;
  std::atomic_bool shouldCancel{false};

  SECTION("partial tail")
  {
    constexpr usize k_Size = 21;
    const std::vector<std::pair<usize, usize>> expectedTransfers{{0, 8}, {8, 8}, {16, 5}};
    std::vector<int32> aValues;
    std::vector<int32> bValues;
    MakeSubtractValues(k_Size, aValues, bValues);
    const std::vector<int32> expected = SubtractOracle(aValues, bValues);
    TransferCountingDataStore<int32> aStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{0});
    TransferCountingDataStore<int32> bStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{0});
    TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{77});
    SetValues<int32>(aStore, aValues);
    SetValues<int32>(bStore, bValues);

    const Result<> result = ImageProcessing::detail::SubtractStoresChunked<int32>(aStore, bStore, outStore, shouldCancel, k_ChunkValues);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == expected);
    REQUIRE(aStore.readValues() == k_Size);
    REQUIRE(bStore.readValues() == k_Size);
    REQUIRE(outStore.writtenValues() == k_Size);
    REQUIRE(aStore.reads() == expectedTransfers);
    REQUIRE(bStore.reads() == expectedTransfers);
    REQUIRE(outStore.writes() == expectedTransfers);
  }

  SECTION("chunk of seven divides the input into three full chunks")
  {
    constexpr usize k_Size = 21;
    constexpr usize k_SevenChunkValues = 7;
    const std::vector<std::pair<usize, usize>> expectedTransfers{{0, 7}, {7, 7}, {14, 7}};
    std::vector<int32> aValues;
    std::vector<int32> bValues;
    MakeSubtractValues(k_Size, aValues, bValues);
    const std::vector<int32> expected = SubtractOracle(aValues, bValues);
    TransferCountingDataStore<int32> aStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{0});
    TransferCountingDataStore<int32> bStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{0});
    TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{77});
    SetValues<int32>(aStore, aValues);
    SetValues<int32>(bStore, bValues);

    const Result<> result = ImageProcessing::detail::SubtractStoresChunked<int32>(aStore, bStore, outStore, shouldCancel, k_SevenChunkValues);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == expected);
    REQUIRE(aStore.reads() == expectedTransfers);
    REQUIRE(bStore.reads() == expectedTransfers);
    REQUIRE(outStore.writes() == expectedTransfers);
  }

  SECTION("zero chunk values clamp to one")
  {
    constexpr usize k_Size = 5;
    const std::vector<std::pair<usize, usize>> expectedTransfers{{0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}};
    std::vector<int32> aValues;
    std::vector<int32> bValues;
    MakeSubtractValues(k_Size, aValues, bValues);
    const std::vector<int32> expected = SubtractOracle(aValues, bValues);
    TransferCountingDataStore<int32> aStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{0});
    TransferCountingDataStore<int32> bStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{0});
    TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{77});
    SetValues<int32>(aStore, aValues);
    SetValues<int32>(bStore, bValues);

    const Result<> result = ImageProcessing::detail::SubtractStoresChunked<int32>(aStore, bStore, outStore, shouldCancel, 0);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == expected);
    REQUIRE(aStore.reads() == expectedTransfers);
    REQUIRE(bStore.reads() == expectedTransfers);
    REQUIRE(outStore.writes() == expectedTransfers);
  }

  SECTION("exact multiple")
  {
    constexpr usize k_Size = 16;
    std::vector<int32> aValues;
    std::vector<int32> bValues;
    MakeSubtractValues(k_Size, aValues, bValues);
    const std::vector<int32> expected = SubtractOracle(aValues, bValues);
    DataStore<int32> aStore = MakeStore(aValues);
    DataStore<int32> bStore = MakeStore(bValues);
    DataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{77});

    const Result<> result = ImageProcessing::detail::SubtractStoresChunked<int32>(aStore, bStore, outStore, shouldCancel, k_ChunkValues);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == expected);
  }

  SECTION("single partial chunk")
  {
    constexpr usize k_Size = 5;
    std::vector<int32> aValues;
    std::vector<int32> bValues;
    MakeSubtractValues(k_Size, aValues, bValues);
    const std::vector<int32> expected = SubtractOracle(aValues, bValues);
    DataStore<int32> aStore = MakeStore(aValues);
    DataStore<int32> bStore = MakeStore(bValues);
    DataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{77});

    const Result<> result = ImageProcessing::detail::SubtractStoresChunked<int32>(aStore, bStore, outStore, shouldCancel, k_ChunkValues);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == expected);
  }

  SECTION("output aliases a across a partial tail")
  {
    constexpr usize k_Size = 21;
    std::vector<int32> aValues;
    std::vector<int32> bValues;
    MakeSubtractValues(k_Size, aValues, bValues);
    const std::vector<int32> expected = SubtractOracle(aValues, bValues);
    DataStore<int32> aStore = MakeStore(aValues);
    DataStore<int32> bStore = MakeStore(bValues);

    const Result<> result = ImageProcessing::detail::SubtractStoresChunked<int32>(aStore, bStore, aStore, shouldCancel, k_ChunkValues);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(aStore) == expected);
  }
}

TEST_CASE("ImageProcessing::StoreElementwise: resident SubtractStores performs no bulk transfers", "[ImageProcessing][StoreElementwise]")
{
  constexpr usize k_Size = 4097;
  std::vector<float32> aValues;
  std::vector<float32> bValues;
  MakeSubtractValues(k_Size, aValues, bValues);
  const std::vector<float32> expected = SubtractOracle(aValues, bValues);
  TransferCountingDataStore<float32> aStore(ShapeType{1, 1, k_Size}, ShapeType{1}, 0.0f);
  TransferCountingDataStore<float32> bStore(ShapeType{1, 1, k_Size}, ShapeType{1}, 0.0f);
  TransferCountingDataStore<float32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, 77.0f);
  SetValues<float32>(aStore, aValues);
  SetValues<float32>(bStore, bValues);
  std::atomic_bool shouldCancel{false};

  const Result<> result = SubtractStores<float32>(aStore, bStore, outStore, shouldCancel);
  REQUIRE(result.valid());
  REQUIRE(ReadAll<float32>(outStore) == expected);
  REQUIRE(aStore.readValues() == 0);
  REQUIRE(aStore.writtenValues() == 0);
  REQUIRE(bStore.readValues() == 0);
  REQUIRE(bStore.writtenValues() == 0);
  REQUIRE(outStore.readValues() == 0);
  REQUIRE(outStore.writtenValues() == 0);
}

TEST_CASE("ImageProcessing::StoreElementwise: resident InvertBinaryStore performs no bulk transfers", "[ImageProcessing][StoreElementwise]")
{
  constexpr usize k_Size = 4097;
  std::vector<int32> inputValues(k_Size);
  std::vector<int32> expected(k_Size);
  for(usize index = 0; index < k_Size; ++index)
  {
    inputValues[index] = index % 5 == 0 ? int32{0} : static_cast<int32>(index) - 2048;
    expected[index] = inputValues[index] == 0 ? int32{1} : int32{0};
  }
  TransferCountingDataStore<int32> inputStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{0});
  TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{77});
  SetValues<int32>(inputStore, inputValues);
  std::atomic_bool shouldCancel{false};

  const Result<> result = InvertBinaryStore<int32>(inputStore, outStore, shouldCancel);
  REQUIRE(result.valid());
  REQUIRE(ReadAll<int32>(outStore) == expected);
  REQUIRE(inputStore.readValues() == 0);
  REQUIRE(outStore.writtenValues() == 0);
}

TEST_CASE("ImageProcessing::StoreElementwise: InvertBinaryStore chunked path uses the requested chunk geometry", "[ImageProcessing][StoreElementwise]")
{
  std::atomic_bool shouldCancel{false};

  SECTION("partial tail")
  {
    constexpr usize k_Size = 21;
    const std::vector<std::pair<usize, usize>> expectedTransfers{{0, 8}, {8, 8}, {16, 5}};
    std::vector<int32> inputValues(k_Size);
    std::vector<int32> expected(k_Size);
    for(usize index = 0; index < k_Size; ++index)
    {
      inputValues[index] = index % 3 == 0 ? int32{0} : static_cast<int32>(index) - 7;
      expected[index] = inputValues[index] == 0 ? int32{1} : int32{0};
    }
    TransferCountingDataStore<int32> inputStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{0});
    TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{77});
    SetValues<int32>(inputStore, inputValues);

    const Result<> result = ImageProcessing::detail::InvertBinaryStoreChunked<int32>(inputStore, outStore, shouldCancel, 8);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == expected);
    REQUIRE(inputStore.reads() == expectedTransfers);
    REQUIRE(outStore.writes() == expectedTransfers);
  }

  SECTION("zero chunk values clamp to one")
  {
    constexpr usize k_Size = 5;
    const std::vector<std::pair<usize, usize>> expectedTransfers{{0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}};
    const std::vector<int32> inputValues{0, 4, 0, -2, 9};
    const std::vector<int32> expected{1, 0, 1, 0, 0};
    TransferCountingDataStore<int32> inputStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{0});
    TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{77});
    SetValues<int32>(inputStore, inputValues);

    const Result<> result = ImageProcessing::detail::InvertBinaryStoreChunked<int32>(inputStore, outStore, shouldCancel, 0);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == expected);
    REQUIRE(inputStore.reads() == expectedTransfers);
    REQUIRE(outStore.writes() == expectedTransfers);
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::StoreElementwise: InvertBinaryStore maps nonzero to zero and zero to one", "[ImageProcessing][StoreElementwise]", uint8, int32)
{
  using T = TestType;
  std::atomic_bool shouldCancel{false};
  std::vector<T> baseValues{T{0}, T{1}, T{7}, std::numeric_limits<T>::max()};
  if constexpr(std::is_same_v<T, int32>)
  {
    baseValues.push_back(T{-3});
  }

  for(const usize size : {usize{0}, usize{1}, usize{21}, usize{100003}})
  {
    std::vector<T> inputValues(size);
    std::vector<T> expected(size);
    for(usize index = 0; index < size; ++index)
    {
      inputValues[index] = baseValues[index % baseValues.size()];
      expected[index] = inputValues[index] != T{0} ? T{0} : T{1};
    }

    DYNAMIC_SECTION("resident output, size " << size)
    {
      DataStore<T> inputStore = MakeStore(inputValues);
      DataStore<T> outputStore(ShapeType{1, 1, size}, ShapeType{1}, T{77});

      const Result<> result = InvertBinaryStore<T>(inputStore, outputStore, shouldCancel);
      REQUIRE(result.valid());
      REQUIRE(ReadAll<T>(outputStore) == expected);
    }

    DYNAMIC_SECTION("resident output aliases input, size " << size)
    {
      DataStore<T> inputStore = MakeStore(inputValues);

      const Result<> result = InvertBinaryStore<T>(inputStore, inputStore, shouldCancel);
      REQUIRE(result.valid());
      REQUIRE(ReadAll<T>(inputStore) == expected);
    }

    if(size == 21)
    {
      DYNAMIC_SECTION("chunked output, size " << size)
      {
        DataStore<T> inputStore = MakeStore(inputValues);
        DataStore<T> outputStore(ShapeType{1, 1, size}, ShapeType{1}, T{77});

        const Result<> result = ImageProcessing::detail::InvertBinaryStoreChunked<T>(inputStore, outputStore, shouldCancel, 8);
        REQUIRE(result.valid());
        REQUIRE(ReadAll<T>(outputStore) == expected);
      }

      DYNAMIC_SECTION("chunked output aliases input, size " << size)
      {
        DataStore<T> inputStore = MakeStore(inputValues);

        const Result<> result = ImageProcessing::detail::InvertBinaryStoreChunked<T>(inputStore, inputStore, shouldCancel, 8);
        REQUIRE(result.valid());
        REQUIRE(ReadAll<T>(inputStore) == expected);
      }
    }
  }
}

TEST_CASE("ImageProcessing::StoreElementwise: pre-cancelled helpers read and write nothing", "[ImageProcessing][StoreElementwise]")
{
  constexpr usize k_Size = 21;
  constexpr int32 k_Poison = 77;
  const std::vector<int32> poisonValues(k_Size, k_Poison);
  std::atomic_bool shouldCancel{true};

  SECTION("resident subtraction")
  {
    TransferCountingDataStore<int32> aStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{3});
    TransferCountingDataStore<int32> bStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{5});
    TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, k_Poison);

    const Result<> result = SubtractStores<int32>(aStore, bStore, outStore, shouldCancel);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == poisonValues);
    REQUIRE(aStore.readValues() == 0);
    REQUIRE(aStore.writtenValues() == 0);
    REQUIRE(bStore.readValues() == 0);
    REQUIRE(bStore.writtenValues() == 0);
    REQUIRE(outStore.readValues() == 0);
    REQUIRE(outStore.writtenValues() == 0);
  }

  SECTION("chunked subtraction")
  {
    TransferCountingDataStore<int32> aStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{3});
    TransferCountingDataStore<int32> bStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{5});
    TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, k_Poison);

    const Result<> result = ImageProcessing::detail::SubtractStoresChunked<int32>(aStore, bStore, outStore, shouldCancel, 8);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == poisonValues);
    REQUIRE(aStore.readValues() == 0);
    REQUIRE(aStore.writtenValues() == 0);
    REQUIRE(bStore.readValues() == 0);
    REQUIRE(bStore.writtenValues() == 0);
    REQUIRE(outStore.readValues() == 0);
    REQUIRE(outStore.writtenValues() == 0);
  }

  SECTION("resident invert")
  {
    TransferCountingDataStore<int32> inputStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{3});
    TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, k_Poison);

    const Result<> result = InvertBinaryStore<int32>(inputStore, outStore, shouldCancel);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == poisonValues);
    REQUIRE(inputStore.readValues() == 0);
    REQUIRE(inputStore.writtenValues() == 0);
    REQUIRE(outStore.readValues() == 0);
    REQUIRE(outStore.writtenValues() == 0);
  }

  SECTION("chunked invert")
  {
    TransferCountingDataStore<int32> inputStore(ShapeType{1, 1, k_Size}, ShapeType{1}, int32{3});
    TransferCountingDataStore<int32> outStore(ShapeType{1, 1, k_Size}, ShapeType{1}, k_Poison);

    const Result<> result = ImageProcessing::detail::InvertBinaryStoreChunked<int32>(inputStore, outStore, shouldCancel, 8);
    REQUIRE(result.valid());
    REQUIRE(ReadAll<int32>(outStore) == poisonValues);
    REQUIRE(inputStore.readValues() == 0);
    REQUIRE(inputStore.writtenValues() == 0);
    REQUIRE(outStore.readValues() == 0);
    REQUIRE(outStore.writtenValues() == 0);
  }
}

TEST_CASE("ImageProcessing::StoreElementwise: resident and chunked paths agree", "[ImageProcessing][StoreElementwise]")
{
  constexpr usize k_Size = 4097;
  std::mt19937 generator(0x5A17U);
  std::uniform_int_distribution<int32> distribution(std::numeric_limits<int16>::lowest(), std::numeric_limits<int16>::max());
  std::vector<int16> aValues(k_Size);
  std::vector<int16> bValues(k_Size);
  for(usize index = 0; index < k_Size; ++index)
  {
    aValues[index] = static_cast<int16>(distribution(generator));
    bValues[index] = static_cast<int16>(distribution(generator));
    if(index % 13 == 0)
    {
      aValues[index] = int16{0};
    }
  }

  std::atomic_bool shouldCancel{false};
  DataStore<int16> aStore = MakeStore(aValues);
  DataStore<int16> bStore = MakeStore(bValues);
  DataStore<int16> residentSubtract(ShapeType{1, 1, k_Size}, ShapeType{1}, int16{77});
  DataStore<int16> chunkedSubtract(ShapeType{1, 1, k_Size}, ShapeType{1}, int16{77});
  REQUIRE(SubtractStores<int16>(aStore, bStore, residentSubtract, shouldCancel).valid());
  REQUIRE(ImageProcessing::detail::SubtractStoresChunked<int16>(aStore, bStore, chunkedSubtract, shouldCancel, 1000).valid());
  REQUIRE(ReadAll<int16>(chunkedSubtract) == ReadAll<int16>(residentSubtract));

  DataStore<int16> residentInvert(ShapeType{1, 1, k_Size}, ShapeType{1}, int16{77});
  DataStore<int16> chunkedInvert(ShapeType{1, 1, k_Size}, ShapeType{1}, int16{77});
  REQUIRE(InvertBinaryStore<int16>(aStore, residentInvert, shouldCancel).valid());
  REQUIRE(ImageProcessing::detail::InvertBinaryStoreChunked<int16>(aStore, chunkedInvert, shouldCancel, 1000).valid());
  REQUIRE(ReadAll<int16>(chunkedInvert) == ReadAll<int16>(residentInvert));
}
