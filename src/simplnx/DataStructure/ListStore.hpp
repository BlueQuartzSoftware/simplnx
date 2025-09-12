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
  using ShapeType = typename std::vector<usize>;

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
    if(grainId < this->getNumberOfLists() && grainId >= 0 && index >= 0 && index < m_Array[grainId].size())
    {
      ok = true;
      return m_Array[grainId][index];
    }

    ok = false;
    return {};
  }

  void setValue(int32 grainId, usize index, T value) override
  {
    if(grainId < this->getNumberOfLists() && grainId >= 0 && index < m_Array[grainId].size())
    {
      m_Array[grainId][index] = value;
    }
  }

  uint64 getNumberOfLists() const override
  {
    return m_NumTuples;
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
    m_NumTuples = lists.size();
    m_TupleShape = ShapeType{m_NumTuples};
    m_Array.resize(m_NumTuples);
    for(usize i = 0; i < m_Array.size(); i++)
    {
      m_Array[i] = *lists[i];
    }
  }

  void setData(const std::vector<vector_type>& lists) override
  {
    m_Array = lists;
    m_NumTuples = m_Array.size();
    m_TupleShape = ShapeType{m_NumTuples};
  }

  void readHdf5(const HDF5::DatasetIO& datasetReader) override
  {
    throw std::runtime_error("ListStore cannot read from HDF5");
  }

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
