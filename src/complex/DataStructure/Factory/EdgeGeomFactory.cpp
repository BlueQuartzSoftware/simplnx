#include "EdgeGeomFactory.hpp"

#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;
using namespace complex::H5;

EdgeGeomFactory::EdgeGeomFactory()
: IDataFactory()
{
}

EdgeGeomFactory::~EdgeGeomFactory() = default;

std::string EdgeGeomFactory::getDataTypeName() const
{
  return "EdgeGeom";
}

H5::ErrorType EdgeGeomFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                           const std::optional<DataObject::IdType>& parentId)
{
  std::string name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto geom = EdgeGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readHdf5(dataStructureReader, groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType EdgeGeomFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                             const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
