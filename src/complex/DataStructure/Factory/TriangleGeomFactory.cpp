#include "TriangleGeomFactory.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;
using namespace complex::H5;

TriangleGeomFactory::TriangleGeomFactory()
: H5::IDataFactory()
{
}

TriangleGeomFactory::~TriangleGeomFactory() = default;

std::string TriangleGeomFactory::getDataTypeName() const
{
  return "TriangleGeom";
}

H5::ErrorType TriangleGeomFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                               const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  auto name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto geom = TriangleGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readHdf5(dataStructureReader, groupReader, preflight);
}

//------------------------------------------------------------------------------
H5::ErrorType TriangleGeomFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                                 const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  return -1;
}
