#include "StringArrayIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/EmptyStringStore.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/DataStructure/StringStore.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <fmt/format.h>
#include <hdf5.h>

#include <memory>
#include <mutex>
#include <stdexcept>

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

  // The importable attribute excludes values unavailable in this file.
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
  else
  {
    // Older files omit TupleDimensions; their dataspace gives the string count.
    // getDimensions() returns the string type size for string datasets, so use the dataspace.
    const hid_t datasetId = datasetReader.getId();
    hssize_t npoints = -1;
    {
      const std::lock_guard<std::mutex> hdf5Lock(nx::core::HDF5::Support::ApiLock());
      const hid_t dataspaceId = H5Dget_space(datasetId);
      if(dataspaceId >= 0)
      {
        npoints = H5Sget_simple_extent_npoints(dataspaceId);
        H5Sclose(dataspaceId);
      }
    }
    if(npoints < 0)
    {
      return MakeErrorResult(-151203, fmt::format("Cannot read the string count of dataset '{}' because it has no TupleDimensions attribute and its dataspace cannot be read.", dataArrayName));
    }
    tupleShape = ShapeType{static_cast<usize>(npoints)};
  }

  StringArray* data = nullptr;
  if(useEmptyDataStore)
  {
    data = StringArray::CreateWithStore(dataStructureReader.getDataStructure(), dataArrayName, std::make_shared<EmptyStringStore>(tupleShape), parentId, importId);
  }
  else
  {
    std::vector<std::string> strings = datasetReader.readAsVectorOfStrings();
    data = StringArray::Import(dataStructureReader.getDataStructure(), dataArrayName, tupleShape, importId, std::move(strings), parentId);
  }

  if(data == nullptr)
  {
    return MakeErrorResult(-404, fmt::format("Error importing StringArray '{}' in group '{}'. Check that TupleDimensions matches the string count and the parent accepts the array", dataArrayName,
                                             parentGroup.getName()));
  }

  return {};
}

Result<> StringArrayIO::writeData(DataStructureWriter& dataStructureWriter, const data_type& stringArray, group_writer_type& parentGroup, bool importable) const
{
  auto datasetWriter = parentGroup.createDataset(stringArray.getName());

  // writeVectorOfStrings can modify its argument, so write a copy of the values.
  data_type::collection_type strings = stringArray.values();
  auto result = datasetWriter.writeVectorOfStrings(strings);
  if(result.invalid())
  {
    return result;
  }

  // Tuple dimensions let metadata-only import create a placeholder without values.
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
  try
  {
    auto stringStore = std::make_shared<StringStore>(std::move(strings), stringArray->getTupleShape());
    stringArray->setStore(stringStore);
  } catch(const std::invalid_argument& error)
  {
    return MakeErrorResult(-151202, fmt::format("Cannot import strings at '{}': {}. Check the dataset values and TupleDimensions attribute.", dataPath.toString(), error.what()));
  }
  return {};
}

Result<> StringArrayIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
