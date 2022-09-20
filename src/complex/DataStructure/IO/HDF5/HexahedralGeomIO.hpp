#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT HexahedralGeomIO : public IDataIO
{
public:
  HexahedralGeomIO();
  virtual ~HexahedralGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  HexahedralGeomIO(const HexahedralGeomIO& other) = delete;
  HexahedralGeomIO(HexahedralGeomIO&& other) = delete;
  HexahedralGeomIO& operator=(const HexahedralGeomIO& rhs) = delete;
  HexahedralGeomIO& operator=(HexahedralGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
