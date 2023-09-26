#pragma once

#include "complex/Common/IteratorUtility.hpp"
#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/IDataStore.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>

namespace complex
{
/**
 * @class AbstractDataStore
 * @brief The AbstractDataStore class serves as an interface class for the
 * various types of data stores used in DataArrays. The basic API and iterators
 * are defined but the specifics relating to how data is stored are implemented
 * in subclasses.
 * @tparam T
 */
template <typename T>
class AbstractDataStore : public IDataStore
{
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using ShapeType = typename IDataStore::ShapeType;
  using index_type = uint64;

  /////////////////////////////////
  // Begin std::iterator support //
  /////////////////////////////////
#if defined(__linux__)
  class Iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = T*;
    using reference = T&;
    using const_reference = const T&;

    Iterator()
    : m_DataStore(nullptr)
    , m_Index(0)
    {
    }

    Iterator(AbstractDataStore& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    Iterator(const Iterator& other)
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }
    Iterator(Iterator&& other) noexcept
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }

    Iterator& operator=(const Iterator& rhs)
    {
      m_DataStore = rhs.m_DataStore;
      m_Index = rhs.m_Index;
      return *this;
    }
    Iterator& operator=(Iterator&& rhs) noexcept
    {
      m_DataStore = rhs.m_DataStore;
      m_Index = rhs.m_Index;
      return *this;
    }

    ~Iterator() noexcept = default;

    inline bool isValid() const
    {
      return m_Index < m_DataStore->getSize();
    }

    inline Iterator operator+(usize offset) const
    {
      return Iterator(*m_DataStore, m_Index + offset);
    }

    inline Iterator operator-(usize offset) const
    {
      return Iterator(*m_DataStore, m_Index - offset);
    }

