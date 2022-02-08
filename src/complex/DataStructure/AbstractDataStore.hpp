#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/IDataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"

#include <algorithm>
#include <functional>
#include <iterator>

namespace complex
{
namespace H5
{
class DatasetWriter;
} // namespace H5

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
  static constexpr const char k_TupleShape[] = "TupleShape";
  static constexpr const char k_ComponentShape[] = "ComponentShape";

  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using ShapeType = typename IDataStore::ShapeType;

  /////////////////////////////////
  // Begin std::iterator support //
  /////////////////////////////////
  class Iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = T*;
    using reference = T&;

    Iterator() = delete;

    Iterator(AbstractDataStore& dataStore, usize index)
    : m_DataStore(dataStore)
    , m_Index(index)
    {
    }

    Iterator(const Iterator&) = default;
    Iterator(Iterator&&) noexcept = default;

    Iterator& operator=(const Iterator&) = default;
    Iterator& operator=(Iterator&&) noexcept = default;

    ~Iterator() noexcept = default;

    Iterator operator+(usize offset) const
    {
      return Iterator(m_DataStore.get(), m_Index + offset);
    }

    Iterator operator-(usize offset) const
    {
      return Iterator(m_DataStore.get(), m_Index - offset);
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
      return m_DataStore.get()[m_Index];
    }

    bool operator==(const Iterator& rhs) const
    {
      return (std::addressof(m_DataStore.get()) == std::addressof(rhs.m_DataStore.get())) && (m_Index == rhs.m_Index);
    }

    bool operator!=(const Iterator& rhs) const
    {
      return !(*this == rhs);
    }

    bool operator<(const Iterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    bool operator>(const Iterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    bool operator<=(const Iterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    bool operator>=(const Iterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    std::reference_wrapper<AbstractDataStore> m_DataStore;
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

    ConstIterator() = delete;

    ConstIterator(const AbstractDataStore& dataStore, usize index)
    : m_DataStore(dataStore)
    , m_Index(index)
    {
    }

    ConstIterator(const ConstIterator&) = default;
    ConstIterator(ConstIterator&&) noexcept = default;

    ConstIterator& operator=(const ConstIterator&) = default;
    ConstIterator& operator=(ConstIterator&&) noexcept = default;

    ~ConstIterator() noexcept = default;

    ConstIterator operator+(usize offset) const
    {
      return ConstIterator(m_DataStore.get(), m_Index + offset);
    }

    ConstIterator operator-(usize offset) const
    {
      return ConstIterator(m_DataStore.get(), m_Index - offset);
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
      return m_Index - rhs.m_Index;
    }

    reference operator*() const
    {
      return m_DataStore.get()[m_Index];
    }

    bool operator==(const ConstIterator& rhs) const
    {
      return (std::addressof(m_DataStore.get()) == std::addressof(rhs.m_DataStore.get())) && (m_Index == rhs.m_Index);
    }

    bool operator!=(const ConstIterator& rhs) const
    {
      return !(*this == rhs);
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
    std::reference_wrapper<const AbstractDataStore> m_DataStore;
    usize m_Index = 0;
  };
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

  /**
   * @brief Returns the DataStore's DataType as an enum
   * @return DataType
   */
  DataType getDataType() const override;

  /**
   * @brief Returns the size of the stored type of the data store.
   * @return usize
   */
  usize getTypeSize() const override
  {
    return sizeof(T);
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
  void fillTuple(usize i, T value)
  {
    usize numComponents = getNumberOfComponents();
    std::fill_n(begin() + (i * numComponents), numComponents, value);
  }

protected:
  /**
   * @brief Default constructor
   */
  AbstractDataStore()
  {
  }
};

template <typename Iter>
typename Iter::difference_type distance(Iter first, Iter last)
{
  return last - first;
}
} // namespace complex
