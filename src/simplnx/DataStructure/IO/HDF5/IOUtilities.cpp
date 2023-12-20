#include "IOUtilities.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataMap.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"

#include "simplnx/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/GroupWriter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/ObjectWriter.hpp"

#include "fmt/format.h"

namespace nx::core
{
Result<> HDF5::WriteObjectAttributes(DataStructureWriter& dataStructureWriter, nx::core::HDF5::ObjectWriter& objectWriter, const DataObject* dataObject, bool importable)
{
  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addWriter(objectWriter, dataObject->getId());

  auto typeAttributeWriter = objectWriter.createAttribute(Constants::k_ObjectTypeTag);
  Result<> result = typeAttributeWriter.writeString(dataObject->getTypeName());
  if(result.invalid())
  {
    std::string ss = fmt::format("Could not write to attribute {}", Constants::k_ObjectTypeTag);
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  auto idAttributeWriter = objectWriter.createAttribute(Constants::k_ObjectIdTag);
  result = idAttributeWriter.writeValue(dataObject->getId());
  if(result.invalid())
  {
    std::string ss = fmt::format("Could not write to attribute {}", Constants::k_ObjectIdTag);
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  auto importableAttributeWriter = objectWriter.createAttribute(Constants::k_ImportableTag);
  result = importableAttributeWriter.writeValue<int32>(importable ? 1 : 0);
  if(result.invalid())
  {
    std::string ss = fmt::format("Could not write to attribute {}", Constants::k_ImportableTag);
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  return {};
}

Result<> HDF5::ReadBaseGroup(DataStructureReader& dataStructureReader, const nx::core::HDF5::GroupReader& groupReader, BaseGroup* baseGroup, bool useEmptyDataStores)
{
  return ReadDataMap(dataStructureReader, baseGroup->getDataMap(), groupReader, baseGroup->getId(), useEmptyDataStores);
}

Result<> HDF5::ReadDataMap(DataStructureReader& dataStructureReader, DataMap& dataMap, const nx::core::HDF5::GroupReader& groupReader, DataObject::IdType parentId, bool useEmptyDataStore)
{
  auto childrenNames = groupReader.getChildNames();
  if(childrenNames.empty())
  {
    return {};
  }

  for(const auto& childName : childrenNames)
  {
    Result<> error = dataStructureReader.readObjectFromGroup(groupReader, childName, parentId, useEmptyDataStore);
    if(error.invalid())
    {
      return error;
    }
  }
  return {};
}

Result<> HDF5::WriteBaseGroup(DataStructureWriter& dataStructureWriter, nx::core::HDF5::GroupWriter& parentGroupWriter, const BaseGroup* baseGroup, bool importable)
{
  auto groupWriter = parentGroupWriter.createGroupWriter(baseGroup->getName());
  Result<> error = WriteObjectAttributes(dataStructureWriter, groupWriter, baseGroup, importable);
  if(error.invalid())
  {
    return error;
  }

  return WriteDataMap(dataStructureWriter, groupWriter, baseGroup->getDataMap());
}

Result<> HDF5::WriteDataMap(DataStructureWriter& dataStructureWriter, nx::core::HDF5::GroupWriter& h5Group, const DataMap& dataMap)
{
  for(const auto& [id, dataObject] : dataMap)
  {
    Result<> error = dataStructureWriter.writeDataObject(dataObject.get(), h5Group);
    if(error.invalid())
    {
      return error;
    }
  }
  return {};
}
} // namespace nx::core
