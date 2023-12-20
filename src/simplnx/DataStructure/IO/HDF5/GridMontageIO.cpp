#include "GridMontageIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"

#include "simplnx/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"

namespace nx::core::HDF5
{
GridMontageIO::GridMontageIO() = default;
GridMontageIO::~GridMontageIO() noexcept = default;

DataObject::Type GridMontageIO::getDataType() const
{
  return DataObject::Type::AbstractMontage;
}

std::string GridMontageIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> GridMontageIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                 const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* montage = GridMontage::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return BaseGroupIO::ReadBaseGroupData(structureReader, *montage, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}
Result<> GridMontageIO::writeData(DataStructureWriter& dataStructureWriter, const GridMontage& montage, group_writer_type& parentGroup, bool importable) const
{
  auto groupWriter = parentGroup.createGroupWriter(montage.getName());
  return BaseGroupIO::WriteBaseGroupData(dataStructureWriter, montage, parentGroup, importable);
}

Result<> GridMontageIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
