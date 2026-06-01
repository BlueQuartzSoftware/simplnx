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
#include <vector>

namespace nx::core
{
namespace HDF5
{
class DatasetIO;
}

/**
 * @class AbstractDataStore
 * @brief The AbstractDataStore class serves as an interface class for the
 * various types of data stores used in DataArrays. The basic API and iterators
 * are defined, but the specifics relating to how data is stored are implemented
 * in subclasses.
 * @tparam T
 */
template <typename T>
class AbstractDataStore : public IDataStore
{
public:
  using value_type = T;
  using index_type = uint64;

  /**
   * @brief ValueProxy replaces actual references in AbstractDataStore.
   * DEVELOPER NOTES:
   *   - Non-const iterators and operator[] will use ValueProxy.
   *   - Since ValueProxy does not return a modifiable reference changes
   * are done via `dataStore.setValue(index, value)`. This means code may have
   * addition function calls.
   *   - Common operators like +=, -=, etc. have convenience functions
   * that reduce the number of functions calls. Instead of
   * `dataStore[i] += 42` being equivalent to
   * `dataStore.setValue(i, dataStore.getValue(i) + 42)` it's instead
   * just one virtual function `dataStore.add(i, 42)`.
   *   - ValueProxy does convert to T but is not the same type T. This is
   * relevant when passing to templated functions. They may deduce the type
   * as ValueProxy rather than the intended T. i.e. `std::max(dataStore[i], 42)`
   * will not compile but `std::max<int32>(dataStore[i], 42)` will.
   *   - auto will also have deduce the type as ValueProxy instead of T so
   * `T value = dataStore[i]` should be preferred.
   *   - Since iterators use ValueProxy, it also affects range based for loops.
   * Previously loops would be typically be written like `for(auto& value : dataStore)`.
   * Now they should be `for(auto value : dataStore)`. The ProxyValue should be taken by
   * value in this case. The old version will fail to compile.
   */
  class ValueProxy
  {
  public:
    ValueProxy(AbstractDataStore<T>& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    ValueProxy& operator=(T value)
    {
      setValue(value);
      return *this;
    }

    ValueProxy& operator+=(T value)
    {
      m_DataStore->add(m_Index, value);
      return *this;
    }

    ValueProxy& operator-=(T value)
    {
      m_DataStore->sub(m_Index, value);
      return *this;
    }

    ValueProxy& operator*=(T value)
    {
      m_DataStore->mul(m_Index, value);
      return *this;
    }

    ValueProxy& operator/=(T value)
    {
      m_DataStore->div(m_Index, value);
      return *this;
    }

    ValueProxy& operator%=(T value)
    {
      m_DataStore->rem(m_Index, value);
      return *this;
    }

    ValueProxy& operator&=(T value)
    {
      m_DataStore->bitwiseAND(m_Index, value);
      return *this;
    }

    ValueProxy& operator|=(T value)
    {
      m_DataStore->bitwiseOR(m_Index, value);
      return *this;
    }

    ValueProxy& operator^=(T value)
    {
      m_DataStore->bitwiseXOR(m_Index, value);
      return *this;
    }

    ValueProxy& operator<<=(T value)
    {
      m_DataStore->bitwiseLShift(m_Index, value);
      return *this;
    }

    ValueProxy& operator>>=(T value)
    {
      m_DataStore->bitwiseRShift(m_Index, value);
      return *this;
    }

    ValueProxy& operator=(const ValueProxy& value)
    {
      return *this = static_cast<T>(value);
    }

    void inc()
    {
      m_DataStore->add(m_Index, 1);
    }

    void dec()
    {
      m_DataStore->sub(m_Index, 1);
    }

    void byteSwap()
    {
      m_DataStore->byteSwap(m_Index);
    }

    operator T() const
    {
      return getValue();
    }

    T getValue() const
    {
      return m_DataStore->getValue(m_Index);
    }

    void setValue(T value)
    {
      m_DataStore->setValue(m_Index, value);
    }

    friend void swap(ValueProxy lhs, ValueProxy rhs)
    {
      lhs.m_DataStore->swap(lhs.m_Index, rhs.m_Index);
    }

  private:
    AbstractDataStore<T>* m_DataStore = nullptr;
    usize m_Index = 0;
  };

  using reference = ValueProxy;

  /////////////////////////////////
  // Begin std::iterator support  //
  /////////////////////////////////
  class Iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = T*;
    using reference = ValueProxy;

    Iterator() = default;

