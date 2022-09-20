#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT DataGroupIO : public IDataIO
{
public:
  DataGroupIO();
  virtual ~DataGroupIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  DataGroupIO(const DataGroupIO& other) = delete;
  DataGroupIO(DataGroupIO&& other) = delete;
  DataGroupIO& operator=(const DataGroupIO& rhs) = delete;
  DataGroupIO& operator=(DataGroupIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex