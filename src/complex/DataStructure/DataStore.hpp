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

  /**
   * @brief Creates a new DataStore with a single tuple dimensions of 'numTuples' and
   * a single component dimension of {1}
   * @param numTuples
   */
  DataStore(size_t numTuples)
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
    const size_t count = getNumberOfTuples() * getNumberOfComponents() ;
    for(size_t i = 0; i < count; i++)
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

  virtual ~DataStore() = default;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return size_t
   */
  size_t getNumberOfTuples() const override
  {
    return std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
  }

  /**
   * @brief Returns the pointer to the allocated data
   * @return
   */
  T* data() const
  {
    return m_Data.get();
  }

  /**
   * @brief Returns the number of elements in each Tuple.
   * @return size_t
   */
  size_t getNumberOfComponents() const override
  {
    return std::accumulate(m_ComponentShape.cbegin(), m_ComponentShape.cend(), static_cast<size_t>(1), std::multiplies<>());
  }

  ShapeType getTupleShape() const
  {
    return m_TupleShape;
  }

  ShapeType getComponentShape() const
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
  void reshapeTuples(const ShapeType& tupleShape) override
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
    for(size_t i = 0; i < newSize && i < oldSize; i++)
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
  typename IDataStore<T>::value_type getValue(size_t index) const override
  {
    return m_Data[index];
  }

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  void setValue(size_t index, value_type value) override
  {
    m_Data[index] = value;
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param  index
   * @return const_reference
   */
  typename IDataStore<T>::const_reference operator[](size_t index) const override
  {
    return m_Data[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return reference
   */
  typename IDataStore<T>::reference operator[](size_t index) override
  {
    return m_Data[index];
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  typename IDataStore<T>::const_reference at(size_t index) const override
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
    for(size_t i = 0; i < IDataStore<T>::getSize(); i++)
      for(size_t i = 0; i < this->getSize(); i++)
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
  H5::ErrorType writeHdf5(H5::IdType dataId) const override
  {
    std::cout << "Writing DataStore to: " << complex::H5::Support::getObjectPath(dataId) << std::endl;
    hsize_t rank = m_TupleShape.size() + m_ComponentShape.size();
    std::vector<hsize_t> h5dims;
    for(const auto& value : m_TupleShape)
    {
      h5dims.push_back(static_cast<hsize_t>(value));
    }
    for(const auto& value : m_ComponentShape)
    {
      h5dims.push_back(static_cast<hsize_t>(value));
    }
    herr_t err = complex::H5::Support::writePointerDataset(dataId, k_DataStore, rank, h5dims.data(), m_Data.get());
    if(err < 0)
    {
    }

    err = complex::H5::Support::writeVectorAttribute(dataId, k_DataStore, k_TupleShape, {m_TupleShape.size()}, m_TupleShape);

    err = complex::H5::Support::writeVectorAttribute(dataId, k_DataStore, k_ComponentShape, {m_ComponentShape.size()}, m_ComponentShape);

#if 0
    int32_t rank = 1;

    hsize_t dataSize = getNumberOfComponents() * getNumberOfTuples();

    hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
    hid_t dataspaceId = H5Screate_simple(rank, &dataSize, nullptr);




    auto err = H5::Writer::Generic::writeScalarAttribute(dataId, ".", complex::H5::Constants::DataStore::NumberOfTuples, m_TupleShape);
    if(err < 0)
    {
      throw std::runtime_error("Error writing DataStore tuple count");
    }
    err = H5::Writer::Generic::writeScalarAttribute(dataId, ".", H5::Constants::DataStore::NumberOfComponents, m_ComponentShape);
    if(err < 0)
    {
      throw std::runtime_error("Error writing DataStore tuple size");
    }
    hid_t storeId = H5Dcreate1(dataId, k_DataStore, dataType, dataspaceId, H5P_DEFAULT);
    err = H5Dwrite(storeId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, m_Data.get());
    if(H5Dclose(storeId) < 0)
    {
      throw std::runtime_error("Error closing HDF5 data store");
    }
    if(H5Sclose(dataspaceId) < 0)
    {
      throw std::runtime_error("Error closing HDF5 dataspace");
    }
#endif
    return err;
  }

  static DataStore* readHdf5(H5::IdType dataId)
  {
    typename DataStore<T>::ShapeType tupleShape;
    typename DataStore<T>::ShapeType componentShape;

    herr_t err = H5::Support::readVectorAttribute(dataId, ".", H5::Constants::DataStore::TupleShape, tupleShape);
    if(err < 0)
    {
    }
    size_t numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());

    err = H5::Support::readVectorAttribute(dataId, ".", H5::Constants::DataStore::ComponentShape, componentShape);
    if(err < 0)
    {
    }
    size_t numComponents = std::accumulate(componentShape.cbegin(), componentShape.cend(), static_cast<size_t>(1), std::multiplies<>());

    auto dataStore = new complex::DataStore<T>(tupleShape, componentShape);
    err = H5::Support::readPointerDataset(dataId, k_DataStore, dataStore->data());
    if(err < 0)
    {
      throw std::runtime_error(fmt::format("Error reading DataStore from HDF5 at {}", H5::Support::getObjectPath(dataId)));
      delete dataStore;
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
using CharDataStore = DataStore<char>;
using UCharDataStore = DataStore<unsigned char>;

using UInt8DataStore = DataStore<uint8_t>;
using UInt16DataStore = DataStore<uint16_t>;
using UInt32DataStore = DataStore<uint32_t>;
using UInt64DataStore = DataStore<uint64_t>;

using Int8DataStore = DataStore<int8_t>;
using Int16DataStore = DataStore<int16_t>;
using Int32DataStore = DataStore<int32_t>;
using Int64DataStore = DataStore<int64_t>;
using SizeDataStore = DataStore<size_t>;

using FloatDataStore = DataStore<float>;
using DoubleDataStore = DataStore<double>;

} // namespace complex
