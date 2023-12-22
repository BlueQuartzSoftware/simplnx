#include "QuadGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"

#include "simplnx/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"

namespace nx::core::HDF5
{
QuadGeomIO::QuadGeomIO() = default;
QuadGeomIO::~QuadGeomIO() noexcept = default;

DataObject::Type QuadGeomIO::getDataType() const
{
  return DataObject::Type::QuadGeom;
}

std::string QuadGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> QuadGeomIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                              const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometry = QuadGeom::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return INodeGeom2dIO::ReadNodeGeom2dData(structureReader, *geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}
Result<> QuadGeomIO::writeData(DataStructureWriter& dataStructureWriter, const QuadGeom& geom, group_writer_type& parentGroup, bool importable) const
{
  return INodeGeom2dIO::WriteNodeGeom2dData(dataStructureWriter, geom, parentGroup, importable);
}

Result<> QuadGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
