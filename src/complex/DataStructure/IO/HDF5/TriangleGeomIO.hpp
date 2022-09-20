#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT DataArrayIO : public IDataIO
{
public:
  DataArrayIO();
  virtual ~DataArrayIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  DataArrayIO(const DataArrayIO& other) = delete;
  DataArrayIO(DataArrayIO&& other) = delete;
  DataArrayIO& operator=(const DataArrayIO& rhs) = delete;
  DataArrayIO& operator=(DataArrayIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
