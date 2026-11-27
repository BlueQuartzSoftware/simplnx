#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
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
  class iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = AbstractStringStore::value_type;
    using difference_type = int64;
    using pointer = value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

    iterator()
    : m_DataStore(nullptr)
    , m_Index(0)
    {
    }

    iterator(AbstractStringStore& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    iterator(const iterator& other)
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }
    iterator(iterator&& other) noexcept
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }

    iterator& operator=(const iterator& rhs)
    {
      m_DataStore = rhs.m_DataStore;
      m_Index = rhs.m_Index;
      return *this;
    }
    iterator& operator=(iterator&& rhs) noexcept
    {
      m_DataStore = rhs.m_DataStore;
      m_Index = rhs.m_Index;
      return *this;
    }

    ~iterator() noexcept = default;

    bool isValid() const
    {
      return m_Index < m_DataStore->size();
    }

    iterator operator+(usize offset) const
    {
      return iterator(*m_DataStore, m_Index + offset);
    }

    iterator operator-(usize offset) const
    {
      return iterator(*m_DataStore, m_Index - offset);
    }

    iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    iterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    iterator operator++(int)
    {
      iterator iter = *this;
      m_Index++;
      return iter;
    }

    iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    iterator operator--(int)
    {
      iterator iter = *this;
      m_Index--;
      return iter;
    }

    difference_type operator-(const iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    reference operator*() const
    {
      return (*m_DataStore)[m_Index];
    }

    bool operator==(const iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    bool operator!=(const iterator& rhs) const
    {
      return !(*this == rhs);
    }

    bool operator<(const iterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    bool operator>(const iterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    bool operator<=(const iterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    bool operator>=(const iterator& rhs) const
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
  class const_iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = AbstractStringStore::value_type;
    using difference_type = int64;
    using pointer = const value_type*;
    using reference = const value_type&;

    const_iterator()
    : m_DataStore(nullptr)
    , m_Index(0)
    {
    }

    const_iterator(const AbstractStringStore& dataStore, usize index)
    : m_DataStore(&dataStore)
    , m_Index(index)
    {
    }

    const_iterator(const const_iterator& other)
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }

    const_iterator(const_iterator&& other) noexcept
    : m_DataStore(other.m_DataStore)
    , m_Index(other.m_Index)
    {
    }

    const_iterator& operator=(const const_iterator& rhs)
    {
      m_DataStore = rhs.m_DataStore;
      m_Index = rhs.m_Index;
      return *this;
    }
    const_iterator& operator=(const_iterator&& rhs) noexcept
    {
      m_DataStore = rhs.m_DataStore;
      m_Index = rhs.m_Index;
      return *this;
    }

    ~const_iterator() noexcept = default;

    bool isValid() const
    {
      return m_DataStore != nullptr && m_Index < m_DataStore->size();
    }

    const_iterator operator+(usize offset) const
    {
      return const_iterator(*m_DataStore, m_Index + offset);
    }

    const_iterator operator-(usize offset) const
    {
      return const_iterator(*m_DataStore, m_Index - offset);
    }

    const_iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    const_iterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    const_iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    const_iterator operator++(int)
    {
      const_iterator iter = *this;
      m_Index++;
      return iter;
    }

    const_iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    const_iterator operator--(int)
    {
      const_iterator iter = *this;
      m_Index--;
      return iter;
    }

    difference_type operator-(const const_iterator& rhs) const
    {
      if(!isValid() && !rhs.isValid())
      {
        return 0;
      }
      return m_Index - rhs.m_Index;
    }

    reference operator*() const
    {
      return (*m_DataStore)[m_Index];
    }

    bool operator==(const const_iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    bool operator!=(const const_iterator& rhs) const
    {
      return m_Index != rhs.m_Index;
    }

    bool operator<(const const_iterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    bool operator>(const const_iterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    bool operator<=(const const_iterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    bool operator>=(const const_iterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    const AbstractStringStore* m_DataStore = nullptr;
    usize m_Index = 0;
  };

  virtual ~AbstractStringStore() = default;

  /**
   * @brief Copies string storage into the required destination format.
   * @param destinationFormat Resolved destination format; empty and the canonical in-memory name select memory.
   * @return Independent values with the same shape, or an independent placeholder without values.
   * @throws std::runtime_error If the selected factory or copy fails, or an OOC build rejects an unavailable format.
   * @throws std::bad_alloc If an allocation fails.
   *
   * In-core builds use memory for unavailable formats. OOC builds reject unavailable formats. Factory failures never fall back.
   * Generic transfer holds one string plus backend buffers.
   * @warning For materialized stores, explicit in-memory selection requires a complete resident destination and can exhaust available RAM.
   * This store obeys destinationFormat and does not resolve storage policy itself.
   * Resolve the destination policy before this call. Use the destination DataStructure for automatic policy selection.
   */
  virtual std::unique_ptr<AbstractStringStore> deepCopy(const std::string& destinationFormat) const = 0;

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

  /**
   * @brief Changes the tuple shape.
   * @param tupleShape New tuple dimensions in slowest-to-fastest order.
   * @return Valid on success. A resize failure returns error -6035.
   *
   * Callers must inspect the result because a failed resize can retain the prior string shape.
   */
  [[nodiscard]] virtual Result<> resizeTuples(const ShapeType& tupleShape) = 0;

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
