#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace nx::core
{
class AbstractStringStore
{
public:
  using value_type = std::string;
  using reference = value_type&;
  using const_reference = const value_type&;

  class Iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = AbstractStringStore::value_type;
    using difference_type = int64;
    using pointer = value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

    Iterator()
    : m_DataStore(nullptr)
    , m_Index(0)
    {
    }

    Iterator(AbstractStringStore& dataStore, usize index)
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
      return m_Index < m_DataStore->size();
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
    AbstractStringStore* m_DataStore;
    usize m_Index = 0;
  };

  class ConstIterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = AbstractStringStore::value_type;
    using difference_type = int64;
    using pointer = const value_type*;
    using reference = const value_type&;

    ConstIterator()
    : m_DataStore(nullptr)
    , m_Index(0)
    {
    }

    ConstIterator(const AbstractStringStore& dataStore, usize index)
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
      return m_DataStore != nullptr && m_Index < m_DataStore->size();
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
    const AbstractStringStore* m_DataStore = nullptr;
    usize m_Index = 0;
  };

  using iterator = Iterator;
  using const_iterator = ConstIterator;

  ~AbstractStringStore() = default;

  /**
   * @brief Creates a deep copy of this AbstractStringStore.
   * @return std::unique_ptr<AbstractStringStore> Unique pointer to the deep copy
   */
  virtual std::unique_ptr<AbstractStringStore> deepCopy() const = 0;

  /**
   * @brief Returns the total number of strings in the store.
   * @return usize The number of strings
   */
  virtual usize size() const = 0;

  /**
   * @brief Returns whether the string store is empty.
   * @return bool True if the store has no strings, false otherwise
   */
  virtual bool empty() const = 0;

  /**
   * @brief Returns the number of tuples in the StringStore.
   * @return usize
   */
  virtual usize getNumberOfTuples() const = 0;
  /**
   * @brief Returns the dimensions of the Tuples
   * @return
   */
  virtual const ShapeType& getTupleShape() const = 0;

  /**
   * @brief This method sets the shape of the dimensions to `tupleShape`.
   * @param tupleShape The new shape of the data where the dimensions are "C" ordered
   * from *slowest* to *fastest*.
   */
  virtual void resizeTuples(const ShapeType& tupleShape) = 0;

  /**
   * @brief Array subscript operator to access the string at the specified index.
   * @param index The index to access
   * @return reference Reference to the string at the specified index
   */
  virtual reference operator[](usize index) = 0;

  /**
   * @brief Const array subscript operator to access the string at the specified index.
   * @param index The index to access
   * @return const_reference Const reference to the string at the specified index
   */
  virtual const_reference operator[](usize index) const = 0;

  /**
   * @brief Returns a const reference to the string at the specified index with bounds checking.
   * @param index The index to access
   * @return const_reference Const reference to the string at the specified index
   */
  virtual const_reference at(usize index) const = 0;

  /**
   * @brief Returns the string value at the specified index.
   * @param index The index to retrieve
   * @return const_reference Const reference to the string value
   */
  virtual const_reference getValue(usize index) const = 0;

  /**
   * @brief Sets the string value at the specified index.
   * @param index The index to set
   * @param value The string value to set
   */
  virtual void setValue(usize index, const value_type& value) = 0;

  /**
   * @brief Returns an iterator to the beginning of the strings.
   * @return iterator Iterator to the first string
   */
  iterator begin();

  /**
   * @brief Returns an iterator to the end of the strings.
   * @return iterator Iterator past the last string
   */
  iterator end();

  /**
   * @brief Returns a const iterator to the beginning of the strings.
   * @return const_iterator Const iterator to the first string
   */
  const_iterator begin() const;

  /**
   * @brief Returns a const iterator to the end of the strings.
   * @return const_iterator Const iterator past the last string
   */
  const_iterator end() const;

  /**
   * @brief Returns a const iterator to the beginning of the strings.
   * @return const_iterator Const iterator to the first string
   */
  const_iterator cbegin() const;

  /**
   * @brief Returns a const iterator to the end of the strings.
   * @return const_iterator Const iterator past the last string
   */
  const_iterator cend() const;

  /**
   * @brief Assignment operator to replace all strings with values from a vector.
   * @param values Vector of strings to assign
   * @return AbstractStringStore& Reference to this AbstractStringStore
   */
  virtual AbstractStringStore& operator=(const std::vector<std::string>& values) = 0;

  /**
   * @brief Equality comparison operator to check if the store contains the same strings as the vector.
   * @param values Vector of strings to compare against
   * @return bool True if all strings match, false otherwise
   */
  bool operator==(const std::vector<std::string>& values) const;

  /**
   * @brief Inequality comparison operator to check if the store differs from the vector.
   * @param values Vector of strings to compare against
   * @return bool True if any strings differ, false otherwise
   */
  bool operator!=(const std::vector<std::string>& values) const;

protected:
  /**
   * @brief Default constructor.
   */
  AbstractStringStore() = default;
};
} // namespace nx::core
