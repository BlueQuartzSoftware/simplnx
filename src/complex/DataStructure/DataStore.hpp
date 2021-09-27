#pragma once

#include "complex/DataStructure/IDataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"

#include <fmt/core.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace complex
{
/**
 * @class DataStore
 * @brief The DataStore class handles the storing and retrieval of data for
 * use in DataArrays.
 * @tparam T
 */
template <typename T>
class DataStore : public IDataStore<T>
{
public:
  using value_type = typename IDataStore<T>::value_type;
  using reference = typename IDataStore<T>::reference;
  using const_reference = typename IDataStore<T>::const_reference;
  using ShapeType = typename std::vector<size_t>;

  static constexpr const char k_DataStore[] = "DataStore";
  static constexpr const char k_TupleShape[] = "TupleShape";
  static constexpr const char k_ComponentShape[] = "ComponentShape";
  static constexpr const char k_DataObjectId[] = "DataObjectId";
  static constexpr const char k_DataArrayTypeName[] = "DataArray";

  /**
   * @brief Creates a new DataStore with a single tuple dimensions of 'numTuples' and
   * a single component dimension of {1}
   * @param numTuples
   */
  DataStore(size_t tupleSize, size_t tupleCount)
  : m_NumComponents(tupleSize)
  , m_TupleCount(tupleCount)
  , m_Data(std::make_unique<value_type[]>(tupleSize * tupleCount))
  {
    reshapeTuples(m_TupleShape);
  }

  /**
   * @brief Constructs a DataStore with the specified tupleSize and tupleCount.
   * @param tupleShape The dimensions of the tuples
   * @param componentShape The dimensions of the component at each tuple
   */
  DataStore(size_t tupleSize, size_t tupleCount, std::unique_ptr<value_type[]>&& data)
  : m_NumComponents(tupleSize)
  , m_TupleCount(tupleCount)
  , m_Data(std::move(data))
  {
    reshapeTuples(tupleShape); // This should allocate the memory for this DataStore
  }

  //  /**
  //   * @brief Constructs a DataStore with the specified tupleSize and tupleCount.
  //   * @param num_tuples
  //   * @param data
  //   */
  //  DataStore(size_t num_tuples, std::unique_ptr<value_type[]>&& data)
  //  : m_TupleShape(num_tuples)
  //  , m_Data(std::move(data))
  //  {
  //  }

  /**
   * @brief Copy constructor
   * @param other
   */
  DataStore(const DataStore& other)
  : m_TupleShape(other.m_TupleShape)
  , m_ComponentShape(other.m_ComponentShape)
  {
    const usize count = getNumberOfTuples() * getNumberOfComponents();
    for(usize i = 0; i < count; i++)
    {
      m_Data[i] = other.m_Data[i];
    }
  }

  /**
   * @brief Move constructor
   * @param other
   */
  DataStore(DataStore&& other) noexcept
  : m_TupleShape(std::move(other.m_TupleShape))
  , m_ComponentShape(std::move(other.m_ComponentShape))
  , m_Data(std::move(other.m_Data))
  {
  }

  ~DataStore() override = default;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return usize
   */
  usize getNumberOfTuples() const override
  {
    return std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
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
    return std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<size_t>(1), std::multiplies<>());
  }

  /**
   * @brief Returns the dimensions of the Tuples
   * @return
   */
  const ShapeType& getTupleShape() const
  {
    return m_TupleShape;
  }

  /**
   * @brief Returns the dimensions of the Components
   * @return
   */
  const ShapeType& getComponentShape() const
  {
    return m_ComponentShape;
  }

  //  /**
  //   * @brief Resizes the DataStore to handle the specified number of tuples.
  //   * @param numTuples
  //   */
  //  void resizeTuples(size_t numTuples) override
  //  {
  //    auto oldSize = this->getSize();
  //    m_TupleShape = numTuples;
  //    auto newSize = this->getSize();
  //
  //    auto data = new value_type[newSize];
  //    for(size_t i = 0; i < newSize && i < oldSize; i++)
  //    {
  //      data[i] = m_Data[i];
  //    }
  //
  //    m_Data.reset(data);
  //  }

  /**
   * @brief
   * @param tupleShape
   */
  void resizeTuples(usize numTuples) override
  {
    auto oldSize = this->getSize();
    // Calculate the total number of values in the new array
    size_t newSize = getNumberOfComponents() * std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
    m_TupleShape = tupleShape;

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
      data[i] = m_Data[i];
    }
    m_Data.reset(data);
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return value_type
   */
  typename IDataStore<T>::value_type getValue(usize index) const override
  {
    return m_Data[index];
  }

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  void setValue(usize index, value_type value) override
  {
    m_Data[index] = value;
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param  index
   * @return const_reference
   */
  typename IDataStore<T>::const_reference operator[](usize index) const override
  {
    return m_Data[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return reference
   */
  typename IDataStore<T>::reference operator[](usize index) override
  {
    return m_Data[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  typename IDataStore<T>::const_reference at(usize index) const override
  {
    if(index >= IDataStore<T>::getSize())
      if(index >= this->getSize())
      {
        throw std::runtime_error("");
      }
    return m_Data[index];
  }

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return IDataStore*
   */
  IDataStore<T>* deepCopy() const override
  {
    auto copy = new DataStore(*this);
    for(usize i = 0; i < IDataStore<T>::getSize(); i++)
      for(usize i = 0; i < this->getSize(); i++)
      {
        copy->setValue(i, getValue(i));
      }
    return copy;
  }

  /**
   * @brief Writes the data store to HDF5. Returns the HDF5 error code should
   * one be encountered. Otherwise, returns 0.
   * @param dataId
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::IdType dataId, const std::string& name, DataObject::IdType objectId) const override
  {
    hsize_t rank = m_TupleShape.size() + m_ComponentShape.size();
    // Consolodate the Tuple and Component Dims into a single array which is used
    // to write the entire data array to HDF5
    std::vector<hsize_t> h5dims;
    for(const auto& value : m_TupleShape)
    {
      h5dims.push_back(static_cast<hsize_t>(value));
    }
    for(const auto& value : m_ComponentShape)
    {
      h5dims.push_back(static_cast<hsize_t>(value));
    }
    herr_t err = complex::H5::Support::WritePointerDataset(dataId, name, rank, h5dims.data(), m_Data.get());
    if(err < 0)
    {
      return err;
    }
    // Write the attributes to the dataset
    err = complex::H5::Support::WriteVectorAttribute(dataId, name, k_TupleShape, {m_TupleShape.size()}, m_TupleShape);
    if(err < 0)
    {
      return err;
    }
    err = complex::H5::Support::WriteVectorAttribute(dataId, name, k_ComponentShape, {m_ComponentShape.size()}, m_ComponentShape);
    if(err < 0)
    {
      return err;
    }

    err = complex::H5::Support::WriteScalarAttribute(dataId, name, k_DataObjectId, objectId);
    if(err < 0)
    {
      return err;
    }
    err = complex::H5::Support::WriteStringAttribute(dataId, name, complex::Constants::k_ObjectTypeTag, k_DataArrayTypeName);
    if(err < 0)
    {
      return err;
    }

    return err;
  }

  static DataStore* readHdf5(H5::IdType parentId, const std::string& datasetName)
  {
    typename DataStore<T>::ShapeType tupleShape;
    typename DataStore<T>::ShapeType componentShape;
    //
    herr_t err = H5::Support::ReadVectorAttribute(parentId, datasetName, k_TupleShape, tupleShape);
    if(err < 0)
    {
    }
    size_t numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
    //
    err = H5::Support::ReadVectorAttribute(parentId, datasetName, k_ComponentShape, componentShape);
    if(err < 0)
    {
    }
    size_t numComponents = std::accumulate(componentShape.cbegin(), componentShape.cend(), static_cast<size_t>(1), std::multiplies<>());
    //
    auto dataStore = new complex::DataStore<T>(tupleShape, componentShape);
    err = H5::Support::ReadPointerDataset(parentId, datasetName, dataStore->data());
    if(err < 0)
    {
      delete dataStore;
      throw std::runtime_error(fmt::format("Error reading DataStore from HDF5 at {}/{}", H5::Support::GetObjectPath(parentId), datasetName));
      return nullptr;
    }

    return dataStore;
  }

private:
  ShapeType m_ComponentShape;
  ShapeType m_TupleShape;
  std::unique_ptr<value_type[]> m_Data;
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

using USizeDataStore = DataStore<usize>;

using Float32DataStore = DataStore<float32>;
using Float64DataStore = DataStore<float64>;
} // namespace complex
