#include "TetrahedralGeomFactory.hpp"

#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;
using namespace complex::H5;

TetrahedralGeomFactory::TetrahedralGeomFactory()
: H5::IDataFactory()
{
}

TetrahedralGeomFactory::~TetrahedralGeomFactory() = default;

std::string TetrahedralGeomFactory::getDataTypeName() const
{
  return "TetrahedralGeom";
}

H5::ErrorType TetrahedralGeomFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto geom = TetrahedralGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readHdf5(dataStructureReader, groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType TetrahedralGeomFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
