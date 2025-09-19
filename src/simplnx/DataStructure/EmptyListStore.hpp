#pragma once

#include "AbstractListStore.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

namespace nx::core
{
template <class T>
class EmptyListStore : public AbstractListStore<T>
{
public:
  using value_type = T;
  using parent_type = AbstractListStore<T>;
  using vector_type = typename parent_type::vector_type;
  using shared_vector_type = typename parent_type::shared_vector_type;

  EmptyListStore() = default;
  /**
   * @brief Constructs a ListStore using the specified tuple shape and list size.
   * @param tupleShape
   * @param listSize
   */
  EmptyListStore(const ShapeType& tupleShape)
  : AbstractListStore<T>()
  , m_TupleShape(tupleShape.cbegin(), tupleShape.cend())
  , m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>()))
  {
  }

  EmptyListStore(const EmptyListStore& rhs) = default;
  EmptyListStore(EmptyListStore&& rhs) = default;

  ~EmptyListStore() override = default;

  /**
   * @brief Returns the number of tuples in the EmptyListStore.
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
   * @brief Returns a copy of the current list store.
   * @return std::unique<AbstractListStore<T>>
   */
  std::unique_ptr<parent_type> deepCopy() const override
  {
    return std::make_unique<EmptyListStore>(*this);
  }

  /**
   * @brief addEntry
   * @param grainId
   * @param value
   */
  void addEntry(int32 grainId, value_type value) override
  {
    throw std::runtime_error("EmptyListStore cannot add values to list");
  }

  void resizeTuples(const ShapeType& tupleShape) override
  {
    m_TupleShape = tupleShape;
    m_NumTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
  }

  void clear() override
  {
    m_NumTuples = 0;
  }

  /**
   * @brief Clear All Lists
   */
  void clearAllLists() override
  {
    m_TupleShape = ShapeType{0};
    m_NumTuples = 0;
  }

  uint64 getNumberOfLists() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief getList
   * @param grainId
   * @return shared_vector_type
   */
  vector_type getList(int32 grainId) const override
  {
    return {};
  }

  /**
   * @brief copyOfList
   * @param grainId
   * @return vector_type
   */
  vector_type copyOfList(int32 grainId) const override
  {
    return {};
  }

  /**
   * @brief setList
   * @param grainId
   * @param neighborList
   */
  void setList(int32 grainId, const shared_vector_type& neighborList) override
  {
    throw std::runtime_error("EmptyListStore cannot set list values");
  }

  /**
   * @brief setList
   * @param grainId
   * @param neighborList
   */
  void setList(int32 grainId, const vector_type& neighborList) override
  {
    throw std::runtime_error("EmptyListStore cannot set list values");
  }

  usize getListSize(usize grainId) const override
  {
    return 0;
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
    throw std::runtime_error("EmptyListStore cannot get list value");
  }

  void setValue(int32 grainId, usize index, T value) override
  {
    throw std::runtime_error("EmptyListStore cannot set list value");
  }

  /**
   * @brief operator []
   * @param grainId
   * @return vector_type&
   */
  vector_type operator[](int32 grainId) const override
  {
    throw std::runtime_error("EmptyListStore cannot get list");
  }

  /**
   * @brief operator []
   * @param grainId
   * @return vector_type&
   */
  vector_type operator[](usize grainId) const override
  {
    throw std::runtime_error("EmptyListStore cannot get list");
  }

  /**
   * @brief Returns a const reference to the vector_type value found at the specified index. This cannot be used to edit the vector_type value found at the specified index.
   * @param grainId
   * @return vector_type
   */
  vector_type at(int32 grainId) const override
  {
    throw std::runtime_error("EmptyListStore cannot get list");
  }

  /**
   * @brief Returns a const reference to the vector_type value found at the specified index. This cannot be used to edit the vector_type value found at the specified index.
   * @param grainId
   * @return vector_type
   */
  vector_type at(usize grainId) const override
  {
    throw std::runtime_error("EmptyListStore cannot get list");
  }

  void setData(const std::vector<shared_vector_type>& lists) override
  {
    throw std::runtime_error("EmptyListStore cannot set lists");
  }

  void setData(const std::vector<vector_type>& lists) override
  {
    throw std::runtime_error("EmptyListStore cannot set lists");
  }

  void readHdf5(const HDF5::DatasetIO& datasetReader) override
  {
    auto tupleDimsResult = datasetReader.readVectorAttribute<uint64>("TupleDimensions");
    if(tupleDimsResult.invalid())
    {
      clear();
    }
    else
    {
      resizeTuples(tupleDimsResult.value());
    }
  }

  void writeHdf5(HDF5::DatasetIO& datasetReader) override
  {
    throw std::runtime_error("EmptyListStore cannot write to HDF5");
  }

private:
  ShapeType m_TupleShape;
  usize m_NumTuples = 0;
};
} // namespace nx::core
