#pragma once

#include "simplnx/DataStructure/AbstractListStore.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

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

  /**
   * @brief Constructs a ListStore using the specified tuple shape and list size.
   * @param tupleShape
   * @param listSize
   */
  explicit ListStore(const ShapeType& tupleShape)
  : parent_type()
  , m_TupleShape(tupleShape.cbegin(), tupleShape.cend())
  , m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>()))
  , m_Array(m_NumTuples)
  {
  }

  /**
   * @brief Creates a ListStore from a vector of vectors.
   * @param vectors
   */
  explicit ListStore(const typename std::vector<shared_vector_type>& vectors)
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
  , m_TupleShape(other.m_TupleShape)
  , m_NumTuples(other.m_NumTuples)
  {
  }

  /**
   * @brief Move constructor
   */
  ListStore(ListStore&& copy) noexcept
  : parent_type(std::move(copy))
  , m_Array(std::move(copy.m_Array))
  , m_TupleShape(std::move(copy.m_TupleShape))
  , m_NumTuples(copy.m_NumTuples)
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
   * @brief Returns the number of tuples in the ListStore.
   * @return usize
   */
  usize getNumberOfTuples() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief Returns the dimensions of the Tuples
   * @return
   */
  const ShapeType& getTupleShape() const override
  {
    return m_TupleShape;
  }

  /**
   * @brief This method sets the shape of the dimensions to `tupleShape`.
   * @param tupleShape The new shape of the data where the dimensions are "C" ordered
   * from *slowest* to *fastest*.
   */
  void resizeTuples(const ShapeType& tupleShape) override
  {
    m_TupleShape = tupleShape;
    m_NumTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
    m_Array.resize(m_NumTuples);
  }

  /**
   * @brief Adds a new entry to the list at the specified grain/tuple index.
   * @param grainId The grain/tuple index to add the entry to
   * @param value The value to add to the list
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
   * @brief Sets the complete list for the specified grain/tuple index using a shared pointer to a vector.
   * @param grainId The grain/tuple index to set the list for
   * @param neighborList Shared pointer to the vector containing the list values
   */
  void setList(int32 grainId, const shared_vector_type& neighborList) override
  {
    setList(grainId, *neighborList);
  }

  /**
   * @brief Sets the complete list for the specified grain/tuple index using a vector.
   * @param grainId The grain/tuple index to set the list for
   * @param neighborList Vector containing the list values
   */
  void setList(int32 grainId, const vector_type& neighborList) override
  {
    m_Array[grainId] = neighborList;
  }

  /**
   * @brief Returns a copy of the list for the specified grain/tuple.
   * @param grainId The grain/tuple index to retrieve
   * @return vector_type A copy of the list vector
   */
  vector_type getList(int32 grainId) const override
  {
    return copyOfList(grainId);
  }

  /**
   * @brief Returns the total number of lists in the list store.
   * Alias for getNumberOfLists().
   * @return usize The number of lists
   */
  usize size() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief Returns the number of elements in the list at the specified grain/tuple index.
   * @param grainId The grain/tuple index to query
   * @return usize The number of elements in the specified list
   */
  usize getListSize(usize grainId) const override
  {
    return m_Array[grainId].size();
  }

  /**
   * @brief Returns a deep copy of the list for the specified grain/tuple.
   * @param grainId The grain/tuple index to copy
   * @return vector_type A deep copy of the list vector
   */
  vector_type copyOfList(int32 grainId) const override
  {
    return at(grainId);
  }

  /**
   * @brief Retrieves a specific value from a grain's list with bounds checking.
   * @param grainId The grain/tuple index to retrieve from
   * @param index The element index within the grain's list
   * @param ok Output parameter set to true if the value was successfully retrieved, false otherwise
   * @return T The value at the specified position, or default value if out of bounds
   */
  T getValue(int32 grainId, int32 index, bool& ok) const override
  {
    if(grainId < this->getNumberOfLists() && grainId >= 0 && index >= 0 && index < m_Array[grainId].size())
    {
      ok = true;
      return m_Array[grainId][index];
    }

    ok = false;
    return {};
  }

  /**
   * @brief Sets the value at a specific position within a grain's list with bounds checking.
   * @param grainId The grain/tuple index
   * @param index The element index within the grain's list
   * @param value The value to set
   */
  void setValue(int32 grainId, usize index, T value) override
  {
    if(grainId < this->getNumberOfLists() && grainId >= 0 && index < m_Array[grainId].size())
    {
      m_Array[grainId][index] = value;
    }
  }

  /**
   * @brief Returns the total number of lists in the ListStore.
   * @return usize The number of lists (equal to the number of tuples)
   */
  usize getNumberOfLists() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief Array subscript operator to access the list at the specified grain/tuple index.
   * @param grainId The grain/tuple index to access
   * @return vector_type A copy of the list at the specified index
   */
  vector_type operator[](int32 grainId) const override
  {
    return m_Array[grainId];
  }

  /**
   * @brief Array subscript operator to access the list at the specified grain/tuple index.
   * @param grainId The grain/tuple index to access
   * @return vector_type A copy of the list at the specified index
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

  /**
   * @brief Clears all lists from the list store.
   */
  void clear() override
  {
    m_Array.clear();
  }

  /**
   * @brief Sets all lists from a vector of shared pointers to vectors.
   * @param lists Vector of shared pointers to vectors containing the list data
   */
  void setData(const std::vector<shared_vector_type>& lists) override
  {
    m_NumTuples = lists.size();
    m_TupleShape = ShapeType{m_NumTuples};
    m_Array.resize(m_NumTuples);
    for(usize i = 0; i < m_Array.size(); i++)
    {
      m_Array[i] = *lists[i];
    }
  }

  /**
   * @brief Sets all lists from a vector of vectors.
   * @param lists Vector of vectors containing the list data
   */
  void setData(const std::vector<vector_type>& lists) override
  {
    m_Array = lists;
    m_NumTuples = m_Array.size();
    m_TupleShape = ShapeType{m_NumTuples};
  }

  /**
   * @brief Throws an error because ListStore cannot read from HDF5.
   * @param datasetReader The HDF5 DatasetIO (unused)
   * @throw std::runtime_error Always throws because HDF5 read is not implemented
   */
  void readHdf5(const HDF5::DatasetIO& datasetReader) override
  {
    throw std::runtime_error("ListStore cannot read from HDF5");
  }

  /**
   * @brief Throws an error because ListStore cannot write to HDF5.
   * @param datasetReader The HDF5 DatasetIO (unused)
   * @throw std::runtime_error Always throws because HDF5 write is not implemented
   */
  void writeHdf5(HDF5::DatasetIO& datasetReader) override
  {
    throw std::runtime_error("ListStore cannot write to HDF5");
  }

private:
  ShapeType m_TupleShape;
  ShapeType::value_type m_NumTuples;
  std::vector<std::vector<T>> m_Array;
};

using UInt8ListStore = ListStore<uint8>;
using UInt16ListStore = ListStore<uint16>;
using UInt32ListStore = ListStore<uint32>;
using UInt64ListStore = ListStore<uint64>;

using Int8ListStore = ListStore<int8>;
using Int16ListStore = ListStore<int16>;
using Int32ListStore = ListStore<int32>;
using Int64ListStore = ListStore<int64>;

using Float32ListStore = ListStore<float32>;
using Float64ListStore = ListStore<float64>;
} // namespace nx::core
