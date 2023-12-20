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
  class Iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = T*;
    using reference = T&;

    Iterator()
    : m_DataVector(nullptr)
    , m_Index(0)
    {
    }

    Iterator(DataVector& DataVector, uint64 index)
    : m_DataVector(DataVector)
    , m_Index(index)
    {
    }

    Iterator(const Iterator& other)
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    Iterator(Iterator&& other) noexcept
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    Iterator& operator=(const Iterator& rhs)
    {
      m_DataVector = rhs.m_DataVector;
      m_Index = rhs.m_Index;
      return *this;
    }

    Iterator& operator=(Iterator&& rhs) noexcept
    {
      m_DataVector = rhs.m_DataVector;
      m_Index = rhs.m_Index;
      return *this;
    }

    ~Iterator() noexcept = default;

    inline bool isValid() const
    {
      return m_Index < m_DataVector->size();
    }

    inline Iterator operator+(usize offset) const
    {
      return Iterator(m_DataVector, m_Index + offset);
    }

    inline Iterator operator-(usize offset) const
    {
      return Iterator(m_DataVector, m_Index - offset);
    }

    inline Iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    inline Iterator& operator-=(usize offset)
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
      return m_DataVector[m_Index];
    }

    inline bool operator==(const Iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const Iterator& rhs) const
    {
      return m_Index != rhs.m_Index;
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
    DataVector& m_DataVector;
    uint64 m_Index = 0;
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
    : m_DataVector(nullptr)
    , m_Index(0)
    {
    }
    ConstIterator(ConstIterator&& other) noexcept
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    ConstIterator(const DataVector& dataVector, uint64 index)
    : m_DataVector(dataVector)
    , m_Index(index)
    {
      m_Index = rhs.m_Index;
      return *this;
    }
    ConstIterator& operator=(ConstIterator&& rhs) noexcept
    {
      m_Index = rhs.m_Index;
      return *this;
    }

    ConstIterator(const ConstIterator& other)
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }
    ConstIterator(ConstIterator&& other) noexcept
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    ConstIterator& operator=(const ConstIterator& rhs)
    {
      m_DataVector = rhs.m_DataVector;
      m_Index = rhs.m_Index;
      return *this;
    }
    ConstIterator& operator=(ConstIterator&& rhs) noexcept
    {
      m_DataVector = rhs.m_DataVector;
      m_Index = rhs.m_Index;
      return *this;
    }

    ~ConstIterator() noexcept = default;

    inline bool isValid() const
    {
      return m_Index < m_DataVector->size();
    }

    inline ConstIterator operator+(usize offset) const
    {
      return ConstIterator(m_DataVector, m_Index + offset);
    }

    inline ConstIterator operator-(usize offset) const
    {
      return ConstIterator(m_DataVector, m_Index - offset);
    }

    inline ConstIterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    inline ConstIterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    // prefix
    inline ConstIterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    inline ConstIterator operator++(int)
    {
      Iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    inline ConstIterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    inline ConstIterator operator--(int)
    {
      ConstIterator iter = *this;
      m_Index--;
      return iter;
    }

    inline difference_type operator-(const ConstIterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return m_DataVector[m_Index];
    }

    inline bool operator==(const ConstIterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const ConstIterator& rhs) const
    {
      return m_Index != rhs.m_Index;
    }

    inline bool operator<(const ConstIterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    inline bool operator>(const ConstIterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    inline bool operator<=(const ConstIterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    inline bool operator>=(const ConstIterator& rhs) const
    {
      return m_Index >= rhs.m_Index;
    }

  private:
    const DataVector& m_DataVector;
    uint64 m_Index = 0;
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

    Iterator(DataVector& DataVector, uint64 index)
    : m_DataVector(DataVector)
    , m_Index(index)
    {
    }

    Iterator(const Iterator& other)
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }

    Iterator(Iterator&& other) noexcept
    : m_DataVector(other.m_DataVector)
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
      return m_Index < m_DataVector.size();
    }

    inline Iterator operator+(usize offset) const
    {
      return Iterator(m_DataVector, m_Index + offset);
    }

    inline Iterator operator-(usize offset) const
    {
      return Iterator(m_DataVector, m_Index - offset);
    }

    inline Iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    inline Iterator& operator-=(usize offset)
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
      return m_DataVector[m_Index];
    }

    inline bool operator==(const Iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const Iterator& rhs) const
    {
      return m_Index != rhs.m_Index;
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
    DataVector& m_DataVector;
    uint64 m_Index = 0;
  };

  class ConstIterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int64;
    using pointer = const T*;
    using reference = const T&;

    ConstIterator(const DataVector& dataVector, uint64 index)
    : m_DataVector(dataVector)
    , m_Index(index)
    {
    }

    ConstIterator(const ConstIterator& other)
    : m_DataVector(other.m_DataVector)
    , m_Index(other.m_Index)
    {
    }
    ConstIterator(ConstIterator&& other) noexcept
    : m_DataVector(other.m_DataVector)
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

    inline bool isValid() const
    {
      return m_Index < m_DataVector.size();
    }

    inline ConstIterator operator+(usize offset) const
    {
      return ConstIterator(m_DataVector, m_Index + offset);
    }

    inline ConstIterator operator-(usize offset) const
    {
      return ConstIterator(m_DataVector, m_Index - offset);
    }

    inline ConstIterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    inline ConstIterator& operator-=(usize offset)
    {
      m_Index -= offset;
      return *this;
    }

    // prefix
    inline ConstIterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    inline ConstIterator operator++(int)
    {
      Iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    inline ConstIterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
    inline ConstIterator operator--(int)
    {
      ConstIterator iter = *this;
      m_Index--;
      return iter;
    }

    inline difference_type operator-(const ConstIterator& rhs) const
    {
      return m_Index - rhs.m_Index;
    }

    inline reference operator*() const
    {
      return m_DataVector[m_Index];
    }

    inline bool operator==(const ConstIterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const ConstIterator& rhs) const
    {
      return m_Index != rhs.m_Index;
    }

    inline bool operator<(const ConstIterator& rhs) const
    {
      return m_Index < rhs.m_Index;
    }

    inline bool operator>(const ConstIterator& rhs) const
    {
      return m_Index > rhs.m_Index;
    }

    inline bool operator<=(const ConstIterator& rhs) const
    {
      return m_Index <= rhs.m_Index;
    }

    inline bool operator>=(const ConstIterator& rhs) const
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
  inline Iterator begin()
  {
    return Iterator(*this, 0);
  }

  /**
   * @brief Returns an Iterator to the end of the DataVector.
   * @return Iterator
   */
  inline Iterator end()
  {
    return Iterator(*this, size());
  }

  /**
   * @brief Returns a ConstIterator to the begining of the DataVector.
   * @return ConstIterator
   */
  inline ConstIterator begin() const
  {
    return ConstIterator(*this, 0);
  }

  /**
   * @brief Returns a ConstIterator to the end of the DataVector.
   * @return ConstIterator
   */
  inline ConstIterator end() const
  {
    return ConstIterator(*this, size());
  }

  /**
   * @brief Returns a ConstIterator to the begining of the DataVector.
   * @return ConstIterator
   */
  inline ConstIterator cbegin() const
  {
    return begin();
  }

  /**
   * @brief Returns a ConstIterator to the end of the DataVector.
   * @return ConstIterator
   */
  inline ConstIterator cend() const
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
