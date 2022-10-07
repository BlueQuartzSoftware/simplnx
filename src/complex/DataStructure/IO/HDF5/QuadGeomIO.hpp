#pragma once

#include "complex/DataStructure/IO/HDF5/INodeGeom2dIO.hpp"

namespace complex
{
class QuadGeom;

namespace HDF5
{
class COMPLEX_EXPORT QuadGeomIO : public INodeGeom2dIO
{
public:
  using data_type = QuadGeom;

  QuadGeomIO();
  virtual ~QuadGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const QuadGeom& geom, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  QuadGeomIO(const QuadGeomIO& other) = delete;
  QuadGeomIO(QuadGeomIO&& other) = delete;
  QuadGeomIO& operator=(const QuadGeomIO& rhs) = delete;
  QuadGeomIO& operator=(QuadGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
