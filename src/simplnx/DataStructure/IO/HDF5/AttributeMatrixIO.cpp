#include "AttributeMatrixIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::HDF5
{
AttributeMatrixIO::AttributeMatrixIO() = default;
AttributeMatrixIO::~AttributeMatrixIO() noexcept = default;

DataObject::Type AttributeMatrixIO::getDataType() const
{
  return DataObject::Type::AttributeMatrix;
}

std::string AttributeMatrixIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> AttributeMatrixIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                     const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{

  auto groupReaderResult = parentGroup.openGroup(objectName);
  if(groupReaderResult.invalid())
  {
    return ConvertResult(std::move(groupReaderResult));
  }
  auto groupReader = std::move(groupReaderResult.value());

  std::vector<usize> tupleShape;
  groupReader.readAttribute(IOConstants::k_TupleDims, tupleShape);

  if(tupleShape.empty())
  {
    return MakeErrorResult(-1550, fmt::format("Failed to read AttributeMatrix tuple shape"));
  }
  auto* dataObject = data_type::Import(structureReader.getDataStructure(), objectName, tupleShape, importId, parentId);

  Result<> result = BaseGroupIO::ReadBaseGroupData(structureReader, *dataObject, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

Result<> AttributeMatrixIO::writeData(DataStructureWriter& dataStructureWriter, const data_type& attributeMatrix, group_writer_type& parentGroup, bool importable) const
{
  auto groupWriterResult = parentGroup.createGroup(attributeMatrix.getName());
  if(groupWriterResult.invalid())
  {
    return ConvertResult(std::move(groupWriterResult));
  }
  auto groupWriter = std::move(groupWriterResult.value());

  auto tupleShape = attributeMatrix.getShape();
  groupWriter.createAttribute(IOConstants::k_TupleDims, tupleShape);

  return WriteBaseGroupData(dataStructureWriter, attributeMatrix, parentGroup, importable);
}

Result<> AttributeMatrixIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
