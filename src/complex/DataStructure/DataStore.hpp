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
  DataStore(size_t tupleSize, size_t tupleCount)
  : m_TupleSize(tupleSize)
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
  DataStore(size_t tupleSize, size_t tupleCount, std::unique_ptr<value_type[]>&& data)
  : m_TupleSize(tupleSize)
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
  , m_TupleSize(other.m_TupleSize)
  , m_Data(std::make_unique<value_type[]>(m_TupleCount * m_TupleSize))
  {
    const size_t count = m_TupleCount * m_TupleSize;
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
  : m_TupleCount(std::move(other.m_TupleCount))
  , m_TupleSize(std::move(other.m_TupleSize))
  , m_Data(std::move(other.m_Data))
  {
  }

  virtual ~DataStore() = default;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return size_t
   */
  size_t getTupleCount() const override
  {
    return m_TupleCount;
  }

  /**
   * @brief Returns the tuple size.
   * @return size_t
   */
  size_t getTupleSize() const override
  {
    return m_TupleSize;
  }

  /**
   * @brief Resizes the DataStore to handle the specified number of tuples.
   * @param numTuples
   */
  void resizeTuples(size_t numTuples) override
  {
    auto oldSize = this->getSize();
    m_TupleCount = numTuples;
    auto newSize = this->getSize();

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
    int32_t rank = 1;

    hsize_t dataSize = m_TupleSize * m_TupleCount;

    hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
    hid_t dataspaceId = H5Screate_simple(rank, &dataSize, nullptr);
    auto err = H5::Writer::Generic::writeScalarAttribute(dataId, ".", H5::Constants::DataStore::TupleCount, m_TupleCount);
    if(err < 0)
    {
      throw std::runtime_error("Error writing DataStore tuple count");
    }
    err = H5::Writer::Generic::writeScalarAttribute(dataId, ".", H5::Constants::DataStore::TupleSize, m_TupleSize);
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
    uint64_t tDims;
    uint64_t cDims;
    H5::Reader::Generic::readScalarAttribute(dataId, ".", H5::Constants::DataStore::TupleCount, tDims);
    H5::Reader::Generic::readScalarAttribute(dataId, ".", H5::Constants::DataStore::TupleSize, cDims);

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
  uint64_t m_TupleSize;
  uint64_t m_TupleCount;
  std::unique_ptr<value_type[]> m_Data;
};
} // namespace complex
