#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/IDataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"

#include <algorithm>
#include <iterator>

namespace complex
{
namespace H5
{
class DatasetWriter;
} // namespace H5

/**
 * @class IDataStore
 * @brief The IDataStore class serves as an interface class for the
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
    virtual ~Iterator() = default;

    Iterator& operator+(usize offset)
    {
      m_Index += offset;
      return *this;
    }
    Iterator& operator-(usize offset)
    {
      m_Index -= offset;
      return *this;
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
    Iterator& operator++()
    {
      m_Index++;
      return *this;
    }
    Iterator& operator++(int)
    {
      m_Index++;
      return *this;
    }
    Iterator& operator--()
    {
      m_Index--;
      return *this;
    }
    Iterator& operator--(int)
    {
      m_Index--;
      return *this;
    }
    difference_type operator-(const Iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    reference operator*() const
    {
      return m_DataStore[m_Index];
    }

    Iterator& operator=(const Iterator& rhs)
    {
      m_Index = rhs.m_Index;
      return *this;
    }
    bool operator==(const Iterator& rhs) const
    {
      return (&m_DataStore == &rhs.m_DataStore) && (m_Index == rhs.m_Index);
    }
    bool operator!=(const Iterator& rhs) const
    {
      return (&m_DataStore != &rhs.m_DataStore) || (m_Index != rhs.m_Index);
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
    virtual ~ConstIterator() = default;

    ConstIterator& operator+(usize offset)
    {
      m_Index += offset;
      return *this;
    }
    ConstIterator& operator-(usize offset)
    {
      m_Index -= offset;
      return *this;
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
    ConstIterator& operator++()
    {
      m_Index++;
      return *this;
    }
    ConstIterator& operator++(int)
    {
      m_Index++;
      return *this;
    }
    ConstIterator& operator--()
    {
      m_Index--;
      return *this;
    }
    ConstIterator& operator--(int)
    {
      m_Index--;
      return *this;
    }
    difference_type operator-(const ConstIterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    reference operator*() const
    {
      return m_DataStore[m_Index];
    }

    ConstIterator& operator=(const ConstIterator& rhs)
    {
      m_Index = rhs.m_Index;
      return *this;
    }
    bool operator==(const ConstIterator& rhs) const
    {
      return (&m_DataStore == &rhs.m_DataStore) && (m_Index == rhs.m_Index);
    }
    bool operator!=(const ConstIterator& rhs) const
    {
      return (&m_DataStore != &rhs.m_DataStore) || (m_Index != rhs.m_Index);
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
