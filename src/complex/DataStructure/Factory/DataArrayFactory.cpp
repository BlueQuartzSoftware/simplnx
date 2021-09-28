#include "DataArrayFactory.hpp"

#include <memory>
#include <numeric>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"

using namespace complex;

namespace Constants
{
inline const std::string CompDims = H5::Constants::DataStore::NumComponents;
inline const std::string TupleDims = H5::Constants::DataStore::TupleCount;
} // namespace Constants

DataArrayFactory::DataArrayFactory()
: IH5DataFactory()
{
}

DataArrayFactory::~DataArrayFactory() = default;

std::string DataArrayFactory::getDataTypeName() const
{
  return "DataArray";
}

void readDims(H5::IdType daId, uint64& tupleCount, uint64& tupleSize)
{
  H5::Reader::Generic::readScalarAttribute(daId, ".", H5::Constants::DataStore::TupleCount, tupleCount);
  H5::Reader::Generic::readScalarAttribute(daId, ".", H5::Constants::DataStore::NumComponents, tupleSize);
}

template <typename T>
DataStore<T>* createDataStore(H5::IdType dataStoreId, uint64 tupleCount, uint64 tupleSize, H5::ErrorType& err)
{
  auto buffer = std::make_unique<T[]>(tupleCount * tupleSize);
  hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
  err = H5Dread(dataStoreId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.get());
  if(err < 0)
  {
    throw std::runtime_error("DataArrayFactory: Error reading HDF5 DataStore");
  }

  return new DataStore<T>(tupleSize, tupleCount, std::move(buffer));
}

H5::ErrorType DataArrayFactory::createFromHdf5(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  H5::ErrorType err = 0;
  std::string name = getObjName(targetId);

  uint64 tupleCount;
  uint64 tupleSize;
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
      auto dataStore = createDataStore<float32>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<float32>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_DOUBLE) > 0)
    {
      auto dataStore = createDataStore<float64>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<float64>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_INT8) > 0)
    {
      auto dataStore = createDataStore<int8>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<int8>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_INT16) > 0)
    {
      auto dataStore = createDataStore<int16>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<int16>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_INT32) > 0)
    {
      auto dataStore = createDataStore<int32>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<int32>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_INT64) > 0)
    {
      auto dataStore = createDataStore<int64>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<int64>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_UINT8) > 0)
    {
      auto dataStore = createDataStore<uint8>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<uint8>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_UINT16) > 0)
    {
      auto dataStore = createDataStore<uint16>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<uint16>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_UINT32) > 0)
    {
      auto dataStore = createDataStore<uint32>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<uint32>::Create(ds, name, dataStore, parentId);
    }
    else if(H5Tequal(dataTypeId, H5T_NATIVE_UINT64) > 0)
    {
      auto dataStore = createDataStore<uint64>(dataStoreId, tupleCount, tupleSize, err);
      DataArray<uint64>::Create(ds, name, dataStore, parentId);
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
