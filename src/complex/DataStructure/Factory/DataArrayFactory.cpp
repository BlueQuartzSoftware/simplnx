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

void readDims(H5::IdType daId, uint64& numTuples, uint64& numComponents)
{
  std::vector<size_t> tupleShape;
  std::vector<size_t> componentShape;

  herr_t err = H5::Support::ReadVectorAttribute(daId, ".", complex::DataStore<size_t>::k_TupleShape, tupleShape);
  if(err < 0)
  {
    throw std::runtime_error(fmt::format("Error reading '{}' Attribute from DataSet: {}", complex::DataStore<size_t>::k_TupleShape, H5::Support::GetObjectPath(daId)));
  }
  numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());

  err = H5::Support::ReadVectorAttribute(daId, ".", complex::DataStore<size_t>::k_ComponentShape, componentShape);
  if(err < 0)
  {
    throw std::runtime_error(fmt::format("Error reading '{}' Attribute from DataSet: {}", complex::DataStore<size_t>::k_ComponentShape, H5::Support::GetObjectPath(daId)));
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
DataStore<T>* createDataStore(H5::IdType dataStoreId, uint64 numTuples, uint64 numComponents, H5::ErrorType& err)
{
  using DataStoreType = DataStore<T>;
  DataStoreType* dataStore = new DataStoreType({numTuples}, {numComponents});

  hid_t dataType = H5::Support::HdfTypeForPrimitive<T>();
  err = H5Dread(dataStoreId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataStore->data());
  if(err < 0)
  {
    throw std::runtime_error(H5::Support::GetObjectPath(dataStoreId));
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
  hid_t typeId = H5::Support::GetDatasetType(h5LocationId, h5DatasetName);
  if(typeId < 0)
  {
    return -1;
  }
  std::vector<hsize_t> dims;
  H5T_class_t attr_type;
  size_t attr_size;

  H5::Support::GetDatasetInfo(h5LocationId, h5DatasetName, dims, attr_type, attr_size);

  if(err >= 0)
  {

    if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
    {
      auto* dataStore = DataStore<float32>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<float32>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
    {
      auto* dataStore = DataStore<float64>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<float64>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
    {
      auto* dataStore = DataStore<int8>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<int8>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
    {
      auto* dataStore = DataStore<int16>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<int16>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
    {
      auto* dataStore = DataStore<int32>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<int32>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
    {
      auto* dataStore = DataStore<int64>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<int64>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
    {
      auto* dataStore = DataStore<uint8>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<uint8>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
    {
      auto* dataStore = DataStore<uint16>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<uint16>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
    {
      auto* dataStore = DataStore<uint32>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<uint32>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
    {
      auto* dataStore = DataStore<uint64>::readHdf5(h5LocationId, h5DatasetName);
      DataArray<uint64>::Create(ds, h5DatasetName, dataStore, parentId);
    }
    else
    {
      err = -777;
    }

    H5Tclose(typeId);
  }
  return err;
}
