#include "QuadGeomFactory.hpp"

#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;
using namespace complex::H5;

QuadGeomFactory::QuadGeomFactory()
: H5::IDataFactory()
{
}

QuadGeomFactory::~QuadGeomFactory() = default;

std::string QuadGeomFactory::getDataTypeName() const
{
  return "QuadGeom";
}

H5::ErrorType QuadGeomFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto geom = QuadGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readHdf5(dataStructureReader, groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType QuadGeomFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
