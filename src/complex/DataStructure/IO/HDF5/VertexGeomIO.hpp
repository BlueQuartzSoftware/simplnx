#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT VertexGeomIO : public IDataIO
{
public:
  VertexGeomIO();
  virtual ~VertexGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  VertexGeomIO(const VertexGeomIO& other) = delete;
  VertexGeomIO(VertexGeomIO&& other) = delete;
  VertexGeomIO& operator=(const VertexGeomIO& rhs) = delete;
  VertexGeomIO& operator=(VertexGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
