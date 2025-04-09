#pragma once

#include "simplnx/DataStructure/AbstractListStore.hpp"

#include <memory>
#include <vector>

namespace nx::core
{
template <typename T>
class ListStore : public AbstractListStore<T>
{
public:
  using parent_type = AbstractListStore<T>;
  using value_type = T;
  using vector_type = typename parent_type::vector_type;
  using shared_vector_type = typename parent_type::shared_vector_type;
  using reference = typename parent_type::reference;
  using const_reference = typename parent_type::const_reference;
  using iterator = typename parent_type::iterator;
  using const_iterator = typename parent_type::const_iterator;
  using shape_type = typename std::vector<usize>;

  /**
   * @brief Constructs a ListStore using the specified tuple shape and list size.
   * @param tupleShape
   * @param listSize
   */
  ListStore(usize numTuples)
  : parent_type()
  , m_Array(numTuples)
  {
  }

  /**
   * @brief Creates a ListStore from a vector of vectors.
   * @param vectors
   */
  ListStore(const typename std::vector<shared_vector_type>& vectors)
  : parent_type()
  {
    setData(vectors);
  }

  /**
   * @brief Copy constructor
   */
  ListStore(const ListStore& other)
  : parent_type(other)
  , m_Array(other.m_Array)
  {
  }

  /**
   * @brief Move constructor
   */
  ListStore(ListStore&& copy) noexcept
  : parent_type(std::move(copy))
  , m_Array(std::move(copy.m_Array))
  {
  }

  ~ListStore() override = default;

  /**
   * @brief Returns a copy of the current list store.
   * @return std::unique<AbstractListStore<T>>
   */
  std::unique_ptr<parent_type> deepCopy() const override
  {
    return std::make_unique<ListStore>(*this);
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
  void resizeTuples(usize tupleCount) override
  {
    m_Array.resize(tupleCount);
  }

  /**
   * @brief addEntry
   * @param grainId
   * @param value
   */
  void addEntry(int32 grainId, value_type value) override
  {
    m_Array[grainId].push_back(value);
  }

  /**
   * @brief Clear All Lists
   */
  void clearAllLists() override
  {
    m_Array.clear();
  }

  /**
   * @brief setList
   * @param grainId
   * @param neighborList
   */
  void setList(int32 grainId, const shared_vector_type& neighborList) override
  {
    setList(grainId, *neighborList);
  }

  /**
   * @brief setList
   * @param grainId
   * @param neighborList
   */
  void setList(int32 grainId, const vector_type& neighborList) override
  {
    m_Array[grainId] = neighborList;
  }

  /**
   * @brief getList
   * @param grainId
   * @return shared_vector_type
   */
  vector_type getList(int32 grainId) const override
  {
    return copyOfList(grainId);
  }

  usize getListSize(usize grainId) const override
  {
    return m_Array[grainId].size();
  }

  /**
   * @brief copyOfList
   * @param grainId
   * @return vector_type
   */
  vector_type copyOfList(int32 grainId) const override
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
  T getValue(int32 grainId, int32 index, bool& ok) const override
  {
    if(grainId >= this->getNumberOfLists() || grainId < 0 || index < 0)
    {
      ok = false;
      return {};
    }

    auto& list = m_Array[grainId];
    if(index > list.size())
    {
      ok = false;
      return {};
    }

    ok = true;
    return list[index];
  }

  void setValue(int32 grainId, usize index, T value) override
  {
    if(grainId >= this->getNumberOfLists())
    {
      return;
    }

    m_Array[grainId][index] = value;
  }

  uint64 getNumberOfLists() const override
  {
    return m_Array.size();
  }

  /**
   * @brief operator []
   * @param grainId
   * @return vector_type&
   */
  vector_type operator[](int32 grainId) const override
  {
    return m_Array[grainId];
  }

  /**
   * @brief operator []
   * @param grainId
   * @return vector_type&
   */
  vector_type operator[](usize grainId) const override
  {
    return m_Array[grainId];
  }

  /**
   * @brief Returns a const reference to the vector_type value found at the specified index. This cannot be used to edit the vector_type value found at the specified index.
   * @param grainId
   * @return vector_type
   */
  vector_type at(int32 grainId) const override
  {
    return this->operator[](grainId);
  }

  /**
   * @brief Returns a const reference to the vector_type value found at the specified index. This cannot be used to edit the vector_type value found at the specified index.
   * @param grainId
   * @return vector_type
   */
  vector_type at(usize grainId) const override
  {
    return this->operator[](grainId);
  }

  void clear() override
  {
    m_Array.clear();
  }

  void setData(const std::vector<shared_vector_type>& lists) override
  {
    m_Array.resize(lists.size());
    for(usize i = 0; i < m_Array.size(); i++)
    {
      m_Array[i] = *lists[i];
    }
  }

  void setData(const std::vector<vector_type>& lists) override
  {
    m_Array = lists;
  }

private:
  std::vector<std::vector<T>> m_Array;
};
} // namespace nx::core
