#include "DataArrayFactory.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

#include <memory>
#include <numeric>

using namespace complex;
using namespace complex::H5;

DataArrayFactory::DataArrayFactory()
: IDataFactory()
{
}

DataArrayFactory::~DataArrayFactory() = default;

std::string DataArrayFactory::getDataTypeName() const
{
  return "DataArray";
}

H5::ErrorType DataArrayFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}

H5::ErrorType DataArrayFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  H5::ErrorType err = 0;
  H5::Type type = datasetReader.getType();
  if(type == H5::Type::unknown)
  {
    return -1;
  }

  std::string dataArrayName = datasetReader.getName();
  auto importId = ReadObjectId(datasetReader);

  switch(type)
  {
  case H5::Type::float32: {
    auto* dataStore = DataStore<float>::readHdf5(datasetReader);
    DataArray<float>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  case H5::Type::float64: {
    auto* dataStore = DataStore<double>::readHdf5(datasetReader);
    DataArray<double>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  case H5::Type::int8: {
    auto* dataStore = DataStore<int8_t>::readHdf5(datasetReader);
    DataArray<int8_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  case H5::Type::int16: {
    auto* dataStore = DataStore<int16_t>::readHdf5(datasetReader);
    DataArray<int16_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  case H5::Type::int32: {
    auto* dataStore = DataStore<int32_t>::readHdf5(datasetReader);
    DataArray<int32_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  case H5::Type::int64: {
    auto* dataStore = DataStore<int64_t>::readHdf5(datasetReader);
    DataArray<int64_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  case H5::Type::uint8: {
    auto* dataStore = DataStore<uint8_t>::readHdf5(datasetReader);
    DataArray<uint8_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  case H5::Type::uint16: {
    auto* dataStore = DataStore<uint16_t>::readHdf5(datasetReader);
    DataArray<uint16_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  case H5::Type::uint32: {
    auto* dataStore = DataStore<uint32_t>::readHdf5(datasetReader);
    DataArray<uint32_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  case H5::Type::uint64: {
    auto* dataStore = DataStore<uint64_t>::readHdf5(datasetReader);
    DataArray<uint64_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataStore, parentId);
    break;
  }
  default:
    err = -777;
    break;
  }

  return err;
}
