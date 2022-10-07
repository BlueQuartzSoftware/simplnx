#pragma once

#include "complex/DataStructure/IO/HDF5/IGridGeometryIO.hpp"

namespace complex
{
class ImageGeom;

namespace HDF5
{
class COMPLEX_EXPORT ImageGeomIO : public IGridGeometryIO
{
public:
  using data_type = ImageGeom;

  ImageGeomIO();
  virtual ~ImageGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const ImageGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  ImageGeomIO(const ImageGeomIO& other) = delete;
  ImageGeomIO(ImageGeomIO&& other) = delete;
  ImageGeomIO& operator=(const ImageGeomIO& rhs) = delete;
  ImageGeomIO& operator=(ImageGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
