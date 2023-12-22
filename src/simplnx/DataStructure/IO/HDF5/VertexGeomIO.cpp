#include "VertexGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include "simplnx/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"

namespace nx::core::HDF5
{
VertexGeomIO::VertexGeomIO() = default;
VertexGeomIO::~VertexGeomIO() noexcept = default;

DataObject::Type VertexGeomIO::getDataType() const
{
  return DataObject::Type::VertexGeom;
}

std::string VertexGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> VertexGeomIO::readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometry = VertexGeom::Import(dataStructureReader.getDataStructure(), objectName, importId, parentId);
  return INodeGeom0dIO::ReadNodeGeom0dData(dataStructureReader, *geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}
Result<> VertexGeomIO::writeData(DataStructureWriter& structureReader, const VertexGeom& geometry, group_writer_type& parentGroupWriter, bool importable) const
{
  return INodeGeom0dIO::WriteNodeGeom0dData(structureReader, geometry, parentGroupWriter, importable);
}

Result<> VertexGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
