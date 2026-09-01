#pragma once

#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <compare>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

namespace HDF5
{
class DatasetIO;
}

/**
 * @class AbstractDataStore
 * @brief Defines typed storage that is independent of its backing implementation.
 * @tparam T Stored value type.
 */
template <typename T>
class AbstractDataStore : public IDataStore
{
public:
  /**
   * @brief Names the data-store value type.
   */
  using value_type = T;

  /**
   * @brief Names the data-store index type.
   */
  using index_type = uint64;

  /**
   * @class ValueProxy
   * @brief Provides indexed access without exposing a native reference.
   *
   * The proxy reads and writes through its parent data store. The parent data
   * store must outlive the proxy. Use `T value = dataStore[index]` when a
   * template requires a value of type T. Use values, not references, in range
   * loops over a non-const data store.
   *
   * Compound operators call data-store operations directly. This avoids separate
   * virtual read and write calls.
   */
  class ValueProxy
  {
  public:
    /**
     * @brief Creates an indexed data-store proxy.
     * @param dataStore Parent data store that must outlive the proxy.
     * @param index Flat value index.
     */
    ValueProxy(AbstractDataStore<T>& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    /**
     * @brief Stores a value through the proxy.
     * @param value Value to store.
     * @return This proxy.
     */
    ValueProxy& operator=(T value)
    {
      setValue(value);
      return *this;
    }

    /**
     * @brief Adds a value through the proxy.
     * @param value Value to add.
     * @return This proxy.
     */
    ValueProxy& operator+=(T value)
    {
      m_DataStore->add(m_Index, value);
      return *this;
    }

    /**
     * @brief Subtracts a value through the proxy.
     * @param value Value to subtract.
     * @return This proxy.
     */
    ValueProxy& operator-=(T value)
    {
      m_DataStore->sub(m_Index, value);
      return *this;
    }

    /**
     * @brief Multiplies the proxy value.
     * @param value Multiplier.
     * @return This proxy.
     */
    ValueProxy& operator*=(T value)
    {
      m_DataStore->mul(m_Index, value);
      return *this;
    }

    /**
     * @brief Divides the proxy value.
     * @param value Divisor.
     * @return This proxy.
     */
    ValueProxy& operator/=(T value)
    {
      m_DataStore->div(m_Index, value);
      return *this;
    }

    /**
     * @brief Replaces the proxy value with its remainder.
     * @param value Divisor.
     * @return This proxy.
     */
    ValueProxy& operator%=(T value)
    {
      m_DataStore->rem(m_Index, value);
      return *this;
    }

    /**
     * @brief Applies a bitwise AND through the proxy.
     * @param value Operand.
     * @return This proxy.
     */
    ValueProxy& operator&=(T value)
    {
      m_DataStore->bitwiseAND(m_Index, value);
      return *this;
    }

    /**
     * @brief Applies a bitwise OR through the proxy.
     * @param value Operand.
     * @return This proxy.
     */
    ValueProxy& operator|=(T value)
    {
      m_DataStore->bitwiseOR(m_Index, value);
      return *this;
    }

    /**
     * @brief Applies a bitwise XOR through the proxy.
     * @param value Operand.
     * @return This proxy.
     */
    ValueProxy& operator^=(T value)
    {
      m_DataStore->bitwiseXOR(m_Index, value);
      return *this;
    }

    /**
     * @brief Shifts the proxy value left.
     * @param value Shift count.
     * @return This proxy.
     */
    ValueProxy& operator<<=(T value)
    {
      m_DataStore->bitwiseLShift(m_Index, value);
      return *this;
    }

    /**
     * @brief Shifts the proxy value right.
     * @param value Shift count.
     * @return This proxy.
     */
    ValueProxy& operator>>=(T value)
    {
      m_DataStore->bitwiseRShift(m_Index, value);
      return *this;
    }

    /**
     * @brief Copies a value from another proxy.
     * @param value Source proxy.
     * @return This proxy.
     */
    ValueProxy& operator=(const ValueProxy& value)
    {
      return *this = static_cast<T>(value);
    }

    /**
     * @brief Increments the proxy value.
     */
    void inc()
    {
      m_DataStore->add(m_Index, 1);
    }

    /**
     * @brief Decrements the proxy value.
     */
    void dec()
    {
      m_DataStore->sub(m_Index, 1);
    }

    /**
     * @brief Swaps the byte order of the proxy value.
     */
    void byteSwap()
    {
      m_DataStore->byteSwap(m_Index);
    }

    /**
     * @brief Converts the proxy to its stored value.
     * @return Stored value.
     */
    operator T() const
    {
      return getValue();
    }

    T getValue() const
    {
      return m_DataStore->getValue(m_Index);
    }

    /**
     * @brief Stores a value through the proxy.
     * @param value Value to store.
     */
    void setValue(T value)
    {
      m_DataStore->setValue(m_Index, value);
    }

    /**
     * @brief Swaps the values of two proxies.
     * @param lhs First proxy.
     * @param rhs Second proxy.
     */
    friend void swap(ValueProxy lhs, ValueProxy rhs)
    {
      lhs.m_DataStore->swap(lhs.m_Index, rhs.m_Index);
    }

  private:
    AbstractDataStore<T>* m_DataStore = nullptr;
    usize m_Index = 0;
  };

