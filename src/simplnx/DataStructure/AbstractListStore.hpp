#pragma once

#include "simplnx/Common/Types.hpp"

#include <xtensor/xarray.hpp>
#include <xtensor/xlayout.hpp>

#include <mutex>
#include <string>

namespace nx::core
{
template <class T>
class AbstractListStore
{
public:
  using value_type = T;
  using vector_type = std::vector<T>;
  using shared_vector_type = typename std::shared_ptr<vector_type>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using xarray_type = typename xt::xarray<value_type>;
  using iterator = typename xarray_type::iterator;
  using const_iterator = typename xarray_type::const_iterator;

  ~AbstractListStore() = default;

  virtual xarray_type& xarray() = 0;
  virtual const xarray_type& xarray() const = 0;

  virtual std::unique_ptr<AbstractListStore> deepCopy() const = 0;

  /**
   * @brief resizeTotalElements
   * @param size
   * @return int32
   */
  virtual int32 resizeTotalElements(usize size)
  {
    resize({size}, xtensorListSize());
    return 1;
  }

  /**
   * @brief This method sets the shape of the dimensions to `tupleShape`.
   *
   * There are 3 possibilities when using this function:
   * [1] The number of tuples of the new shape is *LESS* than the original. In this
   * case a memory allocation will take place and the first 'N' elements of data
   * will be copied into the new array. The remaining data is *LOST*
   *
   * [2] The number of tuples of the new shape is *EQUAL* to the original. In this
   * case the shape is set and the function returns.
   *
   * [3] The number of tuples of the new shape is *GREATER* than the original. In
   * this case a new array is allocated and all the data from the original array
   * is copied into the new array and the remaining elements are initialized to
   * the default initialization value.
   *
   * @param tupleShape The new shape of the data where the dimensions are "C" ordered
   * from *slowest* to *fastest*.
   */
  virtual void resizeTuples(usize tupleCount)
  {
    resize({tupleCount}, xtensorListSize());
  }

  /**
   * @brief addEntry
   * @param grainId
   * @param value
   */
  virtual void addEntry(int32 grainId, value_type value)
  {
    usize listSize = getListSize(grainId);
    usize interalListSize = xtensorListSize();
    if(listSize >= interalListSize + 1)
    {
      interalListSize *= 2;
      setXtensorListSize(interalListSize);
    }

    uint64 offset = (grainId * interalListSize); // First element is list size
    listSize++;

    std::lock_guard<std::mutex> guard(m_Mutex);
    auto& xarr = xarray();
    xarr.flat(offset) = listSize;
    xarr.flat(offset + listSize) = value;
  }

  /**
   * @brief Clear All Lists
   */
  virtual void clearAllLists()
  {
    std::lock_guard<std::mutex> guard(m_Mutex);

    uint64 count = xtensorListSize();
    uint64 numLists = getNumberOfLists();
    auto& xarr = xarray();
    for(uint64 i = 0; i < numLists; i++)
    {
      uint64 offset = i * (count + 1); // First element is list size
      xarr.flat(offset) = 0;
    }
  }

  /**
   * @brief setList
   * @param grainId
   * @param neighborList
   */
  virtual void setList(int32 grainId, const shared_vector_type& neighborList)
  {
    setList(grainId, *neighborList.get());
  }

  /**
   * @brief setList
   * @param grainId
   * @param neighborList
   */
  virtual void setList(int32 grainId, const vector_type& neighborList)
  {
    uint64 count = xtensorListSize();
    uint64 neighborListSize = neighborList.size();

    if(count < neighborListSize + 1)
    {
      count = neighborListSize + 1;
      setXtensorListSize(count);
    }

    uint64 offset = grainId * xtensorListSize() + 1; // First element is list size
    auto& xarr = xarray();
    setListSize(grainId, neighborListSize);

    std::lock_guard<std::mutex> guard(m_Mutex);
    for(uint64 i = 0; i < neighborListSize; i++)
    {
      xarr.flat(offset + i) = neighborList[i];
    }
  }

  /**
   * @brief getList
   * @param grainId
   * @return shared_vector_type
   */
  virtual vector_type getList(int32 grainId) const
  {

    return copyOfList(grainId);
  }

  usize getListSize(uint64 grainId) const
  {
    auto offset = grainId * xtensorListSize();
    return xarray().flat(offset);
  }

  /**
   * @brief copyOfList
   * @param grainId
   * @return vector_type
   */
  virtual vector_type copyOfList(int32 grainId) const
  {
    return at(grainId);
  }

  /**
   * @brief getValue
   * @param grainId
   * @param index
   * @param ok
   * @return T
   */
  virtual T getValue(int32 grainId, int32 index, bool& ok) const
  {
    if(grainId >= getNumberOfLists() || grainId < 0 || index < 0)
    {
      ok = false;
      return {};
    }

    auto list = at(grainId);
    if(index > list.size())
    {
      ok = false;
      return {};
    }

    ok = true;
    return list[index];
  }

  virtual void setValue(int32 grainId, usize index, T value)
  {
    if(grainId >= getNumberOfLists())
    {
      return;
    }

    std::lock_guard<std::mutex> guard(m_Mutex);
    uint64 offset = (grainId * xtensorListSize()) + 1; // First element is list size
    xarray().flat(offset + index) = value;
  }

  /**
   * @brief getNumberOfLists
   * @return int32
   */
  virtual uint64 getNumberOfLists() const
  {
    return std::accumulate(m_TupleShape.begin(), m_TupleShape.end(), static_cast<usize>(1), std::multiplies<usize>());
  }

  virtual uint64 size() const
  {
    return getNumberOfLists();
  }

  /**
   * @brief getListSize
   * @param grainId
   * @return int32
   */
  virtual int32 getListSize(int32 grainId) const
  {
    uint64 offset = (grainId * xtensorListSize()); // First element is list size
    return xarray().flat(offset);
  }

  /**
   * @brief operator []
   * @param grainId
   * @return vector_type&
   */
  vector_type operator[](int32 grainId) const
  {
    uint64 count = getListSize(grainId);
    vector_type output(count);
    uint64 offset = (grainId * xtensorListSize()) + 1; // First element is list size
    for(uint64 i = 0; i < count; i++)
    {
      output[i] = xarray().flat(i + offset);
    }
    return output;
  }

  /**
   * @brief operator []
   * @param grainId
   * @return vector_type&
   */
  vector_type operator[](usize grainId) const
  {
    uint64 count = getListSize(grainId);
    vector_type output(count);
    uint64 offset = (grainId * xtensorListSize()) + 1; // First element is list size
    for(uint64 i = 0; i < count; i++)
    {
      output[i] = xarray().flat(i + offset);
    }
    return output;
  }

  /**
   * @brief Returns a const reference to the vector_type value found at the specified index. This cannot be used to edit the vector_type value found at the specified index.
   * @param grainId
   * @return vector_type
   */
  virtual vector_type at(int32 grainId) const
  {
    std::lock_guard<std::mutex> guard(m_Mutex);
    return this->operator[](grainId);
  }

  /**
   * @brief Returns a const reference to the vector_type value found at the specified index. This cannot be used to edit the vector_type value found at the specified index.
   * @param grainId
   * @return vector_type
   */
  virtual vector_type at(usize grainId) const
  {
    std::lock_guard<std::mutex> guard(m_Mutex);
    return this->operator[](grainId);
  }

  virtual void resizeTuples(std::vector<usize> tupleShape)
  {
    resize(tupleShape, xtensorListSize());
  }

  iterator begin()
  {
    return xarray().begin();
  }
  iterator end()
  {
    return xarray().end();
  }
  const_iterator begin() const
  {
    return xarray().begin();
  }
  const_iterator end() const
  {
    return xarray().end();
  }
  const_iterator cbegin() const
  {
    return xarray().begin();
  }
  const_iterator cend() const
  {
    return xarray().end();
  }

  virtual void setData(const std::vector<shared_vector_type>& lists)
  {
    usize count = lists.size();
    usize maxSize = 0;
    for(const auto& list : lists)
    {
      maxSize = std::max(maxSize, list->size());
    }
    setSize({count}, maxSize);
    for(usize i = 0; i < count; i++)
    {
      setList(i, lists[i]);
    }
  }

  virtual void setData(const std::vector<vector_type>& lists)
  {
    usize count = lists.size();
    usize maxSize = 0;
    for (const auto& list : lists)
    {
      maxSize = std::max(maxSize, list.size());
    }
    setSize({count}, maxSize);
    for(usize i = 0; i < count; i++)
    {
      setList(i, lists[i]);
    }
  }

  AbstractListStore& operator=(const std::vector<shared_vector_type>& lists)
  {
    setData(lists);
    return *this;
  }

  AbstractListStore& operator=(const std::vector<vector_type>& lists)
  {
    setData(lists);
    return *this;
  }

  /**
   * @brief Sets the internal xtensor list dimension and resizes the xtensor array.
   * @param size
   */
  virtual void setXtensorListSize(usize size)
  {
    resize(m_TupleShape, size);
  }

  /**
   * @brief Clears the xtensor array.
   */
  virtual void clear()
  {
    setXtensorListSize(0);
  }

  void copy(const AbstractListStore& other)
  {
    setSize(other.m_TupleShape, other.xtensorListSize());
    const usize count = getNumberOfLists() * xtensorListSize();
    auto& xarr = xarray();
    auto& xarr2 = other.xarray();
    for(usize i = 0; i < count; ++i)
    {
      auto value = xarr2.flat(i);
      xarr.flat(i) = value;
    }
  }

protected:
  std::vector<usize> m_TupleShape;
  mutable std::mutex m_Mutex;

  AbstractListStore() = default;
  AbstractListStore(const AbstractListStore& rhs)
  : m_TupleShape(rhs.m_TupleShape)
  {
  }
  AbstractListStore(AbstractListStore&& rhs)
  : m_TupleShape(std::move(rhs.m_TupleShape))
  {
  }

  virtual usize xtensorListSize() const = 0;
  virtual void resize(std::vector<usize> tupleShape, usize internalSize) = 0;
  virtual void setSize(std::vector<usize> tupleShape, usize internalSize) = 0;
  void setListSize(uint64 grainId, uint64 size)
  {
    std::lock_guard<std::mutex> guard(m_Mutex);

    uint64 internalCount = xtensorListSize();
    uint64 offset = grainId * (internalCount); // First element is list size
    xarray().flat(offset) = size;
  }

private:
};
} // namespace nx::core
