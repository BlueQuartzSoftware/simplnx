#pragma once

#include "complex/Common/Bit.hpp"
#include "complex/Common/IteratorUtility.hpp"
#include "complex/Common/Types.hpp"

#include "nonstd/span.hpp"

#include <memory>

namespace complex
{
template <typename T>
class DataVector
{
public:
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

    /**
     * @brief Default iterator required for some standard library algorithm implementations.
     */
    Iterator()
    : m_DataVector(nullptr)

    {
    }

    Iterator(DataVector& DataVector, uint64 index)
    : m_DataVector(&DataVector)
    , m_Index(index)
    {
    }

    Iterator(const Iterator&) = default;
    Iterator(Iterator&&) noexcept = default;

    Iterator& operator=(const Iterator&) = default;
    Iterator& operator=(Iterator&&) noexcept = default;

    ~Iterator() noexcept = default;

    bool isValid() const
    {
      if(m_DataVector == nullptr)
      {
        return false;
      }
      return m_Index < m_DataVector->size();
    }

    Iterator operator+(usize offset) const
    {
      return Iterator(*m_DataVector, m_Index + offset);
    }

    Iterator operator-(usize offset) const
    {
      return Iterator(*m_DataVector, m_Index - offset);
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
      return (*m_DataVector)[m_Index];
    }

    bool operator==(const Iterator& rhs) const
    {
      if(!isValid() && !rhs.isValid())
      {
        return true;
      }
      if(!isValid() || !rhs.isValid())
      {
        return false;
      }
      return (m_DataVector == rhs.m_DataVector) && (m_Index == rhs.m_Index);
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
    DataVector* m_DataVector = nullptr;
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

    /**
     * @brief Default iterator required for some standard library algorithm implementations.
     */
    ConstIterator()
    : m_DataVector(nullptr)

    {
    }

    ConstIterator(const DataVector& dataVector, uint64 index)
    : m_DataVector(&dataVector)
    , m_Index(index)
    {
    }

    ConstIterator(const ConstIterator&) = default;
    ConstIterator(ConstIterator&&) noexcept = default;

    ConstIterator& operator=(const ConstIterator&) = default;
    ConstIterator& operator=(ConstIterator&&) noexcept = default;

    ~ConstIterator() noexcept = default;

    bool isValid() const
    {
      if(m_DataVector == nullptr)
      {
        return false;
      }
      return m_Index < m_DataVector->size();
    }

    ConstIterator operator+(usize offset) const
    {
      return ConstIterator(*m_DataVector, m_Index + offset);
    }

    ConstIterator operator-(usize offset) const
    {
      return ConstIterator(*m_DataVector, m_Index - offset);
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

    reference operator*() const
    {
      return (*m_DataVector)[m_Index];
    }

    bool operator==(const ConstIterator& rhs) const
    {
      if(!isValid() && !rhs.isValid())
      {
        return true;
      }
      if(!isValid() || !rhs.isValid())
      {
        return false;
      }
      return (m_DataVector == rhs.m_DataVector) && (m_Index == rhs.m_Index);
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
    const DataVector* m_DataVector = nullptr;
    uint64 m_Index = 0;
  };
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
  DataVector(size_type numElements)
  : m_Size(numElements)
  , m_Data(std::make_unique<value_type[]>(numElements))
  {
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
  }

  /**
   * @brief Move constructor
   * @param other
   */
  DataVector(DataVector&& other) noexcept
  : m_Size(other.m_Size)
  , m_Data(std::move(other.m_Data))
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

    auto newData = std::make_unique<value_type[]>(numElements);
    for(size_type i = 0; i < m_Size && i < numElements; i++)
    {
      newData.get()[i] = m_Data.get()[i];
    }
    m_Size = numElements;
    m_Data = std::move(newData);
  }

  /**
   * @brief Returns a pointer to the underlying data.
   * @return pointer
   */
  pointer data()
  {
    return m_Data.get();
  }

  /**
   * @brief Returns a const pointer to the underlying data.
   */
  const_pointer data() const
  {
    return m_Data.get();
  }

  /**
   * @brief Returns a reference to the value at the target index.
   * @param index
   * @return reference
   */
  reference operator[](size_type index)
  {
    return m_Data.get()[index];
  }

  /**
   * @brief Returns a const reference to the value at the target index.
   * @param index
   * @param const_reference
   */
  const_reference operator[](size_type index) const
  {
    return m_Data.get()[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataVector.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  const_reference at(size_type index) const
  {
    if(index >= m_Size)
    {
      throw std::runtime_error("Cannot reference value out of DataVect bounds");
    }
    return m_Data.get()[index];
  }

  /**
   * @brief Byteswaps all values in the DataVector.
   */
  void byteswap()
  {
    pointer rawPtr = data();
    for(value_type& value : *this)
    {
      value = complex::byteswap(value);
    }
  }

  /**
   * @brief Creates and returns a span wrapping the current data.
   * @return nonstd::span<T>
   */
  nonstd::span<T> createSpan()
  {
    return {data(), m_Size};
  }

  /**
   * @brief Creates and returns a span wrapping the current data.
   * @return nonstd::span<const T>
   */
  nonstd::span<const T> createSpan() const
  {
    return {data(), m_Size};
  }

  /**
   * @brief Returns an Iterator to the begining of the DataVector.
   * @return Iterator
   */
  Iterator begin()
  {
    return Iterator(*this, 0);
  }

  /**
   * @brief Returns an Iterator to the end of the DataVector.
   * @return Iterator
   */
  Iterator end()
  {
    return Iterator(*this, size());
  }

  /**
   * @brief Returns a ConstIterator to the begining of the DataVector.
   * @return ConstIterator
   */
  ConstIterator begin() const
  {
    return ConstIterator(*this, 0);
  }

  /**
   * @brief Returns a ConstIterator to the end of the DataVector.
   * @return ConstIterator
   */
  ConstIterator end() const
  {
    return ConstIterator(*this, size());
  }

  /**
   * @brief Returns a ConstIterator to the begining of the DataVector.
   * @return ConstIterator
   */
  ConstIterator cbegin() const
  {
    return begin();
  }

  /**
   * @brief Returns a ConstIterator to the end of the DataVector.
   * @return ConstIterator
   */
  ConstIterator cend() const
  {
    return end();
  }

  /**
   * @brief Copy operator
   * @param rhs
   * @return DataVector&
   */
  DataVector& operator=(const DataVector& rhs)
  {
    const size_type newSize = rhs.m_Size;
    auto newData = std::make_unique<value_type[]>(newSize);
    for(size_type i = 0; i < newSize; i++)
    {
      newData.get()[i] = rhs.m_Data.get()[i];
    }
    m_Size = newSize;
    m_Data = std::move(newData);

    return *this;
  }

  /**
   * @brief Move operator
   * @param rhs
   * @return DataVector&
   */
  DataVector& operator=(DataVector&& rhs) noexcept
  {
    m_Size = rhs.m_Size;
    m_Data = std::move(rhs.m_Data);

    return *this;
  }

private:
  size_type m_Size;
  std::unique_ptr<value_type[]> m_Data = nullptr;
};
} // namespace complex