    Iterator(AbstractDataStore& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    Iterator operator+(usize offset) const
    {
      return Iterator(*m_DataStore, m_Index + offset);
    }

    friend Iterator operator+(usize offset, const Iterator& iter)
    {
      return iter + offset;
    }

    Iterator operator-(usize offset) const
    {
      return Iterator(*m_DataStore, m_Index - offset);
    }

    Iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    Iterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    // prefix
    Iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    Iterator operator++(int)
    {
      Iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    Iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    Iterator operator--(int)
    {
      Iterator iter = *this;
      m_Index--;
      return iter;
    }

    difference_type operator-(const Iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    reference operator*() const
    {
      return (*m_DataStore)[m_Index];
    }

    reference operator[](difference_type n) const
    {
      return *(*this + n);
    }

    friend bool operator==(const Iterator& lhs, const Iterator& rhs)
    {
      return lhs.m_Index == rhs.m_Index;
    }

    friend std::strong_ordering operator<=>(const Iterator& lhs, const Iterator& rhs)
    {
      return lhs.m_Index <=> rhs.m_Index;
    }

  private:
    AbstractDataStore* m_DataStore = nullptr;
    usize m_Index = 0;
  };

  class ConstIterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = const T*;
    using reference = T;

    ConstIterator() = default;

    ConstIterator(const AbstractDataStore& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    ConstIterator operator+(usize offset) const
    {
      return ConstIterator(*m_DataStore, m_Index + offset);
    }

    friend ConstIterator operator+(usize offset, const ConstIterator& iter)
    {
      return iter + offset;
    }

    ConstIterator operator-(usize offset) const
    {
      return ConstIterator(*m_DataStore, m_Index - offset);
    }

    ConstIterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    ConstIterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    // prefix
    ConstIterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    ConstIterator operator++(int)
    {
      ConstIterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    ConstIterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    ConstIterator operator--(int)
    {
      ConstIterator iter = *this;
      m_Index--;
      return iter;
    }

    difference_type operator-(const ConstIterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    reference operator*() const
    {
      return m_DataStore->getValue(m_Index);
    }

    reference operator[](difference_type n) const
    {
      return *(*this + n);
    }

    friend bool operator==(const ConstIterator& lhs, const ConstIterator& rhs)
    {
      return lhs.m_Index == rhs.m_Index;
    }

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

  ~AbstractDataStore() override = default;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return value_type
   */
  virtual value_type getValue(usize index) const = 0;

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  virtual void setValue(usize index, value_type value) = 0;

  /**
   * @brief Copies a contiguous range of values from this data store into the
   * provided caller-owned buffer.
   *
   * This is the primary bulk-read API for algorithms that need to process data
   * in contiguous blocks. It replaces the earlier chunk-based API and provides
   * a single uniform interface that works identically for both in-memory and
   * out-of-core (OOC) data stores:
   *
   * - **In-memory (DataStore):** Performs a direct std::copy from the backing
   *   array into the buffer. This is essentially zero-overhead.
   * - **Out-of-core (OOC stores):** The OOC subclass translates the flat
   *   element range into the appropriate chunk reads from the backing HDF5
   *   file, coalescing I/O where possible. The caller does not need to know
   *   the chunk layout.
   * - **Empty (EmptyDataStore):** Returns an invalid Result<> because no data
   *   exists.
   *
   * The number of elements to copy is determined by `buffer.size()`. The caller
   * is responsible for ensuring the buffer is large enough and that the range
   * `[startIndex, startIndex + buffer.size())` does not exceed `getSize()`.
   *
   * @param startIndex The starting flat element index to read from
   * @param buffer A span to receive the copied values; its size determines how
   *               many elements are read
   * @return Result<> valid on success; invalid with an error message if the
   *         requested range exceeds the store's size or the store has no data.
   */
  virtual Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const = 0;

  /**
   * @brief Copies values from the provided caller-owned buffer into a
   * contiguous range of this data store.
   *
   * This is the primary bulk-write API, the write-side counterpart of
   * copyIntoBuffer(). It provides a single uniform interface for both
   * in-memory and out-of-core (OOC) data stores:
   *
   * - **In-memory (DataStore):** Performs a direct std::copy from the buffer
   *   into the backing array.
   * - **Out-of-core (OOC stores):** The OOC subclass translates the flat
   *   element range into the appropriate chunk writes to the backing HDF5
   *   file.
   * - **Empty (EmptyDataStore):** Returns an invalid Result<> because no data
   *   exists.
   *
   * The number of elements to copy is determined by `buffer.size()`. The caller
   * is responsible for ensuring the range `[startIndex, startIndex + buffer.size())`
   * does not exceed `getSize()`.
   *
   * @param startIndex The starting flat element index to write to
   * @param buffer A span containing the values to copy into the store; its
   *               size determines how many elements are written
   * @return Result<> valid on success; invalid with an error message if the
   *         requested range exceeds the store's size or the store has no data.
   */
  virtual Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) = 0;

  /**
   * @brief Reads the values contained in the given N-dimensional extent.
   *
   * Unlike the flat copyIntoBuffer() range API, this addresses data by
   * per-axis (min, max, stride) in tuple-space, which lets callers (notably
   * the visualization pipeline) pull a strided sub-volume in a single call.
   * The extent's axis order matches getTupleShape() (slowest- to
   * fastest-varying). Out-of-core subclasses translate the extent into the
   * minimal set of chunk reads.
   *
   * @param extent The N-dimensional extent to read (min/max/stride per axis,
   *               in tuple-space dimension order — same order as getTupleShape()).
   * @return std::vector<T> of length extent.totalElements() * getNumberOfComponents(),
   *         laid out row-major with components as the fastest-varying dimension.
   *         Returns an empty vector if the extent does not match the tuple shape
   *         or the dimensionality is unsupported by the implementation.
   */
  virtual std::vector<T> readExtent(const Extent& extent) const = 0;

  /**
   * @brief Writes values into the given N-dimensional extent.
   *
   * Write-side counterpart of readExtent(): the same per-axis (min, max,
   * stride) addressing and the same row-major layout with components as the
   * fastest-varying dimension. Out-of-core subclasses translate the extent
   * into the minimal set of chunk writes.
   *
   * @param extent The N-dimensional extent to write (min/max/stride per axis,
   *               in tuple-space dimension order — same order as getTupleShape()).
   * @param data Span containing the values to write, of length
   *             extent.totalElements() * getNumberOfComponents(), row-major
   *             with components as the fastest-varying dimension.
   */
  virtual void writeExtent(const Extent& extent, nonstd::span<const T> data) = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return value_type
   */
  value_type operator[](usize index) const
  {
    return getValue(index);
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return value_type
   */
  virtual value_type at(usize index) const = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return reference
   */
  reference operator[](usize index)
  {
    return ValueProxy(*this, index);
  }

  /**
   * @brief Adds value to value at index (equivalent to +=)
   * @param index
   * @param value
   */
  virtual void add(usize index, value_type value) = 0;

  /**
   * @brief Subtracts value to value at index (equivalent to -=)
   * @param index
   * @param value
   */
  virtual void sub(usize index, value_type value) = 0;

  /**
   * @brief Multiplies value at index by value (equivalent to *=)
   * @param index
   * @param value
   */
  virtual void mul(usize index, value_type value) = 0;

  /**
   * @brief Divides value at index by value (equivalent to /=)
   * @param index
   * @param value
   */
  virtual void div(usize index, value_type value) = 0;

  /**
   * @brief Takes remainder of value at index divided by value (equivalent to %=)
   * @param index
   * @param value
   */
  virtual void rem(usize index, value_type value) = 0;

  /**
   * @brief Bitwise AND of value at index with value (equivalent to &=)
   * @param index
   * @param value
   */
  virtual void bitwiseAND(usize index, value_type value) = 0;

  /**
   * @brief Bitwise OR of value at index with value (equivalent to |=)
   * @param index
   * @param value
   */
  virtual void bitwiseOR(usize index, value_type value) = 0;

  /**
   * @brief Bitwise XOR of value at index with value (equivalent to ^=)
   * @param index
   * @param value
   */
  virtual void bitwiseXOR(usize index, value_type value) = 0;

  /**
   * @brief Bitwise left shift of value at index with value (equivalent to <<=)
   * @param index
   * @param value
   */
  virtual void bitwiseLShift(usize index, value_type value) = 0;

  /**
   * @brief Bitwise right shift of value at index with value (equivalent to >>=)
   * @param index
   * @param value
   */
  virtual void bitwiseRShift(usize index, value_type value) = 0;

  /**
   * @brief Swaps bytes of value at index
   * @param index
   * @param value
   */
  virtual void byteSwap(usize index) = 0;

  /**
   * @brief Swaps values at index1 and index2
   * @param index1
   * @param index2
   */
  virtual void swap(usize index1, usize index2) = 0;

  /**
   * @brief Returns an Iterator to the beginning of the DataStore.
   * @return Iterator
   */
  Iterator begin()
  {
    return Iterator(*this, 0);
  }

  /**
   * @brief Returns an Iterator to the end of the DataArray.
   * @return Iterator
   */
  Iterator end()
  {
    return Iterator(*this, getSize());
  }

  /**
   * @brief Returns a ConstIterator to the beginning of the DataStore.
   * @return ConstIterator
   */
  ConstIterator begin() const
  {
    return ConstIterator(*this, 0);
  }

  /**
   * @brief Returns a ConstIterator to the end of the DataArray.
   * @return ConstIterator
   */
  ConstIterator end() const
  {
    return ConstIterator(*this, getSize());
  }

  /**
   * @brief Returns a ConstIterator to the beginning of the DataStore.
   * @return ConstIterator
   */
  ConstIterator cbegin() const
  {
    return begin();
  }

  /**
   * @brief Returns a ConstIterator to the end of the DataStore.
   * @return ConstIterator
   */
  ConstIterator cend() const
  {
    return end();
  }

  /**
   * @brief Fills the AbstractDataStore with the specified value.
   * @param value
   */
  virtual void fill(value_type value)
  {
    std::fill(begin(), end(), value);
  }

  /**
   * @brief Copies data from another AbstractDataStore into this one.
   * The two stores must have the same size for the copy to succeed.
   * @param other The source AbstractDataStore to copy from
   * @return bool True if the copy succeeded, false if sizes don't match
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

  /**
   * @brief Returns the DataStore's DataType as an enum
   * @return DataType
   */
  DataType getDataType() const override
  {
    return GetDataType<T>();
  }

  /**
   * @brief Returns the size of the stored type of the data store.
   * @return usize
   */
  usize getTypeSize() const override
  {
    return sizeof(T);
  }

  /**
   * @brief Returns the data format used for storing the array data.
   * @return data format as string
   */
  std::string getDataFormat() const override
  {
    return "";
  }

  /**
   * @brief copyData This method copies the number of tuples specified by the
   * totalSrcTuples value starting from the source tuple offset value in <b>sourceArray</b>
   * into the current array starting at the target destination tuple offset value.
   *
   * For example, if the DataStore has 10 tuples, the source DataArray has 10 tuples,
   *  the destTupleOffset = 5, the srcTupleOffset = 5, and the totalSrcTuples = 3,
   *  then tuples 5, 6, and 7 will be copied from the source into tuples 5, 6, and 7
   * of the destination array. In pseudocode, it would be the following:
   * @code
   *  destArray[5] = sourceArray[5];
   *  destArray[6] = sourceArray[6];
   *  destArray[7] = sourceArray[7];
   *  ...
   * @endcode
   * @param destTupleOffset
   * @param source
   * @param srcTupleOffset
   * @param totalSrcTuples
   * @return
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

    auto srcBegin = source.begin() + (srcTupleOffset * sourceNumComponents);
    auto srcEnd = srcBegin + (totalSrcTuples * sourceNumComponents);
    auto dstBegin = begin() + (destTupleOffset * numComponents);
    std::copy(srcBegin, srcEnd, dstBegin);
    return {};
  }

  /**
   * @brief Sets all the components of tuple tupleIndex to value.
   * @param tupleIndex
   * @param value
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
   * @brief Sets all component values for a tuple using a pointer array of values.
   * The provided pointer is expected to contain at least the same number of values
   * as the number of components.
   *
   * If the tuple index is out of bounds or the provided pointer is null, this method throws a runtime_error.
   * @param tupleIndex
   * @param values
   * @throw std::runtime_error
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
   * @brief Sets all component values for a tuple using a span of values.
   *
   * If the tuple index is out of bounds or the provided span does not match
   * the number of components, this method throws a runtime_error.
   * @param tupleIndex
   * @param values
   * @throw std::runtime_error
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
   * @brief Sets the component value using a given tuple and component index.
   *
   * This method does nothing if the tuple or component indices are out of bounds
   * @param tupleIndex
   * @param componentIndex
   * @param value
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
   * @brief Returns the component value at the specified tuple and component index.
   *
   * This method returns the default T value if either index is out of bounds.
   * @param tupleIndex
   * @param componentIndex
   * @return value_type
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
   * @brief Flushes the data store to its respective target.
   * In-memory DataStores are not affected.
   */
  virtual void flush() const
  {
  }

  /**
   * @brief Returns the approximate memory usage in bytes for this data store.
   * @return uint64 Memory usage in bytes
   */
  virtual uint64 memoryUsage() const
  {
    return sizeof(T) * getSize();
  }

  /**
   * @brief Reads data from an HDF5 dataset into this data store.
   * @param dataset The HDF5 DatasetIO to read from
   * @return Result<> Result indicating success or error details
   */
  virtual Result<> readHdf5(const HDF5::DatasetIO& dataset) = 0;

  /**
   * @brief Writes data from this data store to an HDF5 dataset.
   * @param dataset The HDF5 DatasetIO to write to
   * @return Result<> Result indicating success or error details
   */
  virtual Result<> writeHdf5(HDF5::DatasetIO& dataset) const = 0;

protected:
  /**
   * @brief Default constructor
   */
  AbstractDataStore() = default;

  /**
   * @brief Copy constructor.
   * @param other The AbstractDataStore to copy from
   */
  AbstractDataStore(const AbstractDataStore& other)
  : IDataStore(other)
  {
  }

  /**
   * @brief Move constructor.
   * @param other The AbstractDataStore to move from
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
