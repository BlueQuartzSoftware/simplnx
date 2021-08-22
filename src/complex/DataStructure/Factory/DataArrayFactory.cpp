#include "DataArrayFactory.hpp"

#include <memory>
#include <numeric>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"

using namespace complex;

namespace Constants
{
inline const std::string CompDims = H5::Constants::DataStore::TupleSize;
inline const std::string TupleDims = H5::Constants::DataStore::TupleCount;
}

DataArrayFactory::DataArrayFactory()
: IH5DataFactory()
{
}

DataArrayFactory::~DataArrayFactory() = default;

std::string DataArrayFactory::getDataTypeName() const
{
  return "DataArray";
}

void readDims(H5::IdType daId, uint64_t& tupleCount, uint64_t& tupleSize)
{
  H5::Reader::Generic::readScalarAttribute(daId, ".", H5::Constants::DataStore::TupleCount, tupleCount);
  H5::Reader::Generic::readScalarAttribute(daId, ".", H5::Constants::DataStore::TupleSize, tupleSize);
}

template <typename T>
DataStore<T>* createDataStore(H5::IdType dataStoreId, uint64_t tupleCount, uint64_t tupleSize, H5::ErrorType& err)
{
  auto buffer = std::make_unique<T[]>(tupleCount * tupleSize);
  hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
  err = H5Dread(dataStoreId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.get());
  if(err < 0)
  {
    throw std::runtime_error("DataArrayFactory: Error reading HDF5 DataStore");
  }
  H5Tclose(dataType);

  return new DataStore<T>(tupleSize, tupleCount, std::move(buffer));
}

H5::ErrorType DataArrayFactory::createFromHdf5(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  H5::ErrorType err = 0;
  std::string name = getObjName(targetId);

  uint64_t tupleCount;
  uint64_t tupleSize;
  readDims(targetId, tupleCount, tupleSize);

  if(err >= 0)
  {
    hid_t dataStoreId = H5Dopen(targetId, "DataStore", H5P_DEFAULT);

    hid_t dataTypeId = H5Dget_type(dataStoreId);
    if(dataTypeId < 0)
    {
      throw std::runtime_error("DataArrayFactory: Cannot determine DataArray type when reading from HDF5.");
    }

    if(H5Tequal(dataTypeId, H5T_NATIVE_FLOAT) > 0)
    {
      auto dataStore = createDataStore<float>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<float>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_DOUBLE) > 0)
    {
      auto dataStore = createDataStore<double>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<double>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_INT8) > 0)
    {
      auto dataStore = createDataStore<int8_t>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<int8_t>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_INT16) > 0)
    {
      auto dataStore = createDataStore<int16_t>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<int16_t>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_INT32) > 0)
    {
      auto dataStore = createDataStore<int32_t>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<int32_t>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_INT64) > 0)
    {
      auto dataStore = createDataStore<int64_t>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<int64_t>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_UINT8) > 0)
    {
      auto dataStore = createDataStore<uint8_t>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<uint8_t>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_UINT16) > 0)
    {
      auto dataStore = createDataStore<uint16_t>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<uint16_t>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_UINT32) > 0)
    {
      auto dataStore = createDataStore<uint32_t>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<uint32_t>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_UINT64) > 0)
    {
      auto dataStore = createDataStore<uint64_t>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<uint64_t>::Create(ds, name, dataStore, parentId);
    }
    else
    {
      err = -777;
    }

    H5Tclose(dataTypeId);
    H5Dclose(dataStoreId);
  }
  return err;
}
