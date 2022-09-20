#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT GridMontageIO : public IDataIO
{
public:
  GridMontageIO();
  virtual ~GridMontageIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  GridMontageIO(const GridMontageIO& other) = delete;
  GridMontageIO(GridMontageIO&& other) = delete;
  GridMontageIO& operator=(const GridMontageIO& rhs) = delete;
  GridMontageIO& operator=(GridMontageIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
