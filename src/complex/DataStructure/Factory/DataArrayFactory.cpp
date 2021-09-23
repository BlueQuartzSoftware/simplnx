#include "DataArrayFactory.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"

#include <memory>
#include <numeric>

using namespace complex;

DataArrayFactory::DataArrayFactory()
: IH5DataFactory()
{
}

DataArrayFactory::~DataArrayFactory() = default;

std::string DataArrayFactory::getDataTypeName() const
{
  return "DataArray";
}

void readDims(H5::IdType daId, uint64_t& numTuples, uint64_t& numComponents)
{
  std::vector<size_t> tupleShape;
  std::vector<size_t> componentShape;

  herr_t err = H5::Support::readVectorAttribute(daId, ".", H5::Constants::DataStore::TupleShape, tupleShape);
  if(err < 0)
  {
    throw std::runtime_error(fmt::format("Error reading '{}' Attribute from DataSet: {}", H5::Constants::DataStore::TupleShape, H5::Support::getObjectPath(daId)));
  }
  numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());

  err = H5::Support::readVectorAttribute(daId, ".", H5::Constants::DataStore::ComponentShape, componentShape);
  if(err < 0)
  {
    throw std::runtime_error(fmt::format("Error reading '{}' Attribute from DataSet: {}", H5::Constants::DataStore::ComponentShape, H5::Support::getObjectPath(daId)));
  }
  numComponents = std::accumulate(componentShape.cbegin(), componentShape.cend(), static_cast<size_t>(1), std::multiplies<>());
}

/**
 *
 * @tparam T
 * @param dataStoreId
 * @param numTuples
 * @param numComponents
 * @param err
 * @return
 */
template <typename T>
DataStore<T>* createDataStore(H5::IdType dataStoreId, uint64_t numTuples, uint64_t numComponents, H5::ErrorType& err)
{
  using DataStoreType = DataStore<T>;
  DataStoreType* dataStore = new DataStoreType({numTuples}, {numComponents});

  hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
  err = H5Dread(dataStoreId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataStore->data());
  if(err < 0)
  {
    throw std::runtime_error(H5::Support::getObjectPath(dataStoreId));
  }

  return dataStore;
}

/**
 *
 * @param ds
 * @param targetId
 * @param groupId
 * @param parentId
 * @return
 */
H5::ErrorType DataArrayFactory::createFromHdf5(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  H5::ErrorType err = 0;
  std::string name = getObjName(targetId);

  uint64_t tupleCount;
  uint64_t tupleSize;

  if(err >= 0)
  {
    hid_t dataStoreId = H5Dopen(targetId, "DataStore", H5P_DEFAULT);
    readDims(dataStoreId, tupleCount, tupleSize);

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
