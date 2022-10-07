#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/INodeGeom0dIO.hpp"

namespace complex
{
class INodeGeometry1D;

namespace HDF5
{
class COMPLEX_EXPORT INodeGeom1dIO : public INodeGeom0dIO
{
public:
  INodeGeom1dIO();
  virtual ~INodeGeom1dIO() noexcept;

  INodeGeom1dIO(const INodeGeom1dIO& other) = delete;
  INodeGeom1dIO(INodeGeom1dIO&& other) = delete;
  INodeGeom1dIO& operator=(const INodeGeom1dIO& rhs) = delete;
  INodeGeom1dIO& operator=(INodeGeom1dIO&& rhs) = delete;

protected:
  static Result<> ReadNodeGeom1dData(DataStructureReader& structureReader, INodeGeometry1D& geometry, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                     const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);
  static Result<> WriteNodeGeom1dData(DataStructureWriter& structureReader, const INodeGeometry1D& geometry, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace complex
