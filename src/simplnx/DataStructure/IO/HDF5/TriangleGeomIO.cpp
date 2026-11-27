#include "TriangleGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::hdf5
{
TriangleGeomIO::TriangleGeomIO() = default;
TriangleGeomIO::~TriangleGeomIO() noexcept = default;

DataObject::Type TriangleGeomIO::getDataType() const
{
  return DataObject::Type::TriangleGeom;
}

std::string TriangleGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> TriangleGeomIO::readData(DataStructureReader& structureReader, const GroupReaderType& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                  const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometryPtr = TriangleGeom::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return INodeGeom2dIO::ReadNodeGeom2dData(structureReader, *geometryPtr, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}

Result<> TriangleGeomIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const GroupReaderType& dataStructureGroup) const
{
  auto* geomPtr = dataStructure.getDataAs<TriangleGeom>(dataPath);
  if(geomPtr == nullptr)
  {
    return MakeErrorResult(-50595, fmt::format("Failed to finish importing TriangleGeom at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }
  return INodeGeom2dIO::FinishImportingNodeGeom2dData(dataStructure, dataPath, dataStructureGroup);
}

Result<> TriangleGeomIO::writeData(DataStructureWriter& dataStructureWriter, const TriangleGeom& geometry, GroupWriterType& parentGroupWriter, bool importable)
{
  return INodeGeom2dIO::WriteNodeGeom2dData(dataStructureWriter, geometry, parentGroupWriter, importable);
}

Result<> TriangleGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, GroupWriterType& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::hdf5
