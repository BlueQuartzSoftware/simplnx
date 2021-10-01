#include "EdgeGeomFactory.hpp"

#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

EdgeGeomFactory::EdgeGeomFactory()
: IH5DataFactory()
{
}

EdgeGeomFactory::~EdgeGeomFactory() = default;

std::string EdgeGeomFactory::getDataTypeName() const
{
  return "EdgeGeom";
}

H5::ErrorType EdgeGeomFactory::readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  std::string name = groupReader.getName();
  auto geom = EdgeGeom::Create(ds, name, parentId);
  return geom->readHdf5(groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType EdgeGeomFactory::readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
