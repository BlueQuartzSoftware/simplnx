#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
template <typename T>
class NeighborListIO : public IDataIO
{
public:
  NeighborListIO();
  virtual ~NeighborListIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  NeighborListIO(const NeighborListIO& other) = delete;
  NeighborListIO(NeighborListIO&& other) = delete;
  NeighborListIO& operator=(const NeighborListIO& rhs) = delete;
  DataANeighborListIOrrayIO& operator=(NeighborListIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
