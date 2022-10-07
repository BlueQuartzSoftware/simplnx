#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/INodeGeom1dIO.hpp"

namespace complex
{
class INodeGeometry2D;

namespace HDF5
{
class COMPLEX_EXPORT INodeGeom2dIO : public INodeGeom1dIO
{
public:
  INodeGeom2dIO();
  virtual ~INodeGeom2dIO() noexcept;

  INodeGeom2dIO(const INodeGeom2dIO& other) = delete;
  INodeGeom2dIO(INodeGeom2dIO&& other) = delete;
  INodeGeom2dIO& operator=(const INodeGeom2dIO& rhs) = delete;
  INodeGeom2dIO& operator=(INodeGeom2dIO&& rhs) = delete;

protected:
  static Result<> ReadNodeGeom2dData(DataStructureReader& structureReader, INodeGeometry2D& geometry, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                     const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);
  static Result<> WriteNodeGeom2dData(DataStructureWriter& structureReader, const INodeGeometry2D& geometry, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace complex
