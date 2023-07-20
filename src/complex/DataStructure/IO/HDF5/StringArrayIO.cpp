#include "StringArrayIO.hpp"

#include "DataStructureReader.hpp"
#include "complex/DataStructure/StringArray.hpp"

#include "complex/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"

namespace
{
constexpr complex::StringLiteral k_TupleDimsAttrName = "TupleDimensions";
}

namespace complex::HDF5
{
StringArrayIO::StringArrayIO() = default;
StringArrayIO::~StringArrayIO() noexcept = default;

DataObject::Type StringArrayIO::getDataType() const
{
  return DataObject::Type::AttributeMatrix;
}

std::string StringArrayIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> StringArrayIO::readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                 const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto datasetReader = parentGroup.openDataset(objectName);
  std::string dataArrayName = datasetReader.getName();

  // Check importablility
  auto importableAttribute = datasetReader.getAttribute(Constants::k_ImportableTag);
  if(importableAttribute.isValid() && importableAttribute.readAsValue<int32>() == 0)
  {
    return {};
  }

  auto tupleDimsAttribReader = datasetReader.getAttribute(k_TupleDimsAttrName);
  uint64 numValues = tupleDimsAttribReader.readAsValue<uint64>();

  std::vector<std::string> strings = useEmptyDataStore ? std::vector<std::string>(numValues) : datasetReader.readAsVectorOfStrings();
  const auto* data = StringArray::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(strings), parentId);

  if(data == nullptr)
  {
    std::string ss = "Error importing DataArray";
    return MakeErrorResult(-404, ss);
  }

  return {};
}

Result<> StringArrayIO::writeData(DataStructureWriter& dataStructureWriter, const data_type& dataArray, group_writer_type& parentGroup, bool importable) const
{
  auto datasetWriter = parentGroup.createDatasetWriter(dataArray.getName());

  // writeVectorOfStrings may resize the collection
  data_type::collection_type strings = dataArray.values();
  auto result = datasetWriter.writeVectorOfStrings(strings);
  if(result.invalid())
  {
    return result;
  }

  // Write the number of values as an attribute for quicker preflight times
  {
    auto tupleDimsAttribWriter = datasetWriter.createAttribute(k_TupleDimsAttrName);
    result = tupleDimsAttribWriter.writeValue<uint64>(dataArray.size());
    if(result.invalid())
    {
      std::string ss = fmt::format("Error writing DataObject attribute: {}", k_TupleDimsAttrName);
      return MakeErrorResult(result.errors()[0].code, ss);
    }
  }

  return WriteObjectAttributes(dataStructureWriter, dataArray, datasetWriter, importable);
}

Result<> StringArrayIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace complex::HDF5
