#include "AttributeMatrixIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/Readers/AttributeReader.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"

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

  auto groupReader = parentGroup.openGroup(objectName);
  auto attribute = groupReader.getAttribute(IOConstants::k_TupleDims);
  auto tupleShape = attribute.readAsVector<usize>();

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
  auto groupWriter = parentGroup.createGroupWriter(attributeMatrix.getName());
  auto tupleShape = attributeMatrix.getShape();
  auto tupleDimsAttribute = groupWriter.createAttribute(IOConstants::k_TupleDims);
  auto error = tupleDimsAttribute.writeVector(nx::core::HDF5::AttributeWriter::DimsVector{tupleShape.size()}, tupleShape);

  return WriteBaseGroupData(dataStructureWriter, attributeMatrix, parentGroup, importable);
}

Result<> AttributeMatrixIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