    inline Iterator& operator+=(usize offset)
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
    inline Iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    inline Iterator operator++(int)
    {
      Iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    inline Iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    inline Iterator operator--(int)
    {
      Iterator iter = *this;
      m_Index--;
      return iter;
    }

    inline difference_type operator-(const Iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return (*m_DataStore)[m_Index];
    }

    inline bool operator==(const Iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const Iterator& rhs) const
    {
      return !(*this == rhs);
    }

    inline bool operator<(const Iterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    inline bool operator>(const Iterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    inline bool operator<=(const Iterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    inline bool operator>=(const Iterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    AbstractDataStore* m_DataStore;
    usize m_Index = 0;
  };

  class ConstIterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = const T*;
    using reference = const T&;

    ConstIterator()
    : m_DataStore(nullptr)
    , m_Index(0)
    {
    }

    ConstIterator(const AbstractDataStore& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    ConstIterator(const ConstIterator& other)
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }

    ConstIterator(ConstIterator&& other) noexcept
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }

    ConstIterator& operator=(const ConstIterator& rhs)
    {
      m_DataStore = rhs.m_DataStore;
      m_Index = rhs.m_Index;
      return *this;
    }
    ConstIterator& operator=(ConstIterator&& rhs) noexcept
    {
      m_DataStore = rhs.m_DataStore;
      m_Index = rhs.m_Index;
      return *this;
    }

    ~ConstIterator() noexcept = default;

    bool isValid() const
    {
      return m_DataStore != nullptr && m_Index < m_DataStore->getSize();
    }

    ConstIterator operator+(usize offset) const
    {
      return ConstIterator(*m_DataStore, m_Index + offset);
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
      Iterator iter = *this;
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
      if(!isValid() && !rhs.isValid())
      {
        return 0;
      }
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return (*m_DataStore)[m_Index];
    }

    bool operator==(const ConstIterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    bool operator!=(const ConstIterator& rhs) const
    {
      return m_Index != rhs.m_Index;
    }

    bool operator<(const ConstIterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    bool operator>(const ConstIterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    bool operator<=(const ConstIterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    bool operator>=(const ConstIterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    const AbstractDataStore* m_DataStore = nullptr;
    usize m_Index = 0;
  };
#else
  class Iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = T*;
    using reference = T&;
    using const_reference = const T&;

    Iterator(AbstractDataStore& dataStore, usize index)
    : m_DataStore(dataStore)
    , m_Index(index)
    {
    }

    Iterator(const Iterator& other)
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }
    Iterator(Iterator&& other) noexcept
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }

    Iterator& operator=(const Iterator& rhs)
    {
      m_Index = rhs.m_Index;
      return *this;
    }
    Iterator& operator=(Iterator&& rhs) noexcept
    {
      m_Index = rhs.m_Index;
      return *this;
    }

    ~Iterator() noexcept = default;

    inline bool isValid() const
    {
      return m_Index < m_DataStore.getSize();
    }

    inline Iterator operator+(usize offset) const
    {
      return Iterator(m_DataStore, m_Index + offset);
    }

    inline Iterator operator-(usize offset) const
    {
      return Iterator(m_DataStore, m_Index - offset);
    }

    inline Iterator& operator+=(usize offset)
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
    inline Iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    inline Iterator operator++(int)
    {
      Iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    inline Iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    inline Iterator operator--(int)
    {
      Iterator iter = *this;
      m_Index--;
      return iter;
    }

    inline difference_type operator-(const Iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return m_DataStore[m_Index];
    }

    inline bool operator==(const Iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const Iterator& rhs) const
    {
      return !(*this == rhs);
    }

    inline bool operator<(const Iterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    inline bool operator>(const Iterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    inline bool operator<=(const Iterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    inline bool operator>=(const Iterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    AbstractDataStore& m_DataStore;
    usize m_Index = 0;
  };

  class ConstIterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = const T*;
    using reference = const T&;

    ConstIterator(const AbstractDataStore& dataStore, usize index)
    : m_DataStore(dataStore)
    , m_Index(index)
    {
    }

    ConstIterator(const ConstIterator& other)
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }
    ConstIterator(ConstIterator&& other) noexcept
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }

    ConstIterator& operator=(const ConstIterator& rhs)
    {
      m_Index = rhs.m_Index;
      return *this;
    }
    ConstIterator& operator=(ConstIterator&& rhs) noexcept
    {
      m_Index = rhs.m_Index;
      return *this;
    }

    ~ConstIterator() noexcept = default;

    bool isValid() const
    {
      return m_Index < m_DataStore.getSize();
    }

    ConstIterator operator+(usize offset) const
    {
      return ConstIterator(m_DataStore, m_Index + offset);
    }

    ConstIterator operator-(usize offset) const
    {
      return ConstIterator(m_DataStore, m_Index - offset);
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
      Iterator iter = *this;
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
      if(!isValid() && !rhs.isValid())
      {
        return 0;
      }
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return m_DataStore[m_Index];
    }

    bool operator==(const ConstIterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    bool operator!=(const ConstIterator& rhs) const
    {
      return m_Index != rhs.m_Index;
    }

    bool operator<(const ConstIterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    bool operator>(const ConstIterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    bool operator<=(const ConstIterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    bool operator>=(const ConstIterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    const AbstractDataStore& m_DataStore;
    usize m_Index = 0;
  };
#endif
  ///////////////////////////////
  // End std::iterator support //
  ///////////////////////////////

  virtual ~AbstractDataStore() = default;

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
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  virtual const_reference operator[](usize index) const = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  virtual const_reference at(usize index) const = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return T&
   */
  virtual reference operator[](usize index) = 0;

  /**
   * @brief Returns an Iterator to the begining of the DataStore.
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
   * @brief Returns an ConstIterator to the begining of the DataStore.
   * @return ConstIterator
   */
  ConstIterator begin() const
  {
    return ConstIterator(*this, 0);
  }

  /**
   * @brief Returns an ConstIterator to the end of the DataArray.
   * @return ConstIterator
   */
  ConstIterator end() const
  {
    return ConstIterator(*this, getSize());
  }

  /**
   * @brief Returns an ConstIterator to the begining of the DataStore.
   * @return ConstIterator
   */
  ConstIterator cbegin() const
  {
    return begin();
  }

  /**
   * @brief Returns an ConstIterator to the end of the DataStore.
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
   * For example if the DataStore has 10 tuples, the source DataArray has 10 tuples,
   *  the destTupleOffset = 5, the srcTupleOffset = 5, and the totalSrcTuples = 3,
   *  then tuples 5, 6, and 7 will be copied from the source into tuples 5, 6, and 7
   * of the destination array. In psuedo code it would be the following:
   * @code
   *  destArray[5] = sourceArray[5];
   *  destArray[6] = sourceArray[6];
   *  destArray[7] = sourceArray[7];
   *  .....
   * @endcode
   * @param destTupleOffset
   * @param source
   * @param srcTupleOffset
   * @param totalSrcTuples
   * @return
   */
  bool copyFrom(usize destTupleOffset, const AbstractDataStore& source, usize srcTupleOffset, usize totalSrcTuples)
  {
    if(destTupleOffset >= getNumberOfTuples())
    {
      return false;
    }

    usize sourceNumComponents = source.getNumberOfComponents();
    usize numComponents = getNumberOfComponents();

    if(sourceNumComponents != numComponents)
    {
      return false;
    }

    if((totalSrcTuples * sourceNumComponents + destTupleOffset * numComponents) > getSize())
    {
      return false;
    }

    auto srcBegin = source.begin() + (srcTupleOffset * sourceNumComponents);
    auto srcEnd = srcBegin + (totalSrcTuples * sourceNumComponents);
    auto dstBegin = begin() + (destTupleOffset * numComponents);
    std::copy(srcBegin, srcEnd, dstBegin);
    return true;
  }

  /**
   * @brief Sets all the components of tuple i to value.
   * @param i
   * @param value
   */
  void fillTuple(index_type i, T value)
  {
    usize numComponents = getNumberOfComponents();
    std::fill_n(begin() + (i * numComponents), numComponents, value);
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
    std::copy(values.begin(), values.end(), begin() + offset);
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

  std::optional<ShapeType> getChunkShape() const override
  {
    return {};
  }

  /**
   * @brief Returns the data for a particular data chunk. Returns an empty span if the data is not chunked.
   * @param chunkPosition
   * @return chunk data as span
   */
  virtual std::vector<T> getChunkValues(const ShapeType& chunkPosition) const
  {
    return {};
  }

  /**
   * @brief Flushes the data store to its respective target.
   * In-memory DataStores are not affected.
   */
  virtual void flush() const
  {
  }

  virtual uint64 memoryUsage() const
  {
    return sizeof(T) * getSize();
  }

protected:
  /**
   * @brief Default constructor
   */
  AbstractDataStore() = default;
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
} // namespace complex
