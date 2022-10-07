#include "TriangleGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/IO/Generic/IOConstants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
TriangleGeomIO::TriangleGeomIO() = default;
TriangleGeomIO::~TriangleGeomIO() noexcept = default;

DataObject::Type TriangleGeomIO::getDataType() const
{
  return DataObject::Type::TriangleGeom;
}

std::string TriangleGeomIO::getTypeName() const
{
  return data_type::GetTypeName();
}

Result<> TriangleGeomIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                  const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometry = TriangleGeom::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return INodeGeom2dIO::ReadNodeGeom2dData(structureReader, *geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}
Result<> TriangleGeomIO::writeData(DataStructureWriter& dataStructureWriter, const TriangleGeom& geometry, group_writer_type& parentGroupWriter, bool importable) const
{
  return INodeGeom2dIO::WriteNodeGeom2dData(dataStructureWriter, geometry, parentGroupWriter, importable);
}

Result<> TriangleGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
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
