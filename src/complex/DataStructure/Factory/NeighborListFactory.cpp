#include "NeighborListFactory.hpp"

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

#include <memory>
#include <numeric>
#include <optional>

using namespace complex;
using namespace complex::H5;

NeighborListFactory::NeighborListFactory()
: IDataFactory()
{
}

NeighborListFactory::~NeighborListFactory() = default;

std::string NeighborListFactory::getDataTypeName() const
{
  return "NeighborList<T>";
}

H5::ErrorType NeighborListFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}

H5::ErrorType NeighborListFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
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
    auto dataVector = NeighborList<float32>::ReadHdf5Data(datasetReader);
    NeighborList<float>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  case H5::Type::float64: {
    auto dataVector = NeighborList<float64>::ReadHdf5Data(datasetReader);
    NeighborList<double>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  case H5::Type::int8: {
    auto dataVector = NeighborList<int8>::ReadHdf5Data(datasetReader);
    NeighborList<int8_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  case H5::Type::int16: {
    auto dataVector = NeighborList<int16>::ReadHdf5Data(datasetReader);
    NeighborList<int16_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  case H5::Type::int32: {
    auto dataVector = NeighborList<int32>::ReadHdf5Data(datasetReader);
    NeighborList<int32_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  case H5::Type::int64: {
    auto dataVector = NeighborList<int64>::ReadHdf5Data(datasetReader);
    NeighborList<int64_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  case H5::Type::uint8: {
    auto dataVector = NeighborList<uint8>::ReadHdf5Data(datasetReader);
    NeighborList<uint8_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  case H5::Type::uint16: {
    auto dataVector = NeighborList<uint16>::ReadHdf5Data(datasetReader);
    NeighborList<uint16_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  case H5::Type::uint32: {
    auto dataVector = NeighborList<uint32>::ReadHdf5Data(datasetReader);
    NeighborList<uint32_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  case H5::Type::uint64: {
    auto dataVector = NeighborList<uint64>::ReadHdf5Data(datasetReader);
    NeighborList<uint64_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, dataVector, parentId);
    break;
  }
  default:
    err = -777;
    break;
  }

  return err;
}
