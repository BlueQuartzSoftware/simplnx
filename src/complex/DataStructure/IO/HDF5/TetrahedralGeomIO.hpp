#pragma once

#include "complex/DataStructure/IO/HDF5/INodeGeom3dIO.hpp"

namespace complex
{
class TetrahedralGeom;

namespace HDF5
{
class COMPLEX_EXPORT TetrahedralGeomIO : public INodeGeom3dIO
{
public:
  using data_type = TetrahedralGeom;

  TetrahedralGeomIO();
  virtual ~TetrahedralGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const TetrahedralGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  TetrahedralGeomIO(const TetrahedralGeomIO& other) = delete;
  TetrahedralGeomIO(TetrahedralGeomIO&& other) = delete;
  TetrahedralGeomIO& operator=(const TetrahedralGeomIO& rhs) = delete;
  TetrahedralGeomIO& operator=(TetrahedralGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex