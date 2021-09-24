#include "DataArrayFactory.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"

#include <complex/DataStructure/DataStore.hpp>
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

  herr_t err = H5::Support::readVectorAttribute(daId, ".", complex::DataStore<size_t>::k_TupleShape, tupleShape);
  if(err < 0)
  {
    throw std::runtime_error(fmt::format("Error reading '{}' Attribute from DataSet: {}", complex::DataStore<size_t>::k_TupleShape, H5::Support::getObjectPath(daId)));
  }
  numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());

  err = H5::Support::readVectorAttribute(daId, ".", complex::DataStore<size_t>::k_ComponentShape, componentShape);
  if(err < 0)
  {
    throw std::runtime_error(fmt::format("Error reading '{}' Attribute from DataSet: {}", complex::DataStore<size_t>::k_ComponentShape, H5::Support::getObjectPath(daId)));
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

H5::ErrorType DataArrayFactory::readDataStructureGroup(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}

/**
 *
 * @param ds
 * @param targetId
 * @param groupId
 * @param parentId
 * @return
 */
H5::ErrorType DataArrayFactory::readDataStructureDataset(DataStructure& ds, H5::IdType h5LocationId, const std::string& h5DatasetName, const std::optional<DataObject::IdType>& parentId)
{
  H5::ErrorType err = 0;
  hid_t typeId = H5::Support::getDatasetType(h5LocationId, h5DatasetName);
  if(typeId < 0)
  {
    return -1;
  }
  std::vector<hsize_t> dims;
  H5T_class_t attr_type;
  size_t attr_size;

  H5::Support::getDatasetInfo(h5LocationId, h5DatasetName, dims, attr_type, attr_size);

  if(err >= 0)
  {

    if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
    {
      auto* dataStore = DataStore<float>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<float>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
    {
      auto* dataStore = DataStore<double>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<double>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
    {
      auto* dataStore = DataStore<int8_t>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<int8_t>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
    {
      auto* dataStore = DataStore<int16_t>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<int16_t>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
    {
      auto* dataStore = DataStore<int32_t>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<int32_t>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
    {
      auto* dataStore = DataStore<int64_t>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<int64_t>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
    {
      auto* dataStore = DataStore<uint8_t>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<uint8_t>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
    {
      auto* dataStore = DataStore<uint16_t>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<uint16_t>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
    {
      auto* dataStore = DataStore<uint32_t>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<uint32_t>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
    {
      auto* dataStore = DataStore<uint64_t>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<uint64_t>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else
    {
      err = -777;
    }

    H5Tclose(typeId);
  }
  return err;
}
