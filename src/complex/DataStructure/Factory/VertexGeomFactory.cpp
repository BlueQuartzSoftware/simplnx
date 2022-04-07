#include "VertexGeomFactory.hpp"

#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;
using namespace complex::H5;

VertexGeomFactory::VertexGeomFactory()
: H5::IDataFactory()
{
}

VertexGeomFactory::~VertexGeomFactory() = default;

std::string VertexGeomFactory::getDataTypeName() const
{
  return "VertexGeom";
}

H5::ErrorType VertexGeomFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                             const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  auto name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto geom = VertexGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readHdf5(dataStructureReader, groupReader, preflight);
}

//------------------------------------------------------------------------------
H5::ErrorType VertexGeomFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                               const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  return -1;
}
