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

H5::ErrorType NeighborListFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                               const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  return -1;
}

template <typename T>
inline void importNeighborList(DataStructure& dataStructure, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader, const std::string& dataArrayName, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  using NeighborListType = NeighborList<T>;
  auto dataVector = NeighborListType::ReadHdf5Data(parentReader, datasetReader);
  NeighborListType::Import(dataStructure, dataArrayName, importId, dataVector, parentId);
}

H5::ErrorType NeighborListFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                                 const std::optional<DataObject::IdType>& parentId, bool preflight)
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
    importNeighborList<float32>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  case H5::Type::float64: {
    importNeighborList<float64>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  case H5::Type::int8: {
    importNeighborList<int8>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  case H5::Type::int16: {
    importNeighborList<int16>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  case H5::Type::int32: {
    importNeighborList<int32>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  case H5::Type::int64: {
    importNeighborList<int64>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  case H5::Type::uint8: {
    importNeighborList<uint8>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  case H5::Type::uint16: {
    importNeighborList<uint16>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  case H5::Type::uint32: {
    importNeighborList<uint32>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  case H5::Type::uint64: {
    importNeighborList<uint64>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
    break;
  }
  default:
    err = -777;
    break;
  }

  return err;
}
