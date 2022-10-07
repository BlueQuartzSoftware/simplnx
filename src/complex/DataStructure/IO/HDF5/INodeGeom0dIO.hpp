#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/IGeometryIO.hpp"

namespace complex
{
class INodeGeometry0D;

namespace HDF5
{
class COMPLEX_EXPORT INodeGeom0dIO : public IGeometryIO
{
public:
  INodeGeom0dIO();
  virtual ~INodeGeom0dIO() noexcept;

  INodeGeom0dIO(const INodeGeom0dIO& other) = delete;
  INodeGeom0dIO(INodeGeom0dIO&& other) = delete;
  INodeGeom0dIO& operator=(const INodeGeom0dIO& rhs) = delete;
  INodeGeom0dIO& operator=(INodeGeom0dIO&& rhs) = delete;

protected:
  static Result<> ReadNodeGeom0dData(DataStructureReader& structureReader, INodeGeometry0D& geom, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                     const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);
  static Result<> WriteNodeGeom0dData(DataStructureWriter& structureReader, const INodeGeometry0D& geom, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace complex
