#pragma once

#include "simplnx/Common/Bit.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

#include <fmt/core.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{

/**
 * @class DataStore
 * @brief Stores typed values in contiguous in-memory storage.
 * @tparam T Stored value type.
 */
template <typename T>
class DataStore : public AbstractDataStore<T>
{
public:
  /**
   * @brief Names the abstract data-store base type.
   */
  using parent_type = AbstractDataStore<T>;

  /**
   * @brief Names the stored value type.
   */
  using value_type = typename AbstractDataStore<T>::value_type;

  /**
   * @brief Names the mutable value-proxy type.
   */
  using reference = typename AbstractDataStore<T>::reference;

  /**
   * @brief Names the DataStore record type.
   */
  static constexpr const char k_DataStore[] = "DataStore";

  /**
   * @brief Names the data-object identifier record.
   */
  static constexpr const char k_DataObjectId[] = "DataObjectId";

  /**
   * @brief Names the data-array type record.
   */
  static constexpr const char k_DataArrayTypeName[] = "DataArray";

  /**
   * @brief Creates a one-component data store.
   * @param numTuples Number of tuples.
   * @param initValue Optional value that initializes every element.
   */
  DataStore(usize numTuples, std::optional<T> initValue)
  : DataStore({numTuples}, {1}, initValue)
  {
  }

