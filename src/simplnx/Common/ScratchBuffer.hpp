#pragma once

#include "simplnx/Common/Types.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace nx::core
{
/**
 * @brief Value-initialized scratch storage replacing std::make_unique<T[]> buffers.
 *
 * GCC 13 reports false-positive -Wstringop-overflow / -Walloc-size-larger-than warnings on
 * new T[n]() for one-byte element types. Use std::vector<T> except for bool, since
 * std::vector<bool> cannot provide a raw buffer.
 * @tparam T Element type.
 */
template <class T>
class ScratchBuffer
{
public:
  ScratchBuffer() = default;

  explicit ScratchBuffer(usize count)
  : m_Data(count)
  {
  }

  ScratchBuffer(const ScratchBuffer&) = delete;
  ScratchBuffer& operator=(const ScratchBuffer&) = delete;
  ScratchBuffer(ScratchBuffer&&) noexcept = default;
  ScratchBuffer& operator=(ScratchBuffer&&) noexcept = default;

  T* data()
  {
    return m_Data.data();
  }

  const T* data() const
  {
    return m_Data.data();
  }

  usize size() const
  {
    return m_Data.size();
  }

  T& operator[](usize index)
  {
    return m_Data[index];
  }

  const T& operator[](usize index) const
  {
    return m_Data[index];
  }

  bool empty() const
  {
    return m_Data.empty();
  }

private:
  std::vector<T> m_Data;
};

/**
 * @brief Provides contiguous bool scratch storage without std::vector<bool> proxies.
 */
template <>
class ScratchBuffer<bool>
{
public:
  ScratchBuffer() = default;

  explicit ScratchBuffer(usize count)
  : m_Data(std::make_unique<bool[]>(count))
  , m_Size(count)
  {
  }

  ScratchBuffer(const ScratchBuffer&) = delete;
  ScratchBuffer& operator=(const ScratchBuffer&) = delete;

  ScratchBuffer(ScratchBuffer&& other) noexcept
  : m_Data(std::move(other.m_Data))
  , m_Size(std::exchange(other.m_Size, 0))
  {
  }

  ScratchBuffer& operator=(ScratchBuffer&& other) noexcept
  {
    if(this != &other)
    {
      m_Data = std::move(other.m_Data);
      m_Size = std::exchange(other.m_Size, 0);
    }
    return *this;
  }

  bool* data()
  {
    return m_Data.get();
  }

  const bool* data() const
  {
    return m_Data.get();
  }

  usize size() const
  {
    return m_Size;
  }

  bool& operator[](usize index)
  {
    return m_Data[index];
  }

  const bool& operator[](usize index) const
  {
    return m_Data[index];
  }

  bool empty() const
  {
    return m_Size == 0;
  }

private:
  std::unique_ptr<bool[]> m_Data;
  usize m_Size = 0;
};
} // namespace nx::core
