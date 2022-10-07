#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "complex/DataStructure/IO/HDF5/INodeGeom1dIO.hpp"

namespace complex
{
class EdgeGeom;

namespace HDF5
{
class COMPLEX_EXPORT EdgeGeomIO : public INodeGeom1dIO
{
public:
  using data_type = EdgeGeom;

  EdgeGeomIO();
  virtual ~EdgeGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const EdgeGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  EdgeGeomIO(const EdgeGeomIO& other) = delete;
  EdgeGeomIO(EdgeGeomIO&& other) = delete;
  EdgeGeomIO& operator=(const EdgeGeomIO& rhs) = delete;
  EdgeGeomIO& operator=(EdgeGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
