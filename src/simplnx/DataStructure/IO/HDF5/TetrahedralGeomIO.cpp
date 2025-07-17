#include "TetrahedralGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::HDF5
{
TetrahedralGeomIO::TetrahedralGeomIO() = default;
TetrahedralGeomIO::~TetrahedralGeomIO() noexcept = default;

DataObject::Type TetrahedralGeomIO::getDataType() const
{
  return DataObject::Type::TetrahedralGeom;
}

std::string TetrahedralGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> TetrahedralGeomIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                     const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometry = TetrahedralGeom::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return INodeGeom3dIO::ReadNodeGeom3dData(structureReader, *geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}

Result<> TetrahedralGeomIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const
{
  auto* geom = dataStructure.getDataAs<TetrahedralGeom>(dataPath);
  if(geom == nullptr)
  {
    return MakeErrorResult(-50595, fmt::format("Failed to finish importing TetrahedraldGeom at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }
  return INodeGeom3dIO::FinishImportingNodeGeom3dData(dataStructure, dataPath, dataStructureGroup);
}

Result<> TetrahedralGeomIO::writeData(DataStructureWriter& dataStructureWriter, const TetrahedralGeom& geometry, group_writer_type& parentGroup, bool importable) const
{
  return INodeGeom3dIO::WriteNodeGeom3dData(dataStructureWriter, geometry, parentGroup, importable);
}

Result<> TetrahedralGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
