#pragma once

#include "simplnx/Common/Bit.hpp"
#include "simplnx/Common/IteratorUtility.hpp"
#include "simplnx/Common/Types.hpp"

#include "nonstd/span.hpp"

#include <memory>

namespace nx::core
{
template <typename T>
class DataVector
{
public:
  /////////////////////////////////
  // Begin std::iterator support //
  /////////////////////////////////
#if defined(__linux__)
  class iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = T*;
    using reference = T&;

    iterator()
    : m_DataVector(nullptr)
    , m_Index(0)
    {
    }

    iterator(DataVector& DataVector, uint64 index)
    : m_DataVector(DataVector)
    , m_Index(index)
    {
    }

    iterator(const iterator& other)
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    iterator(iterator&& other) noexcept
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    iterator& operator=(const iterator& rhs)
    {
      m_DataVector = rhs.m_DataVector;
      m_Index = rhs.m_Index;
      return *this;
    }

    iterator& operator=(iterator&& rhs) noexcept
    {
      m_DataVector = rhs.m_DataVector;
      m_Index = rhs.m_Index;
      return *this;
    }

    ~iterator() noexcept = default;

    inline bool isValid() const
    {
      return m_Index < m_DataVector->size();
    }

    inline iterator operator+(usize offset) const
    {
      return iterator(m_DataVector, m_Index + offset);
    }

    inline iterator operator-(usize offset) const
    {
      return iterator(m_DataVector, m_Index - offset);
    }

