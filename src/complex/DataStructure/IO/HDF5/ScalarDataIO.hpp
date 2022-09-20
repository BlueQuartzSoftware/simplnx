#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
template <typename T>
class ScalarDataIO : public IDataIO
{
public:
  ScalarDataIO();
  virtual ~ScalarDataIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  ScalarDataIO(const ScalarDataIO& other) = delete;
  ScalarDataIO(ScalarDataIO&& other) = delete;
  ScalarDataIO& operator=(const ScalarDataIO& rhs) = delete;
  ScalarDataIO& operator=(ScalarDataIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex