#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT QuadGeomIO : public IDataIO
{
public:
  QuadGeomIO();
  virtual ~QuadGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  QuadGeomIO(const QuadGeomIO& other) = delete;
  QuadGeomIO(QuadGeomIO&& other) = delete;
  QuadGeomIO& operator=(const QuadGeomIO& rhs) = delete;
  QuadGeomIO& operator=(QuadGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
