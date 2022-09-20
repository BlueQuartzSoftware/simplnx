#include "HexahedralGeomIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
HexahedralGeomIO::HexahedralGeomIO() = default;
HexahedralGeomIO::~HexahedralGeomIO() noexcept = default;

Result<> HexahedralGeomIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
}
Result<> HexahedralGeomIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
{
}
} // namespace complex::HDF5
