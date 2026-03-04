#include "AbstractNodeGeom3dIO.hpp"

#include "DataStructureReader.hpp"
#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry3D.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
AbstractNodeGeom3dIO::AbstractNodeGeom3dIO() = default;
AbstractNodeGeom3dIO::~AbstractNodeGeom3dIO() noexcept = default;

Result<> AbstractNodeGeom3dIO::ReadNodeGeom3dData(DataStructureReader& dataStructureReader, AbstractNodeGeometry3D& geom, const group_reader_type& parentGroup, const std::string& objectName,
                                                  AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore)
{
  Result<> result = AbstractNodeGeom2dIO::ReadNodeGeom2dData(dataStructureReader, geom, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);

  geom.setPolyhedronListId(ReadDataId(groupReader, IOConstants::k_PolyhedronListTag));
  geom.setPolyhedraDataId(ReadDataId(groupReader, IOConstants::k_PolyhedronDataTag));
  geom.setUnsharedFacedId(ReadDataId(groupReader, IOConstants::k_UnsharedFaceListTag));

  // Required data
  if(useEmptyDataStore)
  {
    dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_PolyhedronListTag));
    dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_PolyhedronDataTag));
    dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_UnsharedFaceListTag));
  }

  return {};
}

Result<> AbstractNodeGeom3dIO::FinishImportingNodeGeom3dData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup)
{
  auto* geom = dataStructure.getDataAs<AbstractNodeGeometry3D>(dataPath);
  if(geom == nullptr)
  {
    return MakeErrorResult(-50592, fmt::format("Failed to finish importing AbstractNodeGeometry3D at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  return AbstractNodeGeom2dIO::FinishImportingNodeGeom2dData(dataStructure, dataPath, dataStructureGroup);
}

Result<> AbstractNodeGeom3dIO::WriteNodeGeom3dData(DataStructureWriter& dataStructureWriter, const AbstractNodeGeometry3D& geom, group_writer_type& parentGroup, bool importable)
{
  Result<> result = AbstractNodeGeom2dIO::WriteNodeGeom2dData(dataStructureWriter, geom, parentGroup, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroup.createGroup(geom.getName());

  result = WriteDataId(groupWriter, geom.getPolyhedronListId(), IOConstants::k_PolyhedronListTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geom.getPolyhedraDataId(), IOConstants::k_PolyhedronDataTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geom.getUnsharedFacesId(), IOConstants::k_UnsharedFaceListTag);
  if(result.invalid())
  {
    return result;
  }

  return {};
}
} // namespace nx::core::HDF5
