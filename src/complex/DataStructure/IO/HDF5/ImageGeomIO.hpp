#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT ImageGeomIO : public IDataIO
{
public:
  ImageGeomIO();
  virtual ~ImageGeomIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const override;

  ImageGeomIO(const ImageGeomIO& other) = delete;
  ImageGeomIO(ImageGeomIO&& other) = delete;
  ImageGeomIO& operator=(const ImageGeomIO& rhs) = delete;
  ImageGeomIO& operator=(ImageGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
