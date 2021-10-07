#include "HexahedralGeomFactory.hpp"

#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

HexahedralGeomFactory::HexahedralGeomFactory()
: IH5DataFactory()
{
}

HexahedralGeomFactory::~HexahedralGeomFactory() = default;

std::string HexahedralGeomFactory::getDataTypeName() const
{
  return "HexahedralGeom";
}

H5::ErrorType HexahedralGeomFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  std::string name = groupReader.getName();
  auto geom = HexahedralGeom::Create(dataStructureReader.getDataStructure(), name, parentId);
  return geom->readHdf5(dataStructureReader, groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType HexahedralGeomFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
