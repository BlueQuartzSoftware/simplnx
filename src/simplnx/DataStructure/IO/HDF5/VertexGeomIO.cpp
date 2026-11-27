#include "VertexGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::hdf5
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

Result<> VertexGeomIO::readData(DataStructureReader& dataStructureReader, const GroupReaderType& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometryPtr = VertexGeom::Import(dataStructureReader.getDataStructure(), objectName, importId, parentId);
  return INodeGeom0dIO::ReadNodeGeom0dData(dataStructureReader, *geometryPtr, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}

Result<> VertexGeomIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const GroupReaderType& dataStructureGroup) const
{
  auto* geomPtr = dataStructure.getDataAs<VertexGeom>(dataPath);
  if(geomPtr == nullptr)
  {
    return MakeErrorResult(-50595, fmt::format("Failed to finish importing VertexGeom at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }
  return INodeGeom0dIO::FinishImportingNodeGeom0dData(dataStructure, dataPath, dataStructureGroup);
}

Result<> VertexGeomIO::writeData(DataStructureWriter& structureReader, const VertexGeom& geometry, GroupWriterType& parentGroupWriter, bool importable)
{
  return INodeGeom0dIO::WriteNodeGeom0dData(structureReader, geometry, parentGroupWriter, importable);
}

Result<> VertexGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, GroupWriterType& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::hdf5
