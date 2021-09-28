#pragma once

#include <memory>
#include <stdexcept>

#include <hdf5.h>

#include "complex/DataStructure/IDataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"

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

  /**
   * @brief Constructs a DataStore with the specified tupleSize and tupleCount.
   * @param tupleSize
   * @param tupleCount
   */
  DataStore(usize tupleSize, usize tupleCount)
  : m_NumComponents(tupleSize)
  , m_TupleCount(tupleCount)
  , m_Data(std::make_unique<value_type[]>(tupleSize * tupleCount))
  {
  }

  /**
   * @brief Constructs a DataStore with the specified tupleSize and tupleCount.
   * @param tupleSize
   * @param tupleCount
   * @param data
   */
  DataStore(usize tupleSize, usize tupleCount, std::unique_ptr<value_type[]>&& data)
  : m_NumComponents(tupleSize)
  , m_TupleCount(tupleCount)
  , m_Data(std::move(data))
  {
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  DataStore(const DataStore& other)
  : m_TupleCount(other.m_TupleCount)
  , m_NumComponents(other.m_NumComponents)
  , m_Data(std::make_unique<value_type[]>(m_TupleCount * m_NumComponents))
  {
    const usize count = m_TupleCount * m_NumComponents;
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
  : m_TupleCount(std::move(other.m_TupleCount))
  , m_NumComponents(std::move(other.m_NumComponents))
  , m_Data(std::move(other.m_Data))
  {
  }

  virtual ~DataStore() = default;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return usize
   */
  usize getTupleCount() const override
  {
    return m_TupleCount;
  }

  /**
   * @brief Returns the number of elements in each Tuple.
   * @return usize
   */
  usize getNumComponents() const override
  {
    return m_NumComponents;
  }

  /**
   * @brief Resizes the DataStore to handle the specified number of tuples.
   * @param numTuples
   */
  void resizeTuples(usize numTuples) override
  {
    auto oldSize = this->getSize();
    m_TupleCount = numTuples;
    auto newSize = this->getSize();

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
  H5::ErrorType writeHdf5(H5::IdType dataId) const override
  {
    int32 rank = 1;

    hsize_t dataSize = m_NumComponents * m_TupleCount;

    hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
    hid_t dataspaceId = H5Screate_simple(rank, &dataSize, nullptr);
    auto err = H5::Writer::Generic::writeScalarAttribute(dataId, ".", H5::Constants::DataStore::TupleCount, m_TupleCount);
    if(err < 0)
    {
      throw std::runtime_error("Error writing DataStore tuple count");
    }
    err = H5::Writer::Generic::writeScalarAttribute(dataId, ".", H5::Constants::DataStore::NumComponents, m_NumComponents);
    if(err < 0)
    {
      throw std::runtime_error("Error writing DataStore tuple size");
    }
    hid_t storeId = H5Dcreate1(dataId, "DataStore", dataType, dataspaceId, H5P_DEFAULT);
    err = H5Dwrite(storeId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, m_Data.get());
    if(H5Dclose(storeId) < 0)
    {
      throw std::runtime_error("Error closing HDF5 data store");
    }
    if(H5Sclose(dataspaceId) < 0)
    {
      throw std::runtime_error("Error closing HDF5 dataspace");
    }

    return err;
  }

  static DataStore* readHdf5(H5::IdType dataId)
  {
    uint64 tDims;
    uint64 cDims;
    H5::Reader::Generic::readScalarAttribute(dataId, ".", H5::Constants::DataStore::TupleCount, tDims);
    H5::Reader::Generic::readScalarAttribute(dataId, ".", H5::Constants::DataStore::NumComponents, cDims);

    auto buffer = new T[tDims * cDims];
    try
    {
      // auto buffer = std::make_unique<T[]>(tDims * cDims);
      hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
      auto err = H5Dread(dataId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);
      if(err < 0)
      {
        throw std::runtime_error("Error reading DataStore from HDF5");
      }
    } catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
      throw std::runtime_error("Error creating DataStore.\nTupleCount: " + std::to_string(tDims) + "\nTupleSize: " + std::to_string(cDims));
    }
    delete[] buffer;

    try
    {
      auto buffer = std::make_unique<T[]>(5);
      auto dataStore = new complex::DataStore<T>(cDims, tDims, std::move(buffer));
      return dataStore;
    } catch(const std::exception& e)
    {
      throw std::runtime_error("Error creating DataStore in DataStore::readHdf5");
    }
    return nullptr;
  }

private:
  uint64 m_NumComponents;
  uint64 m_TupleCount;
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
