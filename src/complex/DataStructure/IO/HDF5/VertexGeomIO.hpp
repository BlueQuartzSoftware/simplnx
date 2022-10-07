#pragma once

#include "complex/DataStructure/IO/HDF5/INodeGeom0dIO.hpp"

namespace complex
{
class VertexGeom;

namespace HDF5
{
class COMPLEX_EXPORT VertexGeomIO : public INodeGeom0dIO
{
public:
  using data_type = VertexGeom;

  VertexGeomIO();
  virtual ~VertexGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const VertexGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  VertexGeomIO(const VertexGeomIO& other) = delete;
  VertexGeomIO(VertexGeomIO&& other) = delete;
  VertexGeomIO& operator=(const VertexGeomIO& rhs) = delete;
  VertexGeomIO& operator=(VertexGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
