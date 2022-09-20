#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT EdgeGeomIO : public IDataIO
{
public:
  EdgeGeomIO();
  virtual ~EdgeGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  EdgeGeomIO(const EdgeGeomIO& other) = delete;
  EdgeGeomIO(EdgeGeomIO&& other) = delete;
  EdgeGeomIO& operator=(const EdgeGeomIO& rhs) = delete;
  EdgeGeomIO& operator=(EdgeGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
