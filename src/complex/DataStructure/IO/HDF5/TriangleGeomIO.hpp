#pragma once

#include "complex/DataStructure/IO/HDF5/INodeGeom3dIO.hpp"

namespace complex
{
class TriangleGeom;

namespace HDF5
{
class COMPLEX_EXPORT TriangleGeomIO : public INodeGeom3dIO
{
public:
  using data_type = TriangleGeom;

  TriangleGeomIO();
  virtual ~TriangleGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const TriangleGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  TriangleGeomIO(const TriangleGeomIO& other) = delete;
  TriangleGeomIO(TriangleGeomIO&& other) = delete;
  TriangleGeomIO& operator=(const TriangleGeomIO& rhs) = delete;
  TriangleGeomIO& operator=(TriangleGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
