#pragma once

#include "simplnx/Common/Types.hpp"

#include <xtensor/xarray.hpp>
#include <xtensor/xlayout.hpp>

#include <mutex>
#include <ostream>
#include <string>
#include <utility>

namespace nx::core
{
template <class T>
class AbstractListStore
{
public:
  /////////////////////////////////
  // Reference Wrapper for lists //
  /////////////////////////////////
  class ConstReferenceList
  {
  public:
    using vector_type = std::vector<T>;
    using const_iterator = typename vector_type::const_iterator;

    ConstReferenceList(const AbstractListStore<T>& store, usize tupleIndex)
    : m_ListStore(store)
    , m_List(store.at(tupleIndex))
    , m_Index(tupleIndex)
    {
    }
    ~ConstReferenceList()
    {
    }

    const T& operator[](usize i) const
    {
      return m_List[i];
    }
    const T& at(usize i) const
    {
      return m_List.at(i);
    }

    usize size() const
    {
      return m_List.size();
    }

    const vector_type& vector() const
    {
      return m_List;
    }

    const_iterator begin() const
    {
      return m_List.begin();
    }
    const_iterator end() const
    {
      return m_List.end();
    }
    const_iterator cbegin() const
    {
      return m_List.begin();
    }
    const_iterator cend() const
    {
      return m_List.end();
    }

  private:
    vector_type m_List;
    usize m_Index = 0;
    const AbstractListStore<T>& m_ListStore;
  };

  class ReferenceList
  {
  public:
    using vector_type = std::vector<T>;
    using iterator = typename vector_type::iterator;
    using const_iterator = typename vector_type::const_iterator;

    ReferenceList(AbstractListStore<T>& store, usize tupleIndex)
    : m_ListStore(store)
    , m_List(store.at(tupleIndex))
    , m_Index(tupleIndex)
    {
    }
    ReferenceList(ReferenceList&& other)
    : m_ListStore(std::move(other.m_ListStore))
    , m_List(std::move(other.m_List))
    , m_Index(std::move(other.m_Index))
    , m_Edited(std::move(m_Edited))
    {
    }
    ~ReferenceList()
    {
      if(m_Edited)
      {
        m_ListStore.setList(m_Index, m_List);
      }
    }

    T& operator[](usize i)
    {
      m_Edited = true;
      return m_List[i];
    }
    const T& operator[](usize i) const
    {
      return m_List[i];
    }
    const T& at(usize i) const
    {
      return m_List.at(i);
    }

    ReferenceList& operator=(const ReferenceList& rhs)
    {
      m_Edited = true;
      m_List = rhs.m_List;
      return *this;
    }
    ReferenceList& operator=(const ConstReferenceList& rhs)
    {
      m_Edited = true;
      m_List = rhs.vector();
      return *this;
    }
    ReferenceList& operator=(const std::vector<T>& rhs)
    {
      m_Edited = true;
      m_List = rhs;
      return *this;
    }
    ReferenceList& operator=(ReferenceList&& rhs)
    {
      m_Edited = std::move(rhs.m_Edited);
      m_List = std::move(rhs.m_List);
      m_Index = std::move(rhs.m_Index);
      return *this;
    }
    constexpr void swap(ReferenceList& rhs) noexcept
    {
      m_Edited = true;
      rhs.m_Edited = true;

      std::swap(m_Edited, rhs.m_Edited);
      std::swap(m_List, rhs.m_List);
      std::swap(m_Index, rhs.m_Index);
    }

    usize size() const
    {
      return m_List.size();
    }

    const vector_type& vector() const
    {
      return m_List;
    }

    iterator begin()
    {
      m_Edited = true;
      return m_List.begin();
    }
    iterator end()
    {
      m_Edited = true;
      return m_List.end();
    }
    const_iterator begin() const
    {
      return m_List.begin();
    }
    const_iterator end() const
    {
      return m_List.end();
    }
    const_iterator cbegin() const
    {
      return m_List.begin();
    }
    const_iterator cend() const
    {
      return m_List.end();
    }

  private:
    bool m_Edited = false;
    vector_type m_List;
    usize m_Index = 0;
    AbstractListStore<T>& m_ListStore;
  };

