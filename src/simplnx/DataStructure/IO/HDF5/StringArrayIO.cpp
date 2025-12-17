#include "StringArrayIO.hpp"
#include <numeric>

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/DataStructure/StringStore.hpp"

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
  if(importableResult.invalid())
  {
    return ConvertResult(std::move(importableResult));
  }
  int32 importable = importableResult.value();
  if(importable == 0)
  {
    return {};
  }

  ShapeType tupleShape;
  auto tupleShapeResult = datasetReader.readVectorAttribute<usize>(k_TupleDimsAttrName);
  if(tupleShapeResult.valid())
  {
    tupleShape = std::move(tupleShapeResult.value());
  }
  usize numValues = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), 1ULL, std::multiplies<>());

  std::vector<std::string> strings = useEmptyDataStore ? std::vector<std::string>(numValues) : datasetReader.readAsVectorOfStrings();
  const auto* data = StringArray::Import(dataStructureReader.getDataStructure(), dataArrayName, tupleShape, importId, std::move(strings), parentId);

  if(data == nullptr)
  {
    return MakeErrorResult(-404, fmt::format("Error importing DataArray with name '{}' that is a child of group '{}'", dataArrayName, parentGroup.getName()));
  }

  return {};
}

Result<> StringArrayIO::writeData(DataStructureWriter& dataStructureWriter, const data_type& stringArray, group_writer_type& parentGroup, bool importable) const
{
  auto datasetWriter = parentGroup.createDataset(stringArray.getName());

  // writeVectorOfStrings may resize the collection
  data_type::collection_type strings = stringArray.values();
  auto result = datasetWriter.writeVectorOfStrings(strings);
  if(result.invalid())
  {
    return result;
  }

  // Write the number of values as an attribute for quicker preflight times
  {
    result = datasetWriter.writeVectorAttribute<usize>(k_TupleDimsAttrName, stringArray.getTupleShape());
    if(result.invalid())
    {
      return result;
    }
  }

  return WriteObjectAttributes(dataStructureWriter, stringArray, datasetWriter, importable);
}

Result<> StringArrayIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& parentGroupReader) const
{
  if(!dataStructure.containsData(dataPath))
  {
    return MakeErrorResult(-151200, fmt::format("Imported DataStructure Object at path '{}' does not exist.", dataPath.toString()));
  }

  auto* stringArray = dataStructure.getDataAs<data_type>(dataPath);
  if(stringArray == nullptr)
  {
    return MakeErrorResult(-151201, fmt::format("Imported DataStructure Object at path '{}' is not of the expected type.", dataPath.toString()));
  }

  auto datasetReader = parentGroupReader.openDataset(dataPath.getTargetName());
  std::vector<std::string> strings = datasetReader.readAsVectorOfStrings();
  auto stringStore = std::make_shared<StringStore>(std::move(strings), stringArray->getTupleShape());
  stringArray->setStore(stringStore);
  return {};
}

Result<> StringArrayIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
