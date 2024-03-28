#pragma once

#include "simplnx/DataStructure/AbstractDataStore.hpp"

#include <fmt/core.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <vector>

namespace nx::core
{
/**
 * @class DataStore
 * @brief The DataStore class handles the storing and retrieval of data for
 * use in DataArrays.
 * @tparam T
 */
template <typename T>
class DataStore : public AbstractDataStore<T>
{
public:
  using parent_type = AbstractDataStore<T>;
  using value_type = typename AbstractDataStore<T>::value_type;
  using reference = typename AbstractDataStore<T>::reference;
  using const_reference = typename AbstractDataStore<T>::const_reference;
  using ShapeType = typename IDataStore::ShapeType;

  static constexpr const char k_DataStore[] = "DataStore";
  static constexpr const char k_DataObjectId[] = "DataObjectId";
  static constexpr const char k_DataArrayTypeName[] = "DataArray";

  /**
   * @brief Creates a new DataStore with a single tuple dimensions of 'numTuples' and
   * a single component dimension of {1}
   * @param numTuples
   * @param initValue
   */
  DataStore(usize numTuples, std::optional<T> initValue)
  : DataStore({numTuples}, {1}, initValue)
  {
  }

  /**
   * @brief Constructs a DataStore with the specified tupleSize and tupleCount.
   * @param tupleShape The dimensions of the tuples
   * @param componentShape The dimensions of the component at each tuple
   * @param initValue
   */
  DataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue)
  : parent_type()
  , m_ComponentShape(componentShape)
  , m_TupleShape(tupleShape)
  , m_NumComponents(std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<size_t>(1), std::multiplies<>()))
  , m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>()))
  {
    resizeTuples(m_TupleShape);
    if(initValue.has_value())
    {
      std::fill_n(data(), this->getSize(), *initValue);
    }
  }

  /**
   * @brief Constructs a DataStore from an existing buffer.
   * @param buffer
   * @param tupleShape
   * @param componentShape
   */
  DataStore(std::unique_ptr<value_type[]> buffer, ShapeType tupleShape, ShapeType componentShape)
  : parent_type()
  , m_ComponentShape(std::move(componentShape))
  , m_TupleShape(std::move(tupleShape))
  , m_Data(std::move(buffer))
  , m_NumComponents(std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<size_t>(1), std::multiplies<>()))
  , m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>()))
  {
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  DataStore(const DataStore& other)
  : parent_type()
  , m_ComponentShape(other.m_ComponentShape)
  , m_TupleShape(other.m_TupleShape)
  , m_NumComponents(other.m_NumComponents)
  , m_NumTuples(other.m_NumTuples)
  {
    const usize count = other.getSize();
    auto* data = new value_type[count];
    std::memcpy(data, other.m_Data.get(), count * sizeof(T));
    m_Data.reset(data);
  }

  /**
   * @brief Move constructor
   * @param other
   */
  DataStore(DataStore&& other) noexcept
  : parent_type()
  , m_ComponentShape(std::move(other.m_ComponentShape))
  , m_TupleShape(std::move(other.m_TupleShape))
  , m_Data(std::move(other.m_Data))
  , m_NumComponents(std::move(other.m_NumComponents))
  , m_NumTuples(std::move(other.m_NumTuples))
  {
  }

  /**
   * @brief Copy assignment.
   * @param rhs
   * @return
   */
  DataStore& operator=(const DataStore& rhs) = delete;

  /**
   * @brief Move assignment.
   * @param rhs
   * @return
   */
  DataStore& operator=(DataStore&& rhs) = default;

  ~DataStore() override = default;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return usize
   */
  usize getNumberOfTuples() const override
  {
    return m_NumTuples;
  }

  /**
   * @brief Returns the pointer to the allocated data. Const version
   * @return
   */
  const T* data() const
  {
    return m_Data.get();
  }

  /**
   * @brief Returns the pointer to the allocated data. Non-const version
   * @return
   */
  T* data()
  {
    return m_Data.get();
  }

  /**
   * @brief Returns the number of elements in each Tuple.
   * @return usize
   */
  usize getNumberOfComponents() const override
  {
    return m_NumComponents;
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
   * @brief Returns the dimensions of the Components
   * @return
   */
  const ShapeType& getComponentShape() const override
  {
    return m_ComponentShape;
  }

  /**
   * @brief Returns the store type e.g. in memory, out of core, etc.
   * @return StoreType
   */
  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::InMemory;
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
  void resizeTuples(const std::vector<usize>& tupleShape) override
  {
    auto oldSize = this->getSize();
    // Calculate the total number of values in the new array
    m_TupleShape = tupleShape;
    m_NumTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());

    usize newSize = getNumberOfComponents() * m_NumTuples;

    if(m_Data.get() == nullptr) // Data was never allocated
    {
      auto data = new value_type[newSize];
      m_Data.reset(data);
      return;
    }

    // The caller is reshaping the array without actually effecting its overall number
    // of elements. Old was 100 x 3 and the new was 300. Both with a {1} comp dim.
    if(newSize == oldSize)
    {
      return;
    }

    // We have now figured out that the old array and the new array are different sizes so
    // copy the old data into the newly allocated data array or as much or as little
    // as possible
    auto data = new value_type[newSize];
    for(usize i = 0; i < newSize && i < oldSize; i++)
    {
      data[i] = m_Data.get()[i];
    }
    m_Data.reset(data);
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return value_type
   */
  value_type getValue(usize index) const override
  {
    return m_Data.get()[index];
  }

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  void setValue(usize index, value_type value) override
  {
    m_Data.get()[index] = value;
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param  index
   * @return const_reference
   */
  const_reference operator[](usize index) const override
  {
    return m_Data.get()[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return reference
   */
  reference operator[](usize index) override
  {
    return m_Data.get()[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  const_reference at(usize index) const override
  {
    if(index >= this->getSize())
    {
      throw std::runtime_error("");
    }
    return m_Data.get()[index];
  }

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> deepCopy() const override
  {
    return std::make_unique<DataStore<T>>(*this);
  }

  /**
   * @brief Returns a data store of the same type as this but with default initialized data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return std::make_unique<DataStore<T>>(this->getTupleShape(), this->getComponentShape(), static_cast<T>(0));
  }

  nonstd::span<T> createSpan()
  {
    return {data(), this->getSize()};
  }

  nonstd::span<const T> createSpan() const
  {
    return {data(), this->getSize()};
  }

  std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const override
  {
    std::ofstream outStrm(absoluteFilePath, std::ios_base::out | std::ios_base::binary);
    if(!outStrm.is_open())
    {
      return {-10170, fmt::format("File could not be opened for writing:\n  '{}'", absoluteFilePath)};
    }

    return writeBinaryFile(outStrm);
  }

  std::pair<int32, std::string> writeBinaryFile(std::ostream& outputStream) const override
  {
    usize totalElements = getNumberOfComponents() * getNumberOfTuples();

    outputStream.write(reinterpret_cast<char*>(m_Data.get()), sizeof(T) * totalElements);

    if(outputStream.bad())
    {
      return {-10175, fmt::format("Error writing binary file:\n  Total Elements:'{}'\n", totalElements)};
    }

    return {0, ""};
  }

private:
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
  std::unique_ptr<value_type[]> m_Data = nullptr;
  size_t m_NumComponents = {0};
  size_t m_NumTuples = {0};
};

// Declare aliases
using UInt8DataStore = DataStore<uint8>;
using UInt16DataStore = DataStore<uint16>;
using UInt32DataStore = DataStore<uint32>;
using UInt64DataStore = DataStore<uint64>;

using Int8DataStore = DataStore<int8>;
using Int16DataStore = DataStore<int16>;
using Int32DataStore = DataStore<int32>;
using Int64DataStore = DataStore<int64>;

using BoolDataStore = DataStore<bool>;

using Float32DataStore = DataStore<float32>;
using Float64DataStore = DataStore<float64>;
} // namespace nx::core