  ///////////////
  // iterators //
  ///////////////
  class iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = ReferenceList;
    using difference_type = int64;
    using pointer = ReferenceList*;
    using reference = ReferenceList;
    using const_reference = const ReferenceList;

    iterator()
    : m_DataStore(nullptr)
    , m_Index(0)
    {
    }

    iterator(AbstractListStore& dataStore, usize index)
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

    inline bool isValid() const
    {
      return m_Index < m_DataStore->size();
    }

    inline iterator operator+(usize offset) const
    {
      return iterator(*m_DataStore, m_Index + offset);
    }

    inline iterator operator-(usize offset) const
    {
      return iterator(*m_DataStore, m_Index - offset);
    }

    inline iterator& operator+=(usize offset)
    {
      m_Index += offset;
      return *this;
    }

    iterator& operator-=(usize offset)
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

    constexpr void swap(iterator& rhs) noexcept
    {
      std::swap(m_Index, rhs.m_Index);
    }

    inline reference operator*() const
    {
      return ReferenceList(*m_DataStore, m_Index);
    }

    inline bool operator==(const iterator& rhs) const
    {
      return m_Index == rhs.m_Index;
    }

    inline bool operator!=(const iterator& rhs) const
    {
      return !(*this == rhs);
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
    AbstractListStore* m_DataStore;
    usize m_Index = 0;
  };

  class const_iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = ConstReferenceList;
    using difference_type = int64;
    using pointer = const ConstReferenceList*;
    using reference = const ConstReferenceList;

    const_iterator()
    : m_DataStore(nullptr)
    , m_Index(0)
    {
    }

    const_iterator(const AbstractListStore& dataStore, usize index)
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

    // prefix
    const_iterator& operator++()
    {
      m_Index++;
      return *this;
    }

    // postfix
    const_iterator operator++(int)
    {
      iterator iter = *this;
      m_Index++;
      return iter;
    }

    // prefix
    const_iterator& operator--()
    {
      m_Index--;
      return *this;
    }

    // postfix
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

    inline reference operator*() const
    {
      return ConstReferenceList(*m_DataStore, m_Index);
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
    const AbstractListStore* m_DataStore = nullptr;
    usize m_Index = 0;
  };

  using value_type = T;
  using vector_type = std::vector<T>;
  using shared_vector_type = typename std::shared_ptr<vector_type>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using xarray_type = typename xt::xarray<value_type>;

  virtual ~AbstractListStore() = default;

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
    if(listSize + 1 >= interalListSize)
    {
      interalListSize = listSize + 2;
      setXtensorListSize(interalListSize);
    }

    std::lock_guard<std::mutex> guard(m_Mutex);
    uint64 offset = (grainId * interalListSize); // First element is list size
    listSize++;

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

  usize getListSize(usize grainId) const
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
    return iterator(*this, 0);
  }
  iterator end()
  {
    return iterator(*this, size());
  }
  const_iterator begin() const
  {
    return const_iterator(*this, 0);
  }
  const_iterator end() const
  {
    return const_iterator(*this, size());
  }
  const_iterator cbegin() const
  {
    return const_iterator(*this, 0);
  }
  const_iterator cend() const
  {
    return const_iterator(*this, size());
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
    for(const auto& list : lists)
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

  /**
   * @brief Write to stream
   * @param out
   */
  virtual void write(std::ostream& out) const = 0;

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
    xarray().flat(offset) = static_cast<T>(size);
  }

private:
};

} // namespace nx::core

template <typename T>
constexpr void swap(typename nx::core::AbstractListStore<T>::ReferenceList& first, typename nx::core::AbstractListStore<T>::ReferenceList& second) noexcept
{
  first.swap(second);
}
template <typename T>
constexpr void swap(typename nx::core::AbstractListStore<T>::ReferenceList first, typename nx::core::AbstractListStore<T>::ReferenceList second) noexcept
{
  first.swap(second);
}

template <typename T>
constexpr void swap(typename nx::core::AbstractListStore<T>::iterator& first, typename nx::core::AbstractListStore<T>::iterator& second) noexcept
{
  first.swap(second);
}
