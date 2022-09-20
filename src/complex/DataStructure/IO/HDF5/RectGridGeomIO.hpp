#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT RectGridGeomIO : public IDataIO
{
public:
  RectGridGeomIO();
  virtual ~RectGridGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  RectGridGeomIO(const RectGridGeomIO& other) = delete;
  RectGridGeomIO(RectGridGeomIO&& other) = delete;
  RectGridGeomIO& operator=(const RectGridGeomIO& rhs) = delete;
  RectGridGeomIO& operator=(RectGridGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
