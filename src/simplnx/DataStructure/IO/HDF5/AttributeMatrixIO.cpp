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

  auto groupReader = parentGroup.openGroup(objectName);

  std::vector<usize> tupleShape;
  auto tupleShapeResult = groupReader.readVectorAttribute<usize>(IOConstants::k_TupleDims);
  if(tupleShapeResult.invalid())
  {
    return ConvertResult(std::move(tupleShapeResult));
  }
  tupleShape = std::move(tupleShapeResult.value());

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
  auto groupWriter = parentGroup.createGroup(attributeMatrix.getName());
  auto tupleShape = attributeMatrix.getShape();
  groupWriter.writeVectorAttribute(IOConstants::k_TupleDims, tupleShape);

  return WriteBaseGroupData(dataStructureWriter, attributeMatrix, parentGroup, importable);
}

Result<> AttributeMatrixIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
