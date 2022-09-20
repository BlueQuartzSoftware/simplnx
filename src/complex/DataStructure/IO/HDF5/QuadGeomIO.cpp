#include "QuadGeomIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
QuadGeomIO::QuadGeomIO() = default;
QuadGeomIO::~QuadGeomIO() noexcept = default;

Result<> QuadGeomIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                              const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
}
Result<> QuadGeomIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
{
}
} // namespace complex::HDF5
