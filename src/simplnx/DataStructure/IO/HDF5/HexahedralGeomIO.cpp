#include "HexahedralGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::HDF5
{
HexahedralGeomIO::HexahedralGeomIO() = default;
HexahedralGeomIO::~HexahedralGeomIO() noexcept = default;

DataObject::Type HexahedralGeomIO::getDataType() const
{
  return DataObject::Type::HexahedralGeom;
}

std::string HexahedralGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> HexahedralGeomIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geom = HexahedralGeom::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return INodeGeom3dIO::ReadNodeGeom3dData(structureReader, *geom, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}

Result<> HexahedralGeomIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const
{
  auto* geometry = dataStructure.getDataAs<HexahedralGeom>(dataPath);
  if(geometry == nullptr)
  {
    return MakeErrorResult(-25070, fmt::format("Failed to finish importing geometry at path '{}'. Geometry does not exist or is of wrong type.", dataPath.toString()));
  }

  return INodeGeom3dIO::FinishImportingNodeGeom3dData(dataStructure, dataPath, dataStructureGroup);
}

Result<> HexahedralGeomIO::writeData(DataStructureWriter& structureReader, const HexahedralGeom& geom, group_writer_type& parentGroup, bool importable) const
{
  auto groupWriter = parentGroup.createGroup(geom.getName());
  return INodeGeom3dIO::WriteNodeGeom3dData(structureReader, geom, parentGroup, importable);
}

Result<> HexahedralGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