    inline iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    inline iterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    // prefix
    inline iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    inline iterator operator++(int)
    {
      iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    inline iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    inline iterator operator--(int)
    {
      iterator iter = *this;
      m_Index--;
      return iter;
    }

    inline difference_type operator-(const iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return m_DataVector[m_Index];
    }

    inline bool operator==(const iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const iterator& rhs) const
    {
      return m_Index != rhs.m_Index;
    }

    inline bool operator<(const iterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    inline bool operator>(const iterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    inline bool operator<=(const iterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    inline bool operator>=(const iterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    DataVector& m_DataVector;
    uint64 m_Index = 0;
  };

  class const_iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = const T*;
    using reference = const T&;

    const_iterator()
    : m_DataVector(nullptr)
    , m_Index(0)
    {
    }
    const_iterator(const_iterator&& other) noexcept
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    const_iterator(const DataVector& dataVector, uint64 index)
    : m_DataVector(dataVector)
    , m_Index(index)
    {
    }

    const_iterator(const const_iterator& other)
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    const_iterator& operator=(const const_iterator& rhs)
    {
      m_DataVector = rhs.m_DataVector;
      m_Index = rhs.m_Index;
      return *this;
    }

    const_iterator& operator=(const_iterator&& rhs) noexcept
    {
      m_DataVector = rhs.m_DataVector;
      m_Index = rhs.m_Index;
      return *this;
    }

    ~const_iterator() noexcept = default;

    inline bool isValid() const
    {
      return m_Index < m_DataVector->size();
    }

    inline const_iterator operator+(usize offset) const
    {
      return const_iterator(m_DataVector, m_Index + offset);
    }

    inline const_iterator operator-(usize offset) const
    {
      return const_iterator(m_DataVector, m_Index - offset);
    }

    inline const_iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    inline const_iterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    // prefix
    inline const_iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    inline const_iterator operator++(int)
    {
      iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    inline const_iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    inline const_iterator operator--(int)
    {
      const_iterator iter = *this;
      m_Index--;
      return iter;
    }

    inline difference_type operator-(const const_iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return m_DataVector[m_Index];
    }

    inline bool operator==(const const_iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const const_iterator& rhs) const
    {
      return m_Index != rhs.m_Index;
    }

    inline bool operator<(const const_iterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    inline bool operator>(const const_iterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    inline bool operator<=(const const_iterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    inline bool operator>=(const const_iterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    const DataVector& m_DataVector;
    uint64 m_Index = 0;
  };
#else
  class iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = T*;
    using reference = T&;

    iterator(DataVector& DataVector, uint64 index)
    : m_DataVector(DataVector)
    , m_Index(index)
    {
    }

    iterator(const iterator& other)
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    iterator(iterator&& other) noexcept
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    iterator& operator=(const iterator& rhs)
    {
      m_Index = rhs.m_Index;
      return *this;
    }

    iterator& operator=(iterator&& rhs) noexcept
    {
      m_Index = rhs.m_Index;
      return *this;
    }

    ~iterator() noexcept = default;

    inline bool isValid() const
    {
      return m_Index < m_DataVector.size();
    }

    inline iterator operator+(usize offset) const
    {
      return iterator(m_DataVector, m_Index + offset);
    }

    inline iterator operator-(usize offset) const
    {
      return iterator(m_DataVector, m_Index - offset);
    }

    inline iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    inline iterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    // prefix
    inline iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    inline iterator operator++(int)
    {
      iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    inline iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    inline iterator operator--(int)
    {
      iterator iter = *this;
      m_Index--;
      return iter;
    }

    inline difference_type operator-(const iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return m_DataVector[m_Index];
    }

    inline bool operator==(const iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const iterator& rhs) const
    {
      return m_Index != rhs.m_Index;
    }

    inline bool operator<(const iterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    inline bool operator>(const iterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    inline bool operator<=(const iterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    inline bool operator>=(const iterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    DataVector& m_DataVector;
    uint64 m_Index = 0;
  };

  class const_iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = const T*;
    using reference = const T&;

    const_iterator(const DataVector& dataVector, uint64 index)
    : m_DataVector(dataVector)
    , m_Index(index)
    {
    }

    const_iterator(const const_iterator& other)
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }
    const_iterator(const_iterator&& other) noexcept
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    const_iterator& operator=(const const_iterator& rhs)
    {
      m_Index = rhs.m_Index;
      return *this;
    }
    const_iterator& operator=(const_iterator&& rhs) noexcept
    {
      m_Index = rhs.m_Index;
      return *this;
    }

    ~const_iterator() noexcept = default;

    inline bool isValid() const
    {
      return m_Index < m_DataVector.size();
    }

    inline const_iterator operator+(usize offset) const
    {
      return const_iterator(m_DataVector, m_Index + offset);
    }

    inline const_iterator operator-(usize offset) const
    {
      return const_iterator(m_DataVector, m_Index - offset);
    }

    inline const_iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    inline const_iterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    // prefix
    inline const_iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    inline const_iterator operator++(int)
    {
      iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    inline const_iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    inline const_iterator operator--(int)
    {
      const_iterator iter = *this;
      m_Index--;
      return iter;
    }

    inline difference_type operator-(const const_iterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return m_DataVector[m_Index];
    }

    inline bool operator==(const const_iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const const_iterator& rhs) const
    {
      return m_Index != rhs.m_Index;
    }

    inline bool operator<(const const_iterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    inline bool operator>(const const_iterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    inline bool operator<=(const const_iterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    inline bool operator>=(const const_iterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    const DataVector& m_DataVector;
    uint64 m_Index = 0;
  };
#endif
  ///////////////////////////////
  // End std::iterator support //
  ///////////////////////////////

  using value_type = T;
  using size_type = uint64;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = T*;
  using const_pointer = const T*;

  /**
   * @brief Constructs a DataVector with the given number of elements.
   * @param numElements
   */
  DataVector()
  : m_Size(0)
  , m_Data(nullptr)
  , m_DataPtr(nullptr)
  {
  }

  /**
   * @brief Constructs a DataVector with the given number of elements.
   * @param numElements
   */
  DataVector(size_type numElements)
  : m_Size(numElements)
  , m_Data(std::make_unique<value_type[]>(numElements))
  {
    updatePtr();
  }

  /**
   * @brief Constructs a DataVector using a buffer with a specified number of elements.
   * The provided number of elements must no larger than the number of elements in the buffer.
   * @param numElements
   * @param buffer
   */
  DataVector(size_type numElements, std::unique_ptr<value_type[]> buffer)
  : m_Size(numElements)
  , m_Data(std::move(buffer))
  {
    updatePtr();
  }

  template <typename InputIter>
  DataVector(InputIter begin, InputIter end)
  : m_Size(end - begin)
  , m_Data(std::make_unique<value_type[]>(m_Size))
  {
    std::copy(begin, end, m_Data.get());
    updatePtr();
  }

  /**
   * @brief Constructs a copy of the target DataVector
   * @param other
   */
  DataVector(const DataVector& other)
  : m_Size(other.m_Size)
  , m_Data(std::make_unique<value_type[]>(other.m_Size))
  {
    for(size_type i = 0; i < m_Size; i++)
    {
      m_Data.get()[i] = other.m_Data.get()[i];
    }
    updatePtr();
  }

  /**
   * @brief Move constructor
   * @param other
   */
  DataVector(DataVector&& other) noexcept
  : m_Size(other.m_Size)
  , m_Data(std::move(other.m_Data))
  , m_DataPtr(std::move(other.m_DataPtr))
  {
  }

  ~DataVector() noexcept = default;

  /**
   * @brief Returns the number of elements in the DataVector.
   * @return size_type
   */
  size_type size() const
  {
    return m_Size;
  }

  /**
   * @brief Resizes the DataVector and copies existing values where applicable.
   * @param numElements
   */
  void resize(size_type numElements)
  {
    if(numElements == m_Size)
    {
      return;
    }
    if(numElements == 0)
    {
      m_Data = nullptr;
      updatePtr();
      return;
    }

    auto newData = std::make_unique<value_type[]>(numElements);
    for(size_type i = 0; i < m_Size && i < numElements; i++)
    {
      newData.get()[i] = m_Data.get()[i];
    }
    m_Size = numElements;
    m_Data = std::move(newData);
    updatePtr();
  }

  /**
   * @brief Returns a pointer to the underlying data.
   * @return pointer
   */
  inline pointer data()
  {
    return m_DataPtr;
  }

  /**
   * @brief Returns a const pointer to the underlying data.
   */
  inline const_pointer data() const
  {
    return m_DataPtr;
  }

  /**
   * @brief Returns a reference to the value at the target index.
   * @param index
   * @return reference
   */
  inline reference operator[](size_type index)
  {
    return m_DataPtr[index];
  }

  /**
   * @brief Returns a const reference to the value at the target index.
   * @param index
   * @param const_reference
   */
  inline const_reference operator[](size_type index) const
  {
    return m_DataPtr[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataVector.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  inline const_reference at(size_type index) const
  {
    if(index >= m_Size)
    {
      throw std::runtime_error("Cannot reference value out of DataVect bounds");
    }
    return m_DataPtr[index];
  }

  /**
   * @brief Byteswaps all values in the DataVector.
   */
  inline void byteswap()
  {
    pointer rawPtr = data();
    for(value_type& value : *this)
    {
      value = nx::core::byteswap(value);
    }
  }

  /**
   * @brief Creates and returns a span wrapping the current data.
   * @return nonstd::span<T>
   */
  inline nonstd::span<T> createSpan()
  {
    return {data(), m_Size};
  }

  /**
   * @brief Creates and returns a span wrapping the current data.
   * @return nonstd::span<const T>
   */
  inline nonstd::span<const T> createSpan() const
  {
    return {data(), m_Size};
  }

  /**
   * @brief Returns an Iterator to the begining of the DataVector.
   * @return Iterator
   */
  inline iterator begin()
  {
    return iterator(*this, 0);
  }

  /**
   * @brief Returns an Iterator to the end of the DataVector.
   * @return Iterator
   */
  inline iterator end()
  {
    return iterator(*this, size());
  }

  /**
   * @brief Returns a ConstIterator to the begining of the DataVector.
   * @return ConstIterator
   */
  inline const_iterator begin() const
  {
    return const_iterator(*this, 0);
  }

  /**
   * @brief Returns a ConstIterator to the end of the DataVector.
   * @return ConstIterator
   */
  inline const_iterator end() const
  {
    return const_iterator(*this, size());
  }

  /**
   * @brief Returns a ConstIterator to the begining of the DataVector.
   * @return ConstIterator
   */
  inline const_iterator cbegin() const
  {
    return begin();
  }

  /**
   * @brief Returns a ConstIterator to the end of the DataVector.
   * @return ConstIterator
   */
  inline const_iterator cend() const
  {
    return end();
  }

  /**
   * @brief Copy operator
   * @param rhs
   * @return DataVector&
   */
  inline DataVector& operator=(const DataVector& rhs)
  {
    const size_type newSize = rhs.m_Size;
    auto newData = std::make_unique<value_type[]>(newSize);
    for(size_type i = 0; i < newSize; i++)
    {
      newData.get()[i] = rhs.m_Data.get()[i];
    }
    m_Size = newSize;
    m_Data = std::move(newData);
    updatePtr();

    return *this;
  }

  /**
   * @brief Move operator
   * @param rhs
   * @return DataVector&
   */
  inline DataVector& operator=(DataVector&& rhs) noexcept
  {
    m_Size = rhs.m_Size;
    m_Data = std::move(rhs.m_Data);
    updatePtr();

    return *this;
  }

protected:
  inline void updatePtr()
  {
    m_DataPtr = m_Data.get();
  }

private:
  size_type m_Size;
  std::unique_ptr<value_type[]> m_Data = nullptr;
  pointer m_DataPtr = nullptr;
};
} // namespace nx::core
