#include "StringArrayFactory.hpp"

#include "complex/DataStructure/StringArray.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetReader.hpp"

namespace complex::H5
{
StringArrayFactory::StringArrayFactory()
: IDataFactory()
{
}
StringArrayFactory::~StringArrayFactory() = default;

std::string StringArrayFactory::getDataTypeName() const
{
  return StringArray::GetTypeName();
}

H5::ErrorType StringArrayFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                              const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  return -1;
}

H5::ErrorType StringArrayFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                                const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  std::string dataArrayName = datasetReader.getName();
  DataObject::IdType importId = ReadObjectId(datasetReader);

  // Check importablility
  auto importableAttribute = datasetReader.getAttribute(complex::Constants::k_ImportableTag);
  if(importableAttribute.isValid() && importableAttribute.readAsValue<int32>() == 0)
  {
    return 0;
  }

  std::vector<std::string> strings = preflight ? std::vector<std::string>{} : datasetReader.readAsVectorOfStrings();
  const auto* data = StringArray::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(strings), parentId);

  return (data == nullptr) ? -400 : 0;
}

} // namespace complex::H5
