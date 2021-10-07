#include "RectGridGeomFactory.hpp"

#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

RectGridGeomFactory::RectGridGeomFactory()
: IH5DataFactory()
{
}

RectGridGeomFactory::~RectGridGeomFactory() = default;

std::string RectGridGeomFactory::getDataTypeName() const
{
  return "RectGridGeom";
}

H5::ErrorType RectGridGeomFactory::readDataStructureGroup(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto geom = RectGridGeom::Create(dataStructureReader.getDataStructure(), name, parentId);
  return geom->readHdf5(dataStructureReader, groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType RectGridGeomFactory::readDataStructureDataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
