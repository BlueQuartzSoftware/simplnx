#include "AttributeMatrixIO.hpp"

#include "DataStructureReader.hpp"
#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/IO/Generic/IOConstants.hpp"

#include "complex/Utilities/Parsing/HDF5/Readers/AttributeReader.hpp"
#include "complex/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"

namespace complex::HDF5
{
AttributeMatrixIO::AttributeMatrixIO() = default;
AttributeMatrixIO::~AttributeMatrixIO() noexcept = default;

DataObject::Type AttributeMatrixIO::getDataType() const
{
  return DataObject::Type::AttributeMatrix;
}

std::string AttributeMatrixIO::getTypeName() const
{
  return data_type::GetTypeName();
}

Result<> AttributeMatrixIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                     const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* dataObject = data_type::Import(structureReader.getDataStructure(), objectName, importId, parentId);

  auto groupReader = parentGroup.openGroup(objectName);
  auto attribute = groupReader.getAttribute(IOConstants::k_TupleDims);
  auto tupleShape = attribute.readAsVector<usize>();

  if(tupleShape.empty())
  {
    std::string ss = "Failed to read AttributeMatrix tuple shape";
    return MakeErrorResult(-1, ss);
  }

  dataObject->setShape(tupleShape);
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
  auto error = tupleDimsAttribute.writeVector(complex::HDF5::AttributeWriter::DimsVector{tupleShape.size()}, tupleShape);

  return WriteBaseGroupData(dataStructureWriter, attributeMatrix, parentGroup, importable);
}

Result<> AttributeMatrixIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  const auto* targetData = dynamic_cast<const data_type*>(dataObject);
  if(targetData == nullptr)
  {
    std::string ss = "Provided DataObject could not be cast to the target type";
    return MakeErrorResult(-800, ss);
  }

  return writeData(dataStructureWriter, *targetData, parentWriter, true);
}
} // namespace complex::HDF5
