#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/INodeGeom2dIO.hpp"

namespace complex
{
class INodeGeometry3D;

namespace HDF5
{
class COMPLEX_EXPORT INodeGeom3dIO : public INodeGeom2dIO
{
public:
  INodeGeom3dIO();
  virtual ~INodeGeom3dIO() noexcept;

  INodeGeom3dIO(const INodeGeom3dIO& other) = delete;
  INodeGeom3dIO(INodeGeom3dIO&& other) = delete;
  INodeGeom3dIO& operator=(const INodeGeom3dIO& rhs) = delete;
  INodeGeom3dIO& operator=(INodeGeom3dIO&& rhs) = delete;

protected:
  static Result<> ReadNodeGeom3dData(DataStructureReader& structureReader, INodeGeometry3D& geom3d, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                     const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);
  static Result<> WriteNodeGeom3dData(DataStructureWriter& structureReader, const INodeGeometry3D& geom3d, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace complex
