#include "ImageGeomFactory.hpp"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;
using namespace complex::H5;

ImageGeomFactory::ImageGeomFactory()
: H5::IDataFactory()
{
}

ImageGeomFactory::~ImageGeomFactory() = default;

std::string ImageGeomFactory::getDataTypeName() const
{
  return "ImageGeom";
}

H5::ErrorType ImageGeomFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                            const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  auto name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto geom = ImageGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readHdf5(dataStructureReader, groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType ImageGeomFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                              const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  return -1;
}
