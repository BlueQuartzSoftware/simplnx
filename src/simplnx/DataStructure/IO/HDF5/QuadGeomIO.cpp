#include "QuadGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::HDF5
{
QuadGeomIO::QuadGeomIO() = default;
QuadGeomIO::~QuadGeomIO() noexcept = default;

AbstractDataObject::Type QuadGeomIO::getDataType() const
{
  return IDataObject::Type::QuadGeom;
}

std::string QuadGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> QuadGeomIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, AbstractDataObject::IdType importId,
                              const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometry = QuadGeom::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return AbstractNodeGeom2dIO::ReadNodeGeom2dData(structureReader, *geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}

Result<> QuadGeomIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const
{
  auto* geom = dataStructure.getDataAs<QuadGeom>(dataPath);
  if(geom == nullptr)
  {
    return MakeErrorResult(-50590, fmt::format("Failed to finish importing QuadGeom at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  return AbstractNodeGeom2dIO::FinishImportingNodeGeom2dData(dataStructure, dataPath, dataStructureGroup);
}

Result<> QuadGeomIO::writeData(DataStructureWriter& dataStructureWriter, const QuadGeom& geom, group_writer_type& parentGroup, bool importable) const
{
  return AbstractNodeGeom2dIO::WriteNodeGeom2dData(dataStructureWriter, geom, parentGroup, importable);
}

Result<> QuadGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const AbstractDataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