  /**
   * @brief Creates a data store with a tuple and component shape.
   * @param tupleShape Tuple dimensions in slowest-to-fastest order.
   * @param componentShape Component dimensions in slowest-to-fastest order.
   * @param initValue Optional value that initializes every element.
   */
  DataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue)
  : parent_type()
  , m_ComponentShape(componentShape)
  , m_TupleShape(tupleShape)
  , m_NumComponents(std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  , m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  , m_InitValue(initValue)
  {
    resizeTuples(m_TupleShape);
    if(m_InitValue.has_value())
    {
      std::fill_n(data(), this->getSize(), *m_InitValue);
    }
  }

  /**
   * @brief Takes ownership of an existing value buffer.
   *
   * The buffer must hold getSize() values for the supplied shapes. Future growth
   * uses a mudflap value because the constructor has no initialization value.
   * @param buffer Owning contiguous value buffer.
   * @param tupleShape Tuple dimensions in slowest-to-fastest order.
   * @param componentShape Component dimensions in slowest-to-fastest order.
   */
  DataStore(std::unique_ptr<value_type[]> buffer, ShapeType tupleShape, ShapeType componentShape)
  : parent_type()
  , m_ComponentShape(std::move(componentShape))
  , m_TupleShape(std::move(tupleShape))
  , m_Data(std::move(buffer))
  , m_NumComponents(std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  , m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>()))
  {
    // Future growth needs a diagnostic value because the supplied buffer has no initialization value.
    m_InitValue = GetMudflap<T>();
  }

  /**
   * @brief Copies a data store and its values.
   * @param other Source data store.
   */
  DataStore(const DataStore& other)
  : parent_type()
  , m_ComponentShape(other.m_ComponentShape)
  , m_TupleShape(other.m_TupleShape)
  , m_NumComponents(other.m_NumComponents)
  , m_NumTuples(other.m_NumTuples)
  , m_InitValue(other.m_InitValue)
  {
    const usize count = other.getSize();
    auto* data = new value_type[count];
    std::memcpy(data, other.m_Data.get(), count * sizeof(T));
    m_Data.reset(data);
  }

  /**
   * @brief Moves a data store and its values.
   * @param other Source data store.
   */
  DataStore(DataStore&& other) noexcept
  : parent_type()
  , m_ComponentShape(std::move(other.m_ComponentShape))
  , m_TupleShape(std::move(other.m_TupleShape))
  , m_Data(std::move(other.m_Data))
  , m_NumComponents(std::move(other.m_NumComponents))
  , m_NumTuples(std::move(other.m_NumTuples))
  , m_InitValue(other.m_InitValue)
  {
  }

  DataStore& operator=(const DataStore& rhs) = delete;

  /**
   * @brief Moves data-store state into this store.
   * @param rhs Source data store.
   * @return This data store.
   */
  DataStore& operator=(DataStore&& rhs)
  {
    m_ComponentShape = std::move(rhs.m_ComponentShape);
    m_TupleShape = std::move(rhs.m_TupleShape);
    m_Data = std::move(rhs.m_Data);
    m_NumComponents = rhs.m_NumComponents;
    m_NumTuples = rhs.m_NumTuples;
    m_InitValue = std::move(rhs.m_InitValue);
    return *this;
  }

  /**
   * @brief Destroys the data store.
   */
  ~DataStore() override = default;

  usize getNumberOfTuples() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief Returns the contiguous value buffer.
   * @return Pointer valid until this store is resized, moved, or destroyed.
   */
  const T* data() const
  {
    return m_Data.get();
  }

  /**
   * @brief Returns the contiguous mutable value buffer.
   * @return Pointer valid until this store is resized, moved, or destroyed.
   */
  T* data()
  {
    return m_Data.get();
  }

  usize getNumberOfComponents() const override
  {
    return m_NumComponents;
  }

  const ShapeType& getTupleShape() const override
  {
    return m_TupleShape;
  }

  const ShapeType& getComponentShape() const override
  {
    return m_ComponentShape;
  }

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::InMemory;
  }

  /**
   * @brief Returns no recovery metadata.
   * @return Empty metadata map because recovery stores in-memory values.
   */
  std::map<std::string, std::string> getRecoveryMetadata() const override
  {
    return {};
  }

  /**
   * @brief Sets the value used when the store grows.
   * @param value Value that initializes new elements.
   */
  void setInitValue(T value)
  {
    m_InitValue = value;
  }

  /**
   * @brief Changes the tuple shape.
   *
   * A size change retains values in the shared prefix. When existing storage
   * grows, added values use the initialization or mudflap value. A size change can invalidate pointers and spans.
   * @param tupleShape New tuple dimensions in slowest-to-fastest order.
   */
  void resizeTuples(const ShapeType& tupleShape) override
  {
    auto oldSize = this->getSize();
    m_TupleShape = tupleShape;
    m_NumTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>());

    usize newSize = getNumberOfComponents() * m_NumTuples;

    if(m_Data.get() == nullptr)
    {
      auto data = new value_type[newSize];
      m_Data.reset(data);
      return;
    }

    // Matching value counts change shape metadata without reallocating storage.
    if(newSize == oldSize)
    {
      return;
    }

    auto data = new value_type[newSize];
    for(usize i = 0; i < newSize && i < oldSize; i++)
    {
      data[i] = m_Data.get()[i];
    }

    // New values use the configured initialization value.
    T initValue = m_InitValue.has_value() ? *m_InitValue : GetMudflap<T>();
    for(usize i = oldSize; i < newSize; i++)
    {
      data[i] = initValue;
    }

    m_Data.reset(data);
  }

  value_type getValue(usize index) const override
  {
    return m_Data.get()[index];
  }

  /**
   * @brief Stores a value at a flat index.
   * @param index Flat value index.
   * @param value Value to store.
   */
  void setValue(usize index, value_type value) override
  {
    m_Data.get()[index] = value;
  }

  /**
   * @brief Copies contiguous values into caller-owned storage.
   *
   * This is the in-memory fast path for storage-neutral bulk I/O.
   * @param startIndex First flat value index to read.
   * @param buffer Receives copied values.
   * @return Error if the requested range exceeds this store.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    const usize count = buffer.size();

    if(startIndex + count > this->getSize())
    {
      return MakeErrorResult(-6020, fmt::format("DataStore bulk read failed: requested range [{}, {}) exceeds store size ({}). Requested {} elements starting at index {}.", startIndex,
                                                startIndex + count, this->getSize(), count, startIndex));
    }

    std::copy(m_Data.get() + startIndex, m_Data.get() + startIndex + count, buffer.data());
    return {};
  }

  /**
   * @brief Copies caller-owned values into contiguous storage.
   *
   * This is the in-memory fast path for storage-neutral bulk I/O.
   * @param startIndex First flat value index to write.
   * @param buffer Values to copy.
   * @return Error if the requested range exceeds this store.
   */
  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    const usize count = buffer.size();

    if(startIndex + count > this->getSize())
    {
      return MakeErrorResult(-6021, fmt::format("DataStore bulk write failed: requested range [{}, {}) exceeds store size ({}). Requested {} elements starting at index {}.", startIndex,
                                                startIndex + count, this->getSize(), count, startIndex));
    }

    std::copy(buffer.begin(), buffer.end(), m_Data.get() + startIndex);
    return {};
  }

  /**
   * @brief Reads a tuple-space extent into a new value vector.
   *
   * The 3D path copies contiguous X rows. Strided and other dimensions use
   * index mapping. Boolean output writes packed vector values without a temporary.
   * @param extent Tuple-space extent with minimum, maximum, and stride values.
   * @return Extent values in row-major, component-fastest order, or an empty vector when invalid.
   */
  std::vector<T> readExtent(const Extent& extent) const override
  {
    const ShapeType& tupleShape = getTupleShape();
    const usize tupleDimensions = tupleShape.size();

    if(extent.dimensions() != tupleDimensions)
    {
      return {};
    }
    for(usize dimension = 0; dimension < tupleDimensions; ++dimension)
    {
      if(extent.stride[dimension] == 0 || extent.min[dimension] > extent.max[dimension] || extent.max[dimension] >= tupleShape[dimension])
      {
        return {};
      }
    }

    const usize totalValues = static_cast<usize>(extent.totalElements()) * getNumberOfComponents();
    std::vector<T> result(totalValues);
    if constexpr(std::is_same_v<T, bool>)
    {
      // Packed vector<bool> storage requires direct proxy writes.
      const usize numComponents = getNumberOfComponents();
      const usize outputTupleCount = static_cast<usize>(extent.totalElements());
      for(usize outputTupleIndex = 0; outputTupleIndex < outputTupleCount; ++outputTupleIndex)
      {
        uint64 remainingOutputIndex = static_cast<uint64>(outputTupleIndex);
        uint64 sourceTupleIndex = 0;
        uint64 sourceDimensionStride = 1;
        for(usize dimension = tupleDimensions; dimension-- > 0;)
        {
          const uint64 extentDimensionSize = extent.size(dimension);
          const uint64 extentCoordinate = remainingOutputIndex % extentDimensionSize;
          remainingOutputIndex /= extentDimensionSize;
          const uint64 sourceCoordinate = extent.min[dimension] + extentCoordinate * extent.stride[dimension];
          sourceTupleIndex += sourceCoordinate * sourceDimensionStride;
          sourceDimensionStride *= static_cast<uint64>(tupleShape[dimension]);
        }

        const usize sourceValueIndex = static_cast<usize>(sourceTupleIndex) * numComponents;
        const usize destinationValueIndex = outputTupleIndex * numComponents;
        for(usize componentIndex = 0; componentIndex < numComponents; ++componentIndex)
        {
          result[destinationValueIndex + componentIndex] = m_Data[sourceValueIndex + componentIndex];
        }
      }
      return result;
    }
    else
    {
      readExtentIntoBuffer(extent, nonstd::span<T>(result.data(), result.size()));
      return result;
    }
  }

  /**
   * @brief Reads a tuple-space extent into caller-owned storage.
   *
   * The 3D path copies contiguous X rows. Other layouts use index mapping.
   * @param extent Tuple-space extent with minimum, maximum, and stride values.
   * @param destination Receives exactly `extent.totalElements() * getNumberOfComponents()` values.
   * @throws std::invalid_argument If the extent or destination size is invalid.
   */
  void readExtentIntoBuffer(const Extent& extent, nonstd::span<T> destination) const override
  {
    const ShapeType& tupleShape = getTupleShape();
    const usize tupleDimensions = tupleShape.size();
    const usize numComponents = getNumberOfComponents();

    if(extent.dimensions() != tupleDimensions)
    {
      throw std::invalid_argument(fmt::format("DataStore::readExtentIntoBuffer: extent dimensions ({}) do not match tuple-shape dimensions ({})", extent.dimensions(), tupleDimensions));
    }
    for(usize dimension = 0; dimension < tupleDimensions; ++dimension)
    {
      if(extent.stride[dimension] == 0 || extent.min[dimension] > extent.max[dimension] || extent.max[dimension] >= tupleShape[dimension])
      {
        throw std::invalid_argument(fmt::format("DataStore::readExtentIntoBuffer: extent dimension {} has min {}, max {}, stride {}, and tuple bound {}", dimension, extent.min[dimension],
                                                extent.max[dimension], extent.stride[dimension], tupleShape[dimension]));
      }
    }

    const usize requiredValues = static_cast<usize>(extent.totalElements()) * numComponents;
    if(destination.size() != requiredValues)
    {
      throw std::invalid_argument(fmt::format("DataStore::readExtentIntoBuffer: destination has {} values; expected {}", destination.size(), requiredValues));
    }
    if(requiredValues == 0 || m_Data.get() == nullptr)
    {
      return;
    }

    const T* source = m_Data.get();
    if(tupleDimensions == 3)
    {
      const uint64 dimY = tupleShape[1];
      const uint64 dimX = tupleShape[2];
      const uint64 xMin = extent.min[2];
      const uint64 xStride = extent.stride[2];
      const uint64 xCount = extent.size(2);

      usize outputIndex = 0;
      for(uint64 z = extent.min[0]; z <= extent.max[0]; z += extent.stride[0])
      {
        for(uint64 y = extent.min[1]; y <= extent.max[1]; y += extent.stride[1])
        {
          const usize sourceRowStart = static_cast<usize>((z * dimY + y) * dimX + xMin) * numComponents;
          if constexpr(!std::is_same_v<T, bool>)
          {
            if(xStride == 1)
            {
              const usize rowValues = static_cast<usize>(xCount) * numComponents;
              std::memcpy(destination.data() + outputIndex, source + sourceRowStart, rowValues * sizeof(T));
              outputIndex += rowValues;
              continue;
            }
          }

          for(uint64 xIndex = 0; xIndex < xCount; ++xIndex)
          {
            const usize sourceOffset = sourceRowStart + static_cast<usize>(xIndex * xStride) * numComponents;
            for(usize componentIndex = 0; componentIndex < numComponents; ++componentIndex)
            {
              destination[outputIndex + componentIndex] = source[sourceOffset + componentIndex];
            }
            outputIndex += numComponents;
          }
        }
      }
      return;
    }

    if(tupleDimensions == 1)
    {
      const uint64 startTuple = extent.min[0];
      const uint64 tupleCount = extent.size(0);
      const uint64 tupleStride = extent.stride[0];
      usize outputIndex = 0;
      for(uint64 tupleOffset = 0; tupleOffset < tupleCount; ++tupleOffset)
      {
        const usize sourceIndex = static_cast<usize>(startTuple + tupleOffset * tupleStride) * numComponents;
        for(usize componentIndex = 0; componentIndex < numComponents; ++componentIndex)
        {
          destination[outputIndex + componentIndex] = source[sourceIndex + componentIndex];
        }
        outputIndex += numComponents;
      }
      return;
    }

    const usize outputTupleCount = static_cast<usize>(extent.totalElements());
    for(usize outputTupleIndex = 0; outputTupleIndex < outputTupleCount; ++outputTupleIndex)
    {
      uint64 remainingOutputIndex = static_cast<uint64>(outputTupleIndex);
      uint64 sourceTupleIndex = 0;
      uint64 sourceDimensionStride = 1;
      for(usize dimension = tupleDimensions; dimension-- > 0;)
      {
        const uint64 extentDimensionSize = extent.size(dimension);
        const uint64 extentCoordinate = remainingOutputIndex % extentDimensionSize;
        remainingOutputIndex /= extentDimensionSize;
        const uint64 sourceCoordinate = extent.min[dimension] + extentCoordinate * extent.stride[dimension];
        sourceTupleIndex += sourceCoordinate * sourceDimensionStride;
        sourceDimensionStride *= static_cast<uint64>(tupleShape[dimension]);
      }

      const usize sourceValueIndex = static_cast<usize>(sourceTupleIndex) * numComponents;
      const usize destinationValueIndex = outputTupleIndex * numComponents;
      for(usize componentIndex = 0; componentIndex < numComponents; ++componentIndex)
      {
        destination[destinationValueIndex + componentIndex] = source[sourceValueIndex + componentIndex];
      }
    }
  }

  /**
   * @brief Writes values into a tuple-space extent.
   *
   * The 3D path copies contiguous X rows. The 1D path writes strided tuples.
   * Other dimensions leave this store unchanged. The span must contain at least
   * `extent.totalElements() * getNumberOfComponents()` values.
   * @param extent Tuple-space extent with minimum, maximum, and stride values.
   * @param data Values in row-major, component-fastest order.
   */
  void writeExtent(const Extent& extent, nonstd::span<const T> data) override
  {
    const ShapeType& tupleShape = getTupleShape();
    const usize tupleDims = tupleShape.size();
    const usize numComp = getNumberOfComponents();

    if(extent.dimensions() != tupleDims)
    {
      return;
    }
    for(usize d = 0; d < tupleDims; ++d)
    {
      if(extent.max[d] >= tupleShape[d])
      {
        return;
      }
    }

    const usize totalIn = static_cast<usize>(extent.totalElements()) * numComp;
    if(totalIn == 0 || data.size() < totalIn || m_Data.get() == nullptr)
    {
      return;
    }

    T* destBase = m_Data.get();

    if(tupleDims == 3)
    {
      const uint64 dimY = tupleShape[1];
      const uint64 dimX = tupleShape[2];
      const uint64 xMin = extent.min[2];
      const uint64 xStride = extent.stride[2];
      const uint64 xCountStrided = extent.size(2);

      usize srcIdx = 0;
      for(uint64 z = extent.min[0]; z <= extent.max[0]; z += extent.stride[0])
      {
        for(uint64 y = extent.min[1]; y <= extent.max[1]; y += extent.stride[1])
        {
          const usize destRowStart = static_cast<usize>((z * dimY + y) * dimX + xMin) * numComp;
          if constexpr(!std::is_same_v<T, bool>)
          {
            if(xStride == 1)
            {
              const usize elements = static_cast<usize>(xCountStrided) * numComp;
              std::memcpy(destBase + destRowStart, data.data() + srcIdx, elements * sizeof(T));
              srcIdx += elements;
              continue;
            }
          }
          // Strided and boolean values require tuple-by-tuple writes.
          for(uint64 xi = 0; xi < xCountStrided; ++xi)
          {
            const usize destOff = destRowStart + static_cast<usize>(xi * xStride) * numComp;
            for(usize c = 0; c < numComp; ++c)
            {
              destBase[destOff + c] = data[srcIdx + c];
            }
            srcIdx += numComp;
          }
        }
      }
      return;
    }

    if(tupleDims == 1)
    {
      const uint64 startTuple = extent.min[0];
      const uint64 count = extent.size(0);
      const uint64 stride = extent.stride[0];
      usize srcIdx = 0;
      for(uint64 i = 0; i < count; ++i)
      {
        const usize destIdx = static_cast<usize>(startTuple + i * stride) * numComp;
        for(usize c = 0; c < numComp; ++c)
        {
          destBase[destIdx + c] = data[srcIdx + c];
        }
        srcIdx += numComp;
      }
    }
  }

  /**
   * @brief Returns a value at a flat index.
   * @param index Flat value index.
   * @return Stored value.
   * @throws std::out_of_range If index is not valid.
   */
  value_type at(usize index) const override
  {
    if(index >= this->getSize())
    {
      throw std::out_of_range(fmt::format("DataStore::at({}) is out of bounds. Size={}", index, this->getSize()));
    }
    return m_Data.get()[index];
  }

  /**
   * @brief Adds a value at a flat index.
   * @param index Flat value index.
   * @param value Value to add.
   * @throws std::runtime_error If T is bool.
   */
  void add(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool>)
    {
      m_Data.get()[index] += value;
    }
    else
    {
      throw std::runtime_error("DataStore<bool>::add() invalid operator");
    }
  }

  /**
   * @brief Subtracts a value at a flat index.
   * @param index Flat value index.
   * @param value Value to subtract.
   * @throws std::runtime_error If T is bool.
   */
  void sub(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool>)
    {
      m_Data.get()[index] -= value;
    }
    else
    {
      throw std::runtime_error("DataStore<bool>::sub() invalid operator");
    }
  }

  /**
   * @brief Multiplies a value at a flat index.
   * @param index Flat value index.
   * @param value Multiplier.
   * @throws std::runtime_error If T is bool.
   */
  void mul(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool>)
    {
      m_Data.get()[index] *= value;
    }
    else
    {
      throw std::runtime_error("DataStore<bool>::mul() invalid operator");
    }
  }

  /**
   * @brief Divides a value at a flat index.
   * @param index Flat value index.
   * @param value Divisor.
   * @throws std::runtime_error If T is bool.
   */
  void div(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool>)
    {
      m_Data.get()[index] /= value;
    }
    else
    {
      throw std::runtime_error("DataStore<bool>::div() invalid operator");
    }
  }

  /**
   * @brief Replaces a value with its remainder.
   * @param index Flat value index.
   * @param value Divisor.
   * @throws std::runtime_error If T is bool or floating point.
   */
  void rem(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool> && !std::is_floating_point_v<T>)
    {
      m_Data.get()[index] %= value;
    }
    else
    {
      throw std::runtime_error("DataStore::rem() invalid operator for bool or floating point");
    }
  }

  /**
   * @brief Applies a bitwise AND at a flat index.
   * @param index Flat value index.
   * @param value Operand.
   * @throws std::runtime_error If T is bool or floating point.
   */
  void bitwiseAND(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool> && !std::is_floating_point_v<T>)
    {
      m_Data.get()[index] &= value;
    }
    else
    {
      throw std::runtime_error("DataStore::bitwiseAND() invalid operator for bool or floating point");
    }
  }

  /**
   * @brief Applies a bitwise OR at a flat index.
   * @param index Flat value index.
   * @param value Operand.
   * @throws std::runtime_error If T is bool or floating point.
   */
  void bitwiseOR(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool> && !std::is_floating_point_v<T>)
    {
      m_Data.get()[index] |= value;
    }
    else
    {
      throw std::runtime_error("DataStore::bitwiseOR() invalid operator for bool or floating point");
    }
  }

  /**
   * @brief Applies a bitwise XOR at a flat index.
   * @param index Flat value index.
   * @param value Operand.
   * @throws std::runtime_error If T is bool or floating point.
   */
  void bitwiseXOR(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool> && !std::is_floating_point_v<T>)
    {
      m_Data.get()[index] ^= value;
    }
    else
    {
      throw std::runtime_error("DataStore::bitwiseXOR() invalid operator for bool or floating point");
    }
  }

  /**
   * @brief Shifts a value left at a flat index.
   * @param index Flat value index.
   * @param value Shift count.
   * @throws std::runtime_error If T is bool or floating point.
   */
  void bitwiseLShift(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool> && !std::is_floating_point_v<T>)
    {
      m_Data.get()[index] <<= value;
    }
    else
    {
      throw std::runtime_error("DataStore::bitwiseLShift() invalid operator for bool or floating point");
    }
  }

  /**
   * @brief Shifts a value right at a flat index.
   * @param index Flat value index.
   * @param value Shift count.
   * @throws std::runtime_error If T is bool or floating point.
   */
  void bitwiseRShift(usize index, value_type value) override
  {
    if constexpr(!std::is_same_v<T, bool> && !std::is_floating_point_v<T>)
    {
      m_Data.get()[index] >>= value;
    }
    else
    {
      throw std::runtime_error("DataStore::bitwiseRShift() invalid operator for bool or floating point");
    }
  }

  /**
   * @brief Swaps the byte order of a value.
   * @param index Flat value index.
   */
  void byteSwap(usize index) override
  {
    T& element = m_Data.get()[index];
    element = nx::core::byteswap(element);
  }

  /**
   * @brief Swaps two values.
   * @param index1 First flat value index.
   * @param index2 Second flat value index.
   */
  void swap(usize index1, usize index2) override
  {
    std::swap(m_Data.get()[index1], m_Data.get()[index2]);
  }

  /**
   * @brief Makes an independent copy of this store.
   * @return Owning copy of this store.
   */
  std::unique_ptr<IDataStore> deepCopy() const override
  {
    return std::make_unique<DataStore<T>>(*this);
  }

  /**
   * @brief Creates an in-memory store with the same shapes.
   * @return Owning store initialized with zero values.
   */
  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return std::make_unique<DataStore<T>>(this->getTupleShape(), this->getComponentShape(), static_cast<T>(0));
  }

  /**
   * @brief Creates a mutable span over contiguous values.
   * @return Span valid until this store is resized, moved, or destroyed.
   */
  nonstd::span<T> createSpan()
  {
    return {data(), this->getSize()};
  }

  /**
   * @brief Creates a read-only span over contiguous values.
   * @return Span valid until this store is resized, moved, or destroyed.
   */
  nonstd::span<const T> createSpan() const
  {
    return {data(), this->getSize()};
  }

  /**
   * @brief Writes contiguous values to a binary file.
   * @param absoluteFilePath Destination file path.
   * @return Error code and message.
   */
  std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const override
  {
    std::ofstream outStrm(absoluteFilePath, std::ios_base::out | std::ios_base::binary);
    if(!outStrm.is_open())
    {
      return {-10170, fmt::format("File could not be opened for writing:\n  '{}'", absoluteFilePath)};
    }

    return writeBinaryFile(outStrm);
  }

  /**
   * @brief Writes contiguous values to a binary stream.
   * @param outputStream Destination stream.
   * @return Error code and message.
   */
  std::pair<int32, std::string> writeBinaryFile(std::ostream& outputStream) const override
  {
    usize totalElements = getNumberOfComponents() * getNumberOfTuples();

    outputStream.write(reinterpret_cast<char*>(m_Data.get()), sizeof(T) * totalElements);

    if(outputStream.bad())
    {
      return {-10175, fmt::format("Error writing binary file:\n  Total Elements:'{}'\n", totalElements)};
    }

    return {0, ""};
  }

  /**
   * @brief Reads HDF5 values into contiguous storage.
   * @param dataset HDF5 dataset to read.
   * @return Error from the HDF5 read.
   */
  Result<> readHdf5(const HDF5::DatasetIO& dataset) override
  {
    return dataset.readIntoSpan(createSpan());
  }

  /**
   * @brief Writes contiguous values to an HDF5 dataset.
   * @param dataset HDF5 dataset to write.
   * @return Error from the HDF5 write.
   */
  Result<> writeHdf5(HDF5::DatasetIO& dataset) const override
  {
    HDF5::DatasetIO::DimsType dims(m_TupleShape.begin(), m_TupleShape.end());
    dims.insert(dims.end(), m_ComponentShape.begin(), m_ComponentShape.end());
    nonstd::span<const T> span = createSpan();
    return dataset.writeSpan(dims, span);
  }

