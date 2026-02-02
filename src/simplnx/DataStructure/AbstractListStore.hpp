#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/IListStore.hpp"

#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace nx::core
{
template <class T>
class AbstractListStore : public IListStore
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
    ~ConstReferenceList() = default;

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

    bool operator==(const ConstReferenceList& rhs) const
    {
      if(m_List.size() != rhs.m_List.size())
      {
        return false;
      }
      for(usize i = 0; i < m_List.size(); i++)
      {
        if(m_List[i] != rhs.m_List[i])
        {
          return false;
        }
      }
      return true;
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
    ReferenceList(ReferenceList&& other) noexcept
    : m_ListStore(std::move(other.m_ListStore))
    , m_List(std::move(other.m_List))
    , m_Index(other.m_Index)
    , m_Edited(other.m_Edited)
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
      m_Edited = rhs.m_Edited;
      m_List = std::move(rhs.m_List);
      m_Index = rhs.m_Index;
      return *this;
    }
    constexpr void swap(ReferenceList& rhs) noexcept
    {
      m_Edited = true;
      rhs.m_Edited = true;

      std::swap(m_List, rhs.m_List);
      std::swap(m_Index, rhs.m_Index);
    }

    friend void swap(ReferenceList lhs, ReferenceList rhs) noexcept
    {
      lhs.swap(rhs);
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
      ReferenceList first(*m_DataStore, m_Index);
      ReferenceList second(*rhs.m_DataStore, rhs.m_Index);
      first.swap(second);
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

  ~AbstractListStore() override = default;

  /**
   * @brief Creates a deep copy of this AbstractListStore.
   * @return std::unique_ptr<AbstractListStore> Unique pointer to the deep copy
   */
  virtual std::unique_ptr<AbstractListStore> deepCopy() const = 0;

  /**
   * @brief Adds a new entry to the list at the specified grain/tuple index.
   * @param grainId The grain/tuple index to add the entry to
   * @param value The value to add to the list
   */
  virtual void addEntry(int32 grainId, value_type value) = 0;

  /**
   * @brief Sets the complete list for the specified grain/tuple index using a shared pointer to a vector.
   * @param grainId The grain/tuple index to set the list for
   * @param neighborList Shared pointer to the vector containing the list values
   */
  virtual void setList(int32 grainId, const shared_vector_type& neighborList) = 0;

  /**
   * @brief Sets the complete list for the specified grain/tuple index using a vector.
   * @param grainId The grain/tuple index to set the list for
   * @param neighborList Vector containing the list values
   */
  virtual void setList(int32 grainId, const vector_type& neighborList) = 0;

  /**
   * @brief Returns a copy of the list for the specified grain/tuple.
   * @param grainId The grain/tuple index to retrieve
   * @return vector_type A copy of the list vector
   */
  virtual vector_type getList(int32 grainId) const = 0;

  /**
   * @brief Returns a deep copy of the list for the specified grain/tuple.
   * @param grainId The grain/tuple index to copy
   * @return vector_type A deep copy of the list vector
   */
  virtual vector_type copyOfList(int32 grainId) const = 0;

  /**
   * @brief Retrieves a specific value from a grain's list.
   * @param grainId The grain/tuple index to retrieve from
   * @param index The element index within the grain's list
   * @param ok Output parameter set to true if the value was successfully retrieved, false otherwise
   * @return T The value at the specified position
   */
  virtual T getValue(int32 grainId, int32 index, bool& ok) const = 0;

  /**
   * @brief Sets the value at a specific position within a grain's list.
   * @param grainId The grain/tuple index
   * @param index The element index within the grain's list
   * @param value The value to set
   */
  virtual void setValue(int32 grainId, usize index, T value) = 0;

  /**
   * @brief Array subscript operator to access the list at the specified grain/tuple index.
   * @param grainId The grain/tuple index to access
   * @return vector_type A copy of the list at the specified index
   */
  virtual vector_type operator[](int32 grainId) const = 0;

  /**
   * @brief Array subscript operator to access the list at the specified grain/tuple index.
   * @param grainId The grain/tuple index to access
   * @return vector_type A copy of the list at the specified index
   */
  virtual vector_type operator[](usize grainId) const = 0;

  /**
   * @brief Returns a const reference to the vector_type value found at the specified index. This cannot be used to edit the vector_type value found at the specified index.
   * @param grainId
   * @return vector_type
   */
  virtual vector_type at(int32 grainId) const = 0;

  /**
   * @brief Returns a const reference to the vector_type value found at the specified index. This cannot be used to edit the vector_type value found at the specified index.
   * @param grainId
   * @return vector_type
   */
  virtual vector_type at(usize grainId) const = 0;

  /**
   * @brief Returns an iterator to the beginning of the list store.
   * @return iterator Iterator to the first list
   */
  iterator begin()
  {
    return iterator(*this, 0);
  }

  /**
   * @brief Returns an iterator to the end of the list store.
   * @return iterator Iterator past the last list
   */
  iterator end()
  {
    return iterator(*this, size());
  }

  /**
   * @brief Returns a const iterator to the beginning of the list store.
   * @return const_iterator Const iterator to the first list
   */
  const_iterator begin() const
  {
    return const_iterator(*this, 0);
  }

  /**
   * @brief Returns a const iterator to the end of the list store.
   * @return const_iterator Const iterator past the last list
   */
  const_iterator end() const
  {
    return const_iterator(*this, size());
  }

  /**
   * @brief Returns a const iterator to the beginning of the list store.
   * @return const_iterator Const iterator to the first list
   */
  const_iterator cbegin() const
  {
    return const_iterator(*this, 0);
  }

  /**
   * @brief Returns a const iterator to the end of the list store.
   * @return const_iterator Const iterator past the last list
   */
  const_iterator cend() const
  {
    return const_iterator(*this, size());
  }

  /**
   * @brief Sets all lists from a vector of shared pointers to vectors.
   * @param lists Vector of shared pointers to vectors containing the list data
   */
  virtual void setData(const std::vector<shared_vector_type>& lists) = 0;

  /**
   * @brief Sets all lists from a vector of vectors.
   * @param lists Vector of vectors containing the list data
   */
  virtual void setData(const std::vector<vector_type>& lists) = 0;

protected:
  /**
   * @brief Default constructor.
   */
  AbstractListStore() = default;

  /**
   * @brief Copy constructor.
   * @param rhs The AbstractListStore to copy from
   */
  AbstractListStore(const AbstractListStore& rhs) = default;

  /**
   * @brief Move constructor.
   * @param rhs The AbstractListStore to move from
   */
  AbstractListStore(AbstractListStore&& rhs) = default;
};

} // namespace nx::core

template <typename T>
void swap(typename nx::core::AbstractListStore<T>::ReferenceList& first, typename nx::core::AbstractListStore<T>::ReferenceList& second) noexcept
{
  first.swap(second);
}
template <typename T>
void swap(typename nx::core::AbstractListStore<T>::ReferenceList first, typename nx::core::AbstractListStore<T>::ReferenceList second) noexcept
{
  first.swap(second);
}

template <typename T>
void swap(typename nx::core::AbstractListStore<T>::iterator& first, typename nx::core::AbstractListStore<T>::iterator& second) noexcept
{
  first.swap(second);
}
