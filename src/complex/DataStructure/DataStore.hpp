#pragma once

#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5AttributeReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include <fmt/core.h>

#include <algorithm>
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
class DataStore : public AbstractDataStore<T>
{
public:
  using value_type = typename AbstractDataStore<T>::value_type;
  using reference = typename AbstractDataStore<T>::reference;
  using const_reference = typename AbstractDataStore<T>::const_reference;
  using ShapeType = typename IDataStore::ShapeType;

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
  DataStore(usize numTuples)
  : m_ComponentShape({1})
  , m_TupleShape({numTuples})
  {
    reshapeTuples(m_TupleShape);
  }

  /**
   * @brief Constructs a DataStore with the specified tupleSize and tupleCount.
   * @param tupleShape The dimensions of the tuples
   * @param componentShape The dimensions of the component at each tuple
   */
  DataStore(const ShapeType& tupleShape, const ShapeType& componentShape)
  : m_ComponentShape(componentShape)
  , m_TupleShape(tupleShape)
  {
    reshapeTuples(m_TupleShape);
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  DataStore(const DataStore& other)
  : m_TupleShape(other.m_TupleShape)
  , m_ComponentShape(other.m_ComponentShape)
  {
    const usize count = other.getSize();
    auto data = new value_type[count];
    for(usize i = 0; i < count; i++)
    {
      data[i] = other.m_Data.get()[i];
    }
    m_Data.reset(data);
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
  void reshapeTuples(const std::vector<usize>& tupleShape) override
  {
    auto oldSize = this->getSize();
    // Calculate the total number of values in the new array
    usize newSize = getNumberOfComponents() * std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
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
  value_type getValue(usize index) const override
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
  const_reference operator[](usize index) const override
  {
    return m_Data[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return reference
   */
  reference operator[](usize index) override
  {
    return m_Data[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  const_reference at(usize index) const override
  {
    if(index >= IDataStore::getSize())
      if(index >= this->getSize())
      {
        throw std::runtime_error("");
      }
    return m_Data[index];
  }

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> deepCopy() const override
  {
    auto copy = std::make_unique<DataStore<T>>(*this);
    auto size = this->getSize();
    for(usize i = 0; i < size; i++)
    {
      copy->setValue(i, getValue(i));
    }
    return copy;
  }

  /**
   * @brief Returns a data store of the same type as this but with default initialized data.
   * @return std::unique_ptr<IDataStore>
   */
  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return std::make_unique<DataStore<T>>(this->getTupleShape(), this->getComponentShape());
  }

  /**
   * @brief Writes the data store to HDF5. Returns the HDF5 error code should
   * one be encountered. Otherwise, returns 0.
   * @param datasetWriter
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DatasetWriter& datasetWriter) const override
  {
    if(!datasetWriter.isValid())
    {
      return -1;
    }

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

    usize count = this->getSize();
    const T* dataPtr = data();
    herr_t err = datasetWriter.writeSpan(h5dims, nonstd::span<const T>{dataPtr, count});
    if(err < 0)
    {
      return err;
    }

    // Write shape attributes to the dataset
    auto tupleAttribute = datasetWriter.createAttribute(k_TupleShape);
    err = tupleAttribute.writeVector({m_TupleShape.size()}, m_TupleShape);
    if(err < 0)
    {
      return err;
    }

    auto componentAttribute = datasetWriter.createAttribute(k_ComponentShape);
    err = componentAttribute.writeVector({m_ComponentShape.size()}, m_ComponentShape);

    return err;
  }

  static std::unique_ptr<DataStore> readHdf5(const H5::DatasetReader& datasetReader)
  {
    // tupleShape
    H5::AttributeReader tupleShapeAttribute = datasetReader.getAttribute(k_TupleShape);
    if(!tupleShapeAttribute.isValid())
    {
      throw std::runtime_error(fmt::format("Error reading DataStore from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
    }
    typename DataStore<T>::ShapeType tupleShape = tupleShapeAttribute.readAsVector<size_t>();

    // componentShape
    H5::AttributeReader componentShapeAttribute = datasetReader.getAttribute(k_ComponentShape);
    if(!componentShapeAttribute.isValid())
    {
      throw std::runtime_error(fmt::format("Error reading DataStore from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
    }
    typename DataStore<T>::ShapeType componentShape = componentShapeAttribute.readAsVector<size_t>();

    // Create DataStore
    auto dataStore = std::make_unique<DataStore<T>>(tupleShape, componentShape);

    auto count = dataStore->getSize();
    auto dataVector = datasetReader.readAsVector<T>();
    auto dataPtr = std::make_unique<value_type[]>(count);
    std::copy(dataVector.begin(), dataVector.end(), dataPtr.get());
    dataStore->m_Data = std::move(dataPtr);

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
