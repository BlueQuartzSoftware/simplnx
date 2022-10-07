#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/IGeometryIO.hpp"

namespace complex
{
class IGridGeometry;

namespace HDF5
{
class COMPLEX_EXPORT IGridGeometryIO : public IGeometryIO
{
public:
  IGridGeometryIO();
  virtual ~IGridGeometryIO() noexcept;

  IGridGeometryIO(const IGridGeometryIO& other) = delete;
  IGridGeometryIO(IGridGeometryIO&& other) = delete;
  IGridGeometryIO& operator=(const IGridGeometryIO& rhs) = delete;
  IGridGeometryIO& operator=(IGridGeometryIO&& rhs) = delete;

protected:
  static Result<> ReadGridGeometryData(DataStructureReader& structureReader, IGridGeometry& geometry, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                   const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);
  static Result<> WriteGridGeometryData(DataStructureWriter& structureReader, const IGridGeometry& geometry, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace complex
