#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace complex
{
class IGeometry;

namespace HDF5
{
class COMPLEX_EXPORT IGeometryIO : public BaseGroupIO
{
public:
  IGeometryIO();
  virtual ~IGeometryIO() noexcept;

  IGeometryIO(const IGeometryIO& other) = delete;
  IGeometryIO(IGeometryIO&& other) = delete;
  IGeometryIO& operator=(const IGeometryIO& rhs) = delete;
  IGeometryIO& operator=(IGeometryIO&& rhs) = delete;

protected:
  static Result<> ReadGeometryData(DataStructureReader& structureReader, IGeometry& geometry, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                   const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);
  static Result<> WriteGeometryData(DataStructureWriter& structureReader, const IGeometry& geometry, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace complex
