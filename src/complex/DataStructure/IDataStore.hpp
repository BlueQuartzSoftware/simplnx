#pragma once

#include <algorithm>
#include <iterator>

namespace complex
{
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
  class Iterator : public std::iterator<std::random_access_iterator_tag, T, int64_t, T*, T&>
  {
  public:
    using reference = T&;
    using difference_type = int64_t;

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
  class ConstIterator : public std::iterator<std::random_access_iterator_tag, T, int64_t, const T*, const T&>
  {
  public:
    using reference = const T&;
    using difference_type = int64_t;

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
  virtual size_t getTupleCount() const = 0;

  /**
   * @brief Returns the tuple size.
   * @return size_t
   */
  virtual size_t getTupleSize() const = 0;

  /**
   * @brief Returns the number of values stored within the DataStore.
   * @return size_t
   */
  size_t getSize() const
  {
    return getTupleCount() * getTupleSize();
  }

  /**
   * @brief Resizes the DataStore to handle the specified number of tuples.
   * @param numTuples
   */
  virtual void resizeTuples(size_t numTuples) = 0;

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
   * @brief Fills the IDataStore with the specified value.
   * @param value
   */
  virtual void fill(value_type value)
  {
    std::fill(begin(), end(), value);
  }

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
