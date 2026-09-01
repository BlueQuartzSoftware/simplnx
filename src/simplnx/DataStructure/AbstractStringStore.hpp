#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <memory>
#include <string>
#include <vector>

namespace nx::core
{

/**
 * @class AbstractStringStore
 * @brief Defines string storage for StringArray.
 *
 * A materialized store provides mutable string values. A placeholder store
 * retains tuple metadata until import materializes values. isPlaceholder()
 * avoids a concrete-store type check during deferred import. Value access
 * requires a materialized store. Bounds-checked access reports std::out_of_range
 * for invalid indexes. Placeholder access reports std::runtime_error.
 */
class SIMPLNX_EXPORT AbstractStringStore
{
public:
  using value_type = std::string;
  using reference = value_type&;
  using const_reference = const value_type&;

  /**
   * @class Iterator
   * @brief Provides non-owning mutable iteration by flat index.
   *
   * The store must outlive the iterator. Comparisons require iterators from the
   * same store. A default-constructed Iterator is unbound. Do not call
   * isValid() or an operation that accesses its store until binding.
   */
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

    inline Iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    inline Iterator operator++(int)
    {
      Iterator iter = *this;
      m_Index++;
      return iter;
    }

    inline Iterator& operator--()
    {
      m_Index--;
      return *this;
    }

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

  /**
   * @class ConstIterator
   * @brief Provides non-owning constant iteration by flat index.
   *
   * The store must outlive the iterator. Comparisons require iterators from the
   * same store. A default-constructed ConstIterator is unbound. isValid()
   * accepts it and returns false. Other store operations require binding.
   */
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

    ConstIterator& operator++()
    {
      m_Index++;
      return *this;
    }

    ConstIterator operator++(int)
    {
      ConstIterator iter = *this;
      m_Index++;
      return iter;
    }

    ConstIterator& operator--()
    {
      m_Index--;
      return *this;
    }

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

  virtual ~AbstractStringStore() = default;

  /**
   * @brief Creates an independent store.
   * @return Owning pointer to the copy.
   */
  virtual std::unique_ptr<AbstractStringStore> deepCopy() const = 0;

  virtual usize size() const = 0;
  virtual bool empty() const = 0;

  virtual bool isPlaceholder() const = 0;

  virtual usize getNumberOfTuples() const = 0;

  /**
   * @brief Returns tuple dimensions.
   * @return Tuple-shape reference owned by this store.
   * @post The reference remains valid until the store changes shape or is destroyed.
   */
  virtual const ShapeType& getTupleShape() const = 0;

  virtual void resizeTuples(const ShapeType& tupleShape) = 0;

  virtual reference operator[](usize index) = 0;

  virtual const_reference operator[](usize index) const = 0;

  virtual const_reference at(usize index) const = 0;

  virtual const_reference getValue(usize index) const = 0;

  virtual void setValue(usize index, const value_type& value) = 0;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  virtual AbstractStringStore& operator=(const std::vector<std::string>& values) = 0;
  bool operator==(const std::vector<std::string>& values) const;
  bool operator!=(const std::vector<std::string>& values) const;

protected:
  AbstractStringStore() = default;
};
} // namespace nx::core
