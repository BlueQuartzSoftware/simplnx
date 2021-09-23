#pragma once

#include <algorithm>
#include <iterator>

#include "complex/Utilities/Parsing/HDF5/H5.hpp"

namespace complex
{
namespace H5::Constants::DataStore
{
static const std::string TupleShape = "TupleShape";
static const std::string ComponentShape = "ComponentShape";
} // namespace H5::Constants::DataStore

/**
 * @class IDataStore
 * @brief The IDataStore class serves as an interface class for the
 * various types of data stores used in DataArrays. The basic API and iterators
 * are defined but the specifics relating to how data is stored are implemented
 * in subclasses.
 * @tparam T
 */
template <typename T>
class IDataStore
{
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;

  /////////////////////////////////
  // Begin std::iterator support //
  /////////////////////////////////
  class Iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64_t;
    using pointer = T*;
    using reference = T&;

    Iterator(IDataStore& dataStore, size_t index)
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

    Iterator& operator+(size_t offset)
    {
      m_Index += offset;
      return *this;
    }
    Iterator& operator-(size_t offset)
    {
      m_Index -= offset;
      return *this;
    }
    Iterator& operator+=(size_t offset)
    {
      m_Index += offset;
      return *this;
    }
    Iterator& operator-=(size_t offset)
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
    IDataStore& m_DataStore;
    size_t m_Index = 0;
  };
  class ConstIterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64_t;
    using pointer = const T*;
    using reference = const T&;

    ConstIterator(const IDataStore& dataStore, size_t index)
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

    ConstIterator& operator+(size_t offset)
    {
      m_Index += offset;
      return *this;
    }
    ConstIterator& operator-(size_t offset)
    {
      m_Index -= offset;
      return *this;
    }
    ConstIterator& operator+=(size_t offset)
    {
      m_Index += offset;
      return *this;
    }
    ConstIterator& operator-=(size_t offset)
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
    const IDataStore& m_DataStore;
    size_t m_Index = 0;
  };
  ///////////////////////////////
  // End std::iterator support //
  ///////////////////////////////

  virtual ~IDataStore() = default;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return size_t
   */
  virtual size_t getNumberOfTuples() const = 0;

  /**
   * @brief Returns the number of components
   * @return size_t
   */
  virtual size_t getNumberOfComponents() const = 0;

  /**
   * @brief Returns the number of values stored within the DataStore.
   * @return size_t
   */
  size_t getSize() const
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  /**
   * @brief Resizes the DataStore to handle the specified number of tuples.
   * @param numTuples
   */
  virtual void reshapeTuples(const std::vector<size_t>& tupleShape) = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return T
   */
  virtual value_type getValue(size_t index) const = 0;

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  virtual void setValue(size_t index, value_type value) = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  virtual const_reference operator[](size_t index) const = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  virtual const_reference at(size_t index) const = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return T&
   */
  virtual reference operator[](size_t index) = 0;

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return IDataStore*
   */
  virtual IDataStore* deepCopy() const = 0;

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
   * @brief Fills the IDataStore with the specified value.
   * @param value
   */
  virtual void fill(value_type value)
  {
    std::fill(begin(), end(), value);
  }

  /**
   * @brief Writes the data store to HDF5. Returns the HDF5 error code should
   * one be encountered. Otherwise, returns 0.
   * @param dataId
   * @return H5::ErrorType
   */
  virtual H5::ErrorType writeHdf5(H5::IdType dataId) const = 0;

protected:
  /**
   * @brief Default constructor
   */
  IDataStore()
  {
  }
};

template <typename Iter>
typename Iter::difference_type distance(Iter first, Iter last)
{
  return last - first;
}
} // namespace complex
