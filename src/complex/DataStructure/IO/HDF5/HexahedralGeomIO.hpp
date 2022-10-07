#pragma once

#include "complex/DataStructure/IO/HDF5/INodeGeom3dIO.hpp"

namespace complex
{
class HexahedralGeom;

namespace HDF5
{
class COMPLEX_EXPORT HexahedralGeomIO : public INodeGeom3dIO
{
public:
  using data_type = HexahedralGeom;

  HexahedralGeomIO();
  virtual ~HexahedralGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureWriter, const HexahedralGeom& geom, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string HexahedralGeomIO::getTypeName() const override;

  HexahedralGeomIO(const HexahedralGeomIO& other) = delete;
  HexahedralGeomIO(HexahedralGeomIO&& other) = delete;
  HexahedralGeomIO& operator=(const HexahedralGeomIO& rhs) = delete;
  HexahedralGeomIO& operator=(HexahedralGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
