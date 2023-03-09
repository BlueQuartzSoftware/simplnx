#include "HexahedralGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"

#include "complex/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"

namespace complex::HDF5
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
Result<> HexahedralGeomIO::writeData(DataStructureWriter& structureReader, const HexahedralGeom& geom, group_writer_type& parentGroup, bool importable) const
{
  auto groupWriter = parentGroup.createGroupWriter(geom.getName());
  return INodeGeom3dIO::WriteNodeGeom3dData(structureReader, geom, parentGroup, importable);
}

Result<> HexahedralGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
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