private:
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
  std::unique_ptr<value_type[]> m_Data = nullptr;
  usize m_NumComponents = {0};
  usize m_NumTuples = {0};
  std::optional<T> m_InitValue;
};

/**
 * @brief Names an in-memory uint8 data store.
 */
using UInt8DataStore = DataStore<uint8>;

/**
 * @brief Names an in-memory uint16 data store.
 */
using UInt16DataStore = DataStore<uint16>;

/**
 * @brief Names an in-memory uint32 data store.
 */
using UInt32DataStore = DataStore<uint32>;

/**
 * @brief Names an in-memory uint64 data store.
 */
using UInt64DataStore = DataStore<uint64>;

/**
 * @brief Names an in-memory int8 data store.
 */
using Int8DataStore = DataStore<int8>;

/**
 * @brief Names an in-memory int16 data store.
 */
using Int16DataStore = DataStore<int16>;

/**
 * @brief Names an in-memory int32 data store.
 */
using Int32DataStore = DataStore<int32>;

/**
 * @brief Names an in-memory int64 data store.
 */
using Int64DataStore = DataStore<int64>;

/**
 * @brief Names an in-memory bool data store.
 */
using BoolDataStore = DataStore<bool>;

/**
 * @brief Names an in-memory float32 data store.
 */
using Float32DataStore = DataStore<float32>;

/**
 * @brief Names an in-memory float64 data store.
 */
using Float64DataStore = DataStore<float64>;
} // namespace nx::core
