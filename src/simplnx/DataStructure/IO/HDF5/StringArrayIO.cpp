#include "StringArrayIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace
{
constexpr nx::core::StringLiteral k_TupleDimsAttrName = "TupleDimensions";
}

namespace nx::core::HDF5
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

  // Check ability to import the data
  auto importableResult = datasetReader.readScalarAttribute<int32>(Constants::k_ImportableTag);
  if (importableResult.invalid())
  {
    return ConvertResult(std::move(importableResult));
  }
  int32 importable = std::move(importableResult.value());
  if(importable == 0)
  {
    return {};
  }

  
  auto numValuesResult = datasetReader.readScalarAttribute<uint64>(k_TupleDimsAttrName);
  if(numValuesResult.invalid())
  {
    return ConvertResult(std::move(numValuesResult));
  }
  uint64 numValues = std::move(numValuesResult.value());

  std::vector<std::string> strings = useEmptyDataStore ? std::vector<std::string>(numValues) : datasetReader.readAsVectorOfStrings();
  const auto* data = StringArray::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(strings), parentId);

  if(data == nullptr)
  {
    return MakeErrorResult(-404, fmt::format("Error importing DataArray with name '{}' that is a child of group '{}'", dataArrayName, parentGroup.getName()));
  }

  return {};
}

Result<> StringArrayIO::writeData(DataStructureWriter& dataStructureWriter, const data_type& dataArray, group_writer_type& parentGroup, bool importable) const
{
  auto datasetWriter = parentGroup.createDataset(dataArray.getName());
  
  // writeVectorOfStrings may resize the collection
  data_type::collection_type strings = dataArray.values();
  auto result = datasetWriter.writeVectorOfStrings(strings);
  if(result.invalid())
  {
    return result;
  }

  // Write the number of values as an attribute for quicker preflight times
  {
    uint64 value = dataArray.size();
    datasetWriter.writeScalarAttribute(k_TupleDimsAttrName, value);
  }

  return WriteObjectAttributes(dataStructureWriter, dataArray, datasetWriter, importable);
}

Result<> StringArrayIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
