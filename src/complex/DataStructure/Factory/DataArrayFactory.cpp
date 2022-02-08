#include "DataArrayFactory.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

#include <memory>
#include <numeric>
#include <optional>

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

H5::ErrorType DataArrayFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  return -1;
}

/**
 * @brief Creates and imports a DataArray based on the provided DatasetReader
 * @param dataStructure
 * @param datasetReader
 * @param dataArrayName
 * @param importId
 * @param err
 * @param parentId
 * @param preflight
 */
template <typename T>
void importDataArray(DataStructure& dataStructure, const H5::DatasetReader& datasetReader, const std::string dataArrayName, DataObject::IdType importId, H5::ErrorType& err,
                     const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  if(preflight)
  {
    auto dataStore = EmptyDataStore<float>::readHdf5(datasetReader);
    DataArray<float>::Import(dataStructure, dataArrayName, importId, std::move(dataStore), parentId);
  }
  else
  {
    auto dataStore = DataStore<float>::readHdf5(datasetReader);
    DataArray<float>::Import(dataStructure, dataArrayName, importId, std::move(dataStore), parentId);
  }
}

H5::ErrorType DataArrayFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  H5::ErrorType err = 0;
  H5::Type type = datasetReader.getType();
  if(type == H5::Type::unknown)
  {
    return -1;
  }

  std::string dataArrayName = datasetReader.getName();
  DataObject::IdType importId = ReadObjectId(datasetReader);

  switch(type)
  {
  case H5::Type::float32: {
    importDataArray<float32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  case H5::Type::float64: {
    importDataArray<float64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  case H5::Type::int8: {
    importDataArray<int8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  case H5::Type::int16: {
    importDataArray<int16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  case H5::Type::int32: {
    importDataArray<int32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  case H5::Type::int64: {
    importDataArray<int64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  case H5::Type::uint8: {
    importDataArray<uint8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  case H5::Type::uint16: {
    importDataArray<uint16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  case H5::Type::uint32: {
    importDataArray<uint32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  case H5::Type::uint64: {
    importDataArray<uint64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
    break;
  }
  default:
    err = -777;
    break;
  }

  return err;
}