  /**
   * @brief Names the mutable value-proxy type.
   */
  using reference = ValueProxy;

  /////////////////////////////////
  // Begin std::iterator support  //
  /////////////////////////////////
  /**
   * @class Iterator
   * @brief Iterates over mutable values through ValueProxy.
   */
  class Iterator
  {
  public:
    /**
     * @brief Identifies random-access iterator behavior.
     */
    using iterator_category = std::random_access_iterator_tag;

    /**
     * @brief Names the iterator value type.
     */
    using value_type = T;

    /**
     * @brief Names the iterator distance type.
     */
    using difference_type = int64;

    /**
     * @brief Names the associated pointer type.
     */
    using pointer = T*;

    /**
     * @brief Names the mutable value-proxy type.
     */
    using reference = ValueProxy;

    /**
     * @brief Creates an unbound iterator.
     */
    Iterator() = default;

    /**
     * @brief Creates an iterator at a flat value index.
     * @param dataStore Data store that must outlive the iterator.
     * @param index Flat value index.
     */
    Iterator(AbstractDataStore& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    /**
     * @brief Returns an iterator advanced by an offset.
     * @param offset Number of values to advance.
     * @return Advanced iterator.
     */
    Iterator operator+(usize offset) const
    {
      return Iterator(*m_DataStore, m_Index + offset);
    }

    /**
     * @brief Returns an iterator advanced by an offset.
     * @param offset Number of values to advance.
     * @param iter Iterator to advance.
     * @return Advanced iterator.
     */
    friend Iterator operator+(usize offset, const Iterator& iter)
    {
      return iter + offset;
    }

    /**
     * @brief Returns an iterator moved back by an offset.
     * @param offset Number of values to move back.
     * @return Moved iterator.
     */
    Iterator operator-(usize offset) const
    {
      return Iterator(*m_DataStore, m_Index - offset);
    }

    /**
     * @brief Advances this iterator.
     * @param offset Number of values to advance.
     * @return This iterator.
     */
    Iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    /**
     * @brief Moves this iterator back.
     * @param offset Number of values to move back.
     * @return This iterator.
     */
    Iterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    /**
     * @brief Advances this iterator by one value.
     * @return This iterator after the increment.
     */
    Iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    /**
     * @brief Advances this iterator by one value.
     * @param unused Distinguishes the postfix operator.
     * @return Iterator before the increment.
     */
    Iterator operator++([[maybe_unused]] int unused)
    {
      Iterator iter = *this;
      m_Index++;
      return iter;
    }

    /**
     * @brief Moves this iterator back by one value.
     * @return This iterator after the decrement.
     */
    Iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    /**
     * @brief Moves this iterator back by one value.
     * @param unused Distinguishes the postfix operator.
     * @return Iterator before the decrement.
     */
    Iterator operator--([[maybe_unused]] int unused)
    {
      Iterator iter = *this;
      m_Index--;
      return iter;
    }

    /**
     * @brief Returns the distance to another iterator.
     * @param rhs Iterator that defines the endpoint.
     * @return Difference between the flat indexes.
     */
    difference_type operator-(const Iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    /**
     * @brief Returns the proxy at the current index.
     * @return Value proxy.
     */
    reference operator*() const
    {
      return (*m_DataStore)[m_Index];
    }

    /**
     * @brief Returns the proxy at a relative index.
     * @param n Relative value index.
     * @return Value proxy.
     */
    reference operator[](difference_type n) const
    {
      return *(*this + n);
    }

    /**
     * @brief Tests two flat indexes for equality.
     * @param lhs First iterator.
     * @param rhs Second iterator.
     * @return True when both flat indexes are equal.
     */
    friend bool operator==(const Iterator& lhs, const Iterator& rhs)
    {
      return lhs.m_Index == rhs.m_Index;
    }

    /**
     * @brief Compares two flat indexes.
     * @param lhs First iterator.
     * @param rhs Second iterator.
     * @return Relative ordering of the flat indexes.
     */
    friend std::strong_ordering operator<=>(const Iterator& lhs, const Iterator& rhs)
    {
      return lhs.m_Index <=> rhs.m_Index;
    }

  private:
    AbstractDataStore* m_DataStore = nullptr;
    usize m_Index = 0;
  };

  /**
   * @class ConstIterator
   * @brief Iterates over values without modifying the data store.
   */
  class ConstIterator
  {
  public:
    /**
     * @brief Identifies random-access iterator behavior.
     */
    using iterator_category = std::random_access_iterator_tag;

    /**
     * @brief Names the iterator value type.
     */
    using value_type = T;

    /**
     * @brief Names the iterator distance type.
     */
    using difference_type = int64;

    /**
     * @brief Names the associated pointer type.
     */
    using pointer = const T*;

    /**
     * @brief Names the value type returned by dereference.
     */
    using reference = T;

    /**
     * @brief Creates an unbound iterator.
     */
    ConstIterator() = default;

    /**
     * @brief Creates an iterator at a flat value index.
     * @param dataStore Data store that must outlive the iterator.
     * @param index Flat value index.
     */
    ConstIterator(const AbstractDataStore& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    /**
     * @brief Returns an iterator advanced by an offset.
     * @param offset Number of values to advance.
     * @return Advanced iterator.
     */
    ConstIterator operator+(usize offset) const
    {
      return ConstIterator(*m_DataStore, m_Index + offset);
    }

    /**
     * @brief Returns an iterator advanced by an offset.
     * @param offset Number of values to advance.
     * @param iter Iterator to advance.
     * @return Advanced iterator.
     */
    friend ConstIterator operator+(usize offset, const ConstIterator& iter)
    {
      return iter + offset;
    }

    /**
     * @brief Returns an iterator moved back by an offset.
     * @param offset Number of values to move back.
     * @return Moved iterator.
     */
    ConstIterator operator-(usize offset) const
    {
      return ConstIterator(*m_DataStore, m_Index - offset);
    }

    /**
     * @brief Advances this iterator.
     * @param offset Number of values to advance.
     * @return This iterator.
     */
    ConstIterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    /**
     * @brief Moves this iterator back.
     * @param offset Number of values to move back.
     * @return This iterator.
     */
    ConstIterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    /**
     * @brief Advances this iterator by one value.
     * @return This iterator after the increment.
     */
    ConstIterator& operator++()
    {
      m_Index++;
      return *this;
    }

    /**
     * @brief Advances this iterator by one value.
     * @param unused Distinguishes the postfix operator.
     * @return Iterator before the increment.
     */
    ConstIterator operator++([[maybe_unused]] int unused)
    {
      ConstIterator iter = *this;
      m_Index++;
      return iter;
    }

    /**
     * @brief Moves this iterator back by one value.
     * @return This iterator after the decrement.
     */
    ConstIterator& operator--()
    {
      m_Index--;
      return *this;
    }

    /**
     * @brief Moves this iterator back by one value.
     * @param unused Distinguishes the postfix operator.
     * @return Iterator before the decrement.
     */
    ConstIterator operator--([[maybe_unused]] int unused)
    {
      ConstIterator iter = *this;
      m_Index--;
      return iter;
    }

    /**
     * @brief Returns the distance to another iterator.
     * @param rhs Iterator that defines the endpoint.
     * @return Difference between the flat indexes.
     */
    difference_type operator-(const ConstIterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    /**
     * @brief Returns the value at the current index.
     * @return Stored value.
     */
    reference operator*() const
    {
      return m_DataStore->getValue(m_Index);
    }

    /**
     * @brief Returns the value at a relative index.
     * @param n Relative value index.
     * @return Stored value.
     */
    reference operator[](difference_type n) const
    {
      return *(*this + n);
    }

    /**
     * @brief Tests two flat indexes for equality.
     * @param lhs First iterator.
     * @param rhs Second iterator.
     * @return True when both flat indexes are equal.
     */
    friend bool operator==(const ConstIterator& lhs, const ConstIterator& rhs)
    {
      return lhs.m_Index == rhs.m_Index;
    }

    /**
     * @brief Compares two flat indexes.
     * @param lhs First iterator.
     * @param rhs Second iterator.
     * @return Relative ordering of the flat indexes.
     */
    friend std::strong_ordering operator<=>(const ConstIterator& lhs, const ConstIterator& rhs)
    {
      return lhs.m_Index <=> rhs.m_Index;
    }

  private:
    const AbstractDataStore* m_DataStore = nullptr;
    usize m_Index = 0;
  };
  ///////////////////////////////
  // End std::iterator support //
  ///////////////////////////////

  /**
   * @brief Destroys the data store.
   */
  ~AbstractDataStore() override = default;

  virtual value_type getValue(usize index) const = 0;

  /**
   * @brief Stores a value at a flat index.
   * @param index Flat value index.
   * @param value Value to store.
   */
  virtual void setValue(usize index, value_type value) = 0;

  /**
   * @brief Copies a contiguous value range into caller-owned storage.
   *
   * The buffer size selects the number of values. The store does not own the
   * buffer. Implementations support in-memory and out-of-core storage.
   * @param startIndex First flat value index to read.
   * @param buffer Receives copied values.
   * @return Error if the range is invalid or the store has no data.
   */
  virtual Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const = 0;

  /**
   * @brief Copies caller-owned values into a contiguous value range.
   *
   * The buffer size selects the number of values. The store does not retain the
   * buffer. Implementations support in-memory and out-of-core storage.
   * @param startIndex First flat value index to write.
   * @param buffer Values to copy.
   * @return Error if the range is invalid or the store has no data.
   */
  virtual Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) = 0;

  /**
   * @brief Reads a tuple-space extent into a new value vector.
   *
   * Extent axes use getTupleShape() order. Values use row-major order with
   * components as the fastest-varying dimension.
   * @param extent Tuple-space extent with minimum, maximum, and stride values.
   * @return Extent values, or an empty vector when the extent is not valid.
   */
  virtual std::vector<T> readExtent(const Extent& extent) const = 0;

  /**
   * @brief Reads an N-dimensional extent into caller-owned storage.
   *
   * The destination uses the readExtent() layout. Each concrete store implements
   * this operation. The operation does not allocate result storage.
   *
   * @param extent N-dimensional tuple-space extent to read.
   * @param destination Receives exactly `extent.totalElements() * getNumberOfComponents()` values.
   * @throws std::invalid_argument If the extent or destination size is invalid.
   * @throws std::runtime_error If the store does not support data access.
   */
  virtual void readExtentIntoBuffer(const Extent& extent, nonstd::span<T> destination) const = 0;

  /**
   * @brief Reads related N-dimensional extents into caller-owned storage.
   *
   * All extents and destination sizes are validated before this method writes a
   * destination. The base implementation reads each extent independently.
   * Backends can override this method to share a backing-store traversal.
   *
   * @param extents Tuple-space extents to read.
   * @param destinations One exact-sized destination for each extent, in input order.
   * @throws std::invalid_argument If counts, extents, or destination sizes are invalid.
   * @throws std::runtime_error If the store does not support extent reads.
   */
  virtual void readExtentsIntoBuffers(nonstd::span<const Extent> extents, nonstd::span<nonstd::span<T>> destinations) const
  {
    if(extents.size() != destinations.size())
    {
      throw std::invalid_argument(fmt::format("AbstractDataStore::readExtentsIntoBuffers: extent count ({}) does not match destination count ({})", extents.size(), destinations.size()));
    }

    const ShapeType& tupleShape = getTupleShape();
    for(usize extentIndex = 0; extentIndex < extents.size(); ++extentIndex)
    {
      const Extent& extent = extents[extentIndex];
      if(extent.dimensions() != tupleShape.size())
      {
        throw std::invalid_argument(
            fmt::format("AbstractDataStore::readExtentsIntoBuffers: extent {} dimensions ({}) do not match tuple-shape dimensions ({})", extentIndex, extent.dimensions(), tupleShape.size()));
      }
      for(usize dimension = 0; dimension < tupleShape.size(); ++dimension)
      {
        if(extent.stride[dimension] == 0 || extent.min[dimension] > extent.max[dimension] || extent.max[dimension] >= tupleShape[dimension])
        {
          throw std::invalid_argument(fmt::format("AbstractDataStore::readExtentsIntoBuffers: extent {} dimension {} has min {}, max {}, stride {}, and tuple bound {}", extentIndex, dimension,
                                                  extent.min[dimension], extent.max[dimension], extent.stride[dimension], tupleShape[dimension]));
        }
      }

      const usize requiredValues = static_cast<usize>(extent.totalElements()) * getNumberOfComponents();
      if(destinations[extentIndex].size() != requiredValues)
      {
        throw std::invalid_argument(fmt::format("AbstractDataStore::readExtentsIntoBuffers: destination {} has {} values; expected {}", extentIndex, destinations[extentIndex].size(), requiredValues));
      }
    }

    for(usize extentIndex = 0; extentIndex < extents.size(); ++extentIndex)
    {
      readExtentIntoBuffer(extents[extentIndex], destinations[extentIndex]);
    }
  }

  /**
   * @brief Reads several tuple-space extents into new value vectors.
   *
   * The base implementation reads each extent independently. Backends can
   * override this method to share one backing-store traversal. Each result uses
   * the readExtent() layout and has the same order as its input extent.
   * @param extents Tuple-space extents to read.
   * @return One value vector for each input extent.
   */
  virtual std::vector<std::vector<T>> readExtents(nonstd::span<const Extent> extents) const
  {
    std::vector<std::vector<T>> results;
    results.reserve(extents.size());
    for(const Extent& extent : extents)
    {
      results.push_back(readExtent(extent));
    }
    return results;
  }

  /**
   * @brief Writes values into a tuple-space extent.
   *
   * Extent axes use getTupleShape() order. Values use row-major order with
   * components as the fastest-varying dimension.
   * @param extent Tuple-space extent with minimum, maximum, and stride values.
   * @param data Values to write. The span has `extent.totalElements() * getNumberOfComponents()` values.
   */
  virtual void writeExtent(const Extent& extent, nonstd::span<const T> data) = 0;

  value_type operator[](usize index) const
  {
    return getValue(index);
  }

  virtual value_type at(usize index) const = 0;

  /**
   * @brief Returns a proxy that writes through to a flat value index.
   * @param index Flat value index.
   * @return Proxy that accesses the stored value.
   */
  reference operator[](usize index)
  {
    return ValueProxy(*this, index);
  }

  /**
   * @brief Adds a value at a flat index.
   * @param index Flat value index.
   * @param value Value to add.
   */
  virtual void add(usize index, value_type value) = 0;

  /**
   * @brief Subtracts a value at a flat index.
   * @param index Flat value index.
   * @param value Value to subtract.
   */
  virtual void sub(usize index, value_type value) = 0;

  /**
   * @brief Multiplies a value at a flat index.
   * @param index Flat value index.
   * @param value Multiplier.
   */
  virtual void mul(usize index, value_type value) = 0;

  /**
   * @brief Divides a value at a flat index.
   * @param index Flat value index.
   * @param value Divisor.
   */
  virtual void div(usize index, value_type value) = 0;

  /**
   * @brief Replaces a value with its remainder.
   * @param index Flat value index.
   * @param value Divisor.
   */
  virtual void rem(usize index, value_type value) = 0;

  /**
   * @brief Applies a bitwise AND at a flat index.
   * @param index Flat value index.
   * @param value Operand.
   */
  virtual void bitwiseAND(usize index, value_type value) = 0;

  /**
   * @brief Applies a bitwise OR at a flat index.
   * @param index Flat value index.
   * @param value Operand.
   */
  virtual void bitwiseOR(usize index, value_type value) = 0;

  /**
   * @brief Applies a bitwise XOR at a flat index.
   * @param index Flat value index.
   * @param value Operand.
   */
  virtual void bitwiseXOR(usize index, value_type value) = 0;

  /**
   * @brief Shifts a value left at a flat index.
   * @param index Flat value index.
   * @param value Shift count.
   */
  virtual void bitwiseLShift(usize index, value_type value) = 0;

  /**
   * @brief Shifts a value right at a flat index.
   * @param index Flat value index.
   * @param value Shift count.
   */
  virtual void bitwiseRShift(usize index, value_type value) = 0;

  /**
   * @brief Swaps the byte order of a value.
   * @param index Flat value index.
   */
  virtual void byteSwap(usize index) = 0;

  /**
   * @brief Swaps two stored values.
   * @param index1 First flat value index.
   * @param index2 Second flat value index.
   */
  virtual void swap(usize index1, usize index2) = 0;

  Iterator begin()
  {
    return Iterator(*this, 0);
  }

  Iterator end()
  {
    return Iterator(*this, getSize());
  }

  ConstIterator begin() const
  {
    return ConstIterator(*this, 0);
  }

  ConstIterator end() const
  {
    return ConstIterator(*this, getSize());
  }

  ConstIterator cbegin() const
  {
    return begin();
  }
  ConstIterator cend() const
  {
    return end();
  }

  /**
   * @brief Stores one value in every element.
   * @param value Value to store.
   */
  virtual void fill(value_type value)
  {
    std::fill(begin(), end(), value);
  }

  /**
   * @brief Copies all values from another data store.
   * @param other Source data store.
   * @return True when both stores have the same size.
   */
  virtual bool copy(const AbstractDataStore& other)
  {
    if(getSize() != other.getSize())
    {
      return false;
    }
    std::copy(other.begin(), other.end(), begin());
    return true;
  }

  DataType getDataType() const override
  {
    return GetDataType<T>();
  }

  usize getTypeSize() const override
  {
    return sizeof(T);
  }

  std::string getDataFormat() const override
  {
    return "";
  }

  /**
   * @brief Copies complete tuples from another data store.
   *
   * The stores must have the same component count. Bounded bulk pages preserve
   * resolver-selected destination backends. They avoid full source
   * materialization and per-value virtual or cache access.
   * @param destTupleOffset First destination tuple to replace.
   * @param source Source store; it may use a different backing implementation.
   * @param srcTupleOffset First source tuple to read.
   * @param totalSrcTuples Number of complete tuples to transfer.
   * @return First bounds, component-count, source-read, or destination-write error.
   */
  Result<> copyFrom(usize destTupleOffset, const AbstractDataStore& source, usize srcTupleOffset, usize totalSrcTuples)
  {
    if(destTupleOffset >= getNumberOfTuples())
    {
      return MakeErrorResult(-14600, fmt::format("The destination tuple offset ({}) is out of range of the number of available tuples in the data store ({}). Please ensure the destination tuple "
                                                 "offset is less than the number of available tuples.",
                                                 destTupleOffset, getNumberOfTuples()));
    }

    usize sourceNumComponents = source.getNumberOfComponents();
    usize numComponents = getNumberOfComponents();

    if(sourceNumComponents != numComponents)
    {
      return MakeErrorResult(-14601, fmt::format("The number of components in the source data store ({}) does not match the number of components in the destination data store ({}). Please verify "
                                                 "that source and destination data stores have the same number of components.",
                                                 sourceNumComponents, numComponents));
    }

    if((totalSrcTuples * sourceNumComponents + destTupleOffset * numComponents) > getSize())
    {
      return MakeErrorResult(-14602,
                             fmt::format("The total size of tuples to be copied ({}) plus the offset in the destination data store ({}) exceeds the available size of the destination data store ({}).",
                                         totalSrcTuples * sourceNumComponents, destTupleOffset * numComponents, getSize()));
    }

    if((totalSrcTuples * sourceNumComponents + srcTupleOffset * sourceNumComponents) > source.getSize())
    {
      return MakeErrorResult(-14603, fmt::format("The total size of tuples to be copied ({}) plus the offset in the source data store ({}) exceeds the available size of the source data store ({}).",
                                                 totalSrcTuples * sourceNumComponents, srcTupleOffset * sourceNumComponents, source.getSize()));
    }

    constexpr usize k_TargetBufferBytes = 1024 * 1024;
    const usize totalElements = totalSrcTuples * numComponents;
    const usize bufferElements = std::max<usize>(1, std::min(totalElements, k_TargetBufferBytes / sizeof(T)));
    auto buffer = std::make_unique<T[]>(bufferElements);

    const usize sourceStart = srcTupleOffset * sourceNumComponents;
    const usize destinationStart = destTupleOffset * numComponents;
    for(usize copiedElements = 0; copiedElements < totalElements; copiedElements += bufferElements)
    {
      const usize count = std::min(bufferElements, totalElements - copiedElements);
      nonstd::span<T> readBuffer(buffer.get(), count);
      Result<> readResult = source.copyIntoBuffer(sourceStart + copiedElements, readBuffer);
      if(readResult.invalid())
      {
        return readResult;
      }

      Result<> writeResult = copyFromBuffer(destinationStart + copiedElements, nonstd::span<const T>(buffer.get(), count));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
    return {};
  }

  /**
   * @brief Sets all values in a tuple.
   * @param tupleIndex Tuple index.
   * @param value Value to store in each component.
   */
  void fillTuple(index_type tupleIndex, T value)
  {
    usize numComponents = getNumberOfComponents();
    index_type offset = tupleIndex * numComponents;
    for(usize i = 0; i < numComponents; i++)
    {
      setValue(offset + i, value);
    }
  }

  /**
   * @brief Sets all values in a tuple from a pointer.
   * @param tupleIndex Tuple index.
   * @param values Pointer to at least getNumberOfComponents() values.
   * @throws std::runtime_error If values is null or tupleIndex is invalid.
   */
  void setTuple(index_type tupleIndex, const value_type* values)
  {
    if(values == nullptr)
    {
      throw std::runtime_error("Provided values pointer cannot be null");
    }

    nonstd::span<const value_type> valueSpan(values, values + getNumberOfComponents());
    setTuple(tupleIndex, valueSpan);
  }

  /**
   * @brief Sets all values in a tuple from a span.
   * @param tupleIndex Tuple index.
   * @param values Exactly getNumberOfComponents() values.
   * @throws std::runtime_error If values has the wrong size or tupleIndex is invalid.
   */
  void setTuple(index_type tupleIndex, nonstd::span<const value_type> values)
  {
    if(values.size() != getNumberOfComponents())
    {
      auto ss = fmt::format("Span size ({}) does not match the number of components ({})", values.size(), getNumberOfComponents());
      throw std::runtime_error(ss);
    }

    if(tupleIndex >= getNumberOfTuples())
    {
      auto ss = fmt::format("Tuple index ({}) is greater than or equal to the number of tuples ({})", tupleIndex, getNumberOfTuples());
      throw std::runtime_error(ss);
    }

    index_type numComponents = getNumberOfComponents();
    index_type offset = tupleIndex * numComponents;
    usize count = values.size();
    for(usize i = 0; i < count; i++)
    {
      setValue(offset + i, values[i]);
    }
  }

  /**
   * @brief Sets one component value in a tuple.
   * @param tupleIndex Tuple index.
   * @param componentIndex Component index.
   * @param value Value to store.
   * @throws std::runtime_error If tupleIndex or componentIndex is invalid.
   */
  void setComponent(index_type tupleIndex, index_type componentIndex, value_type value)
  {
    if(tupleIndex >= getNumberOfTuples())
    {
      auto ss = fmt::format("Tuple index ({}) is greater than or equal to the number of tuples ({})", tupleIndex, getNumberOfTuples());
      throw std::runtime_error(ss);
    }

    if(componentIndex >= getNumberOfComponents())
    {
      auto ss = fmt::format("Component index ({}) is greater than or equal to the number of components ({})", componentIndex, getNumberOfComponents());
      throw std::runtime_error(ss);
    }

    index_type index = tupleIndex * getNumberOfComponents() + componentIndex;
    setValue(index, value);
  }

  /**
   * @brief Returns one component value from a tuple.
   * @param tupleIndex Tuple index.
   * @param componentIndex Component index.
   * @return Stored component value.
   * @throws std::runtime_error If tupleIndex or componentIndex is invalid.
   */
  value_type getComponentValue(index_type tupleIndex, index_type componentIndex) const
  {
    if(tupleIndex >= getNumberOfTuples())
    {
      auto ss = fmt::format("Tuple index ({}) is greater than or equal to the number of tuples ({})", tupleIndex, getNumberOfTuples());
      throw std::runtime_error(ss);
    }

    if(componentIndex >= getNumberOfComponents())
    {
      auto ss = fmt::format("Component index ({}) is greater than or equal to the number of components ({})", componentIndex, getNumberOfComponents());
      throw std::runtime_error(ss);
    }

    index_type index = tupleIndex * getNumberOfComponents() + componentIndex;
    return getValue(index);
  }

  /**
   * @brief Flushes storage changes to the backing target.
   *
   * In-memory stores do not have a backing target.
   */
  virtual void flush() const
  {
  }

  /**
   * @brief Returns the approximate memory usage in bytes.
   * @return Approximate memory usage in bytes.
   */
  virtual uint64 memoryUsage() const
  {
    return sizeof(T) * getSize();
  }

  /**
   * @brief Reads values from an HDF5 dataset.
   * @param dataset HDF5 dataset to read.
   * @return Error from the HDF5 read.
   */
  virtual Result<> readHdf5(const HDF5::DatasetIO& dataset) = 0;

  /**
   * @brief Writes values to an HDF5 dataset.
   * @param dataset HDF5 dataset to write.
   * @return Error from the HDF5 write.
   */
  virtual Result<> writeHdf5(HDF5::DatasetIO& dataset) const = 0;

protected:
  /**
   * @brief Creates a data store without values.
   */
  AbstractDataStore() = default;

  /**
   * @brief Copies base data-store state.
   * @param other Data store to copy.
   */
  AbstractDataStore(const AbstractDataStore& other)
  : IDataStore(other)
  {
  }

  /**
   * @brief Moves base data-store state.
   * @param other Data store to move.
   */
  AbstractDataStore(AbstractDataStore&& other)
  : IDataStore(std::move(other))
  {
  }
};

using UInt8AbstractDataStore = AbstractDataStore<uint8>;
using UInt16AbstractDataStore = AbstractDataStore<uint16>;
using UInt32AbstractDataStore = AbstractDataStore<uint32>;
using UInt64AbstractDataStore = AbstractDataStore<uint64>;

using Int8AbstractDataStore = AbstractDataStore<int8>;
using Int16AbstractDataStore = AbstractDataStore<int16>;
using Int32AbstractDataStore = AbstractDataStore<int32>;
using Int64AbstractDataStore = AbstractDataStore<int64>;

using BoolAbstractDataStore = AbstractDataStore<bool>;

using Float32AbstractDataStore = AbstractDataStore<float32>;
using Float64AbstractDataStore = AbstractDataStore<float64>;
} // namespace nx::core
