#include "ImageGeomFactory.hpp"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

ImageGeomFactory::ImageGeomFactory()
: IH5DataFactory()
{
}

ImageGeomFactory::~ImageGeomFactory() = default;

std::string ImageGeomFactory::getDataTypeName() const
{
  return "ImageGeom";
}

H5::ErrorType ImageGeomFactory::readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto geom = ImageGeom::Create(ds, name, parentId);
  return geom->readHdf5(groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType ImageGeomFactory::readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
