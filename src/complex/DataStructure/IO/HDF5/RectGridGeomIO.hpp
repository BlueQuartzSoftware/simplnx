#pragma once

#include "complex/DataStructure/IO/HDF5/IGridGeometryIO.hpp"

namespace complex
{
class RectGridGeom;

namespace HDF5
{
class COMPLEX_EXPORT RectGridGeomIO : public IGridGeometryIO
{
public:
  using data_type = RectGridGeom;

  RectGridGeomIO();
  virtual ~RectGridGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const RectGridGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  RectGridGeomIO(const RectGridGeomIO& other) = delete;
  RectGridGeomIO(RectGridGeomIO&& other) = delete;
  RectGridGeomIO& operator=(const RectGridGeomIO& rhs) = delete;
  RectGridGeomIO& operator=(RectGridGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
