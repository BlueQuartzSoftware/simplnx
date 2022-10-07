#include "TetrahedralGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/IO/Generic/IOConstants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

namespace complex::HDF5
{
TetrahedralGeomIO::TetrahedralGeomIO() = default;
TetrahedralGeomIO::~TetrahedralGeomIO() noexcept = default;

DataObject::Type TetrahedralGeomIO::getDataType() const
{
  return DataObject::Type::TetrahedralGeom;
}

std::string TetrahedralGeomIO::getTypeName() const
{
  return data_type::GetTypeName();
}

Result<> TetrahedralGeomIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                     const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometry = TetrahedralGeom::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return INodeGeom3dIO::ReadNodeGeom3dData(structureReader, *geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}
Result<> TetrahedralGeomIO::writeData(DataStructureWriter& dataStructureWriter, const TetrahedralGeom& geometry, group_writer_type& parentGroup, bool importable) const
{
  return INodeGeom3dIO::WriteNodeGeom3dData(dataStructureWriter, geometry, parentGroup, importable);
}

Result<> TetrahedralGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  auto* targetData = dynamic_cast<const data_type*>(dataObject);
  if(targetData == nullptr)
  {
    std::string ss = "Provided DataObject could not be cast to the target type";
    return MakeErrorResult(-800, ss);
  }

  return writeData(dataStructureWriter, *targetData, parentWriter, true);
}
} // namespace complex::HDF5
