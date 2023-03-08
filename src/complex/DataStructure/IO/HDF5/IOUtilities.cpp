#include "IOUtilities.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataMap.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureWriter.hpp"

#include "complex/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/GroupWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/ObjectWriter.hpp"

#include "fmt/format.h"

namespace complex
{
Result<> HDF5::WriteObjectAttributes(DataStructureWriter& dataStructureWriter, complex::HDF5::ObjectWriter& objectWriter, const DataObject* dataObject, bool importable)
{
  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addWriter(objectWriter, dataObject->getId());

  auto typeAttributeWriter = objectWriter.createAttribute(Constants::k_ObjectTypeTag);
  complex::HDF5::ErrorType error = typeAttributeWriter.writeString(dataObject->getTypeName());
  if(error < 0)
  {
    std::string ss = fmt::format("Could not write to attribute {}", Constants::k_ObjectTypeTag);
    return MakeErrorResult(error, ss);
  }

  auto idAttributeWriter = objectWriter.createAttribute(Constants::k_ObjectIdTag);
  error = idAttributeWriter.writeValue(dataObject->getId());
  if(error < 0)
  {
    std::string ss = fmt::format("Could not write to attribute {}", Constants::k_ObjectIdTag);
    return MakeErrorResult(error, ss);
  }

  auto importableAttributeWriter = objectWriter.createAttribute(Constants::k_ImportableTag);
  error = importableAttributeWriter.writeValue<int32>(importable ? 1 : 0);
  if(error < 0)
  {
    std::string ss = fmt::format("Could not write to attribute {}", Constants::k_ImportableTag);
    return MakeErrorResult(error, ss);
  }

  return {};
}

Result<> HDF5::ReadBaseGroup(DataStructureReader& dataStructureReader, const complex::HDF5::GroupReader& groupReader, BaseGroup* baseGroup, bool useEmptyDataStores)
{
  return ReadDataMap(dataStructureReader, baseGroup->getDataMap(), groupReader, baseGroup->getId(), useEmptyDataStores);
}

Result<> HDF5::ReadDataMap(DataStructureReader& dataStructureReader, DataMap& dataMap, const complex::HDF5::GroupReader& groupReader, DataObject::IdType parentId, bool useEmptyDataStore)
{
  auto childrenNames = groupReader.getChildNames();
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

Result<> HDF5::WriteBaseGroup(DataStructureWriter& dataStructureWriter, complex::HDF5::GroupWriter& parentGroupWriter, const BaseGroup* baseGroup, bool importable)
{
  auto groupWriter = parentGroupWriter.createGroupWriter(baseGroup->getName());
  Result<> error = WriteObjectAttributes(dataStructureWriter, groupWriter, baseGroup, importable);
  if(error.invalid())
  {
    return error;
  }

  return WriteDataMap(dataStructureWriter, groupWriter, baseGroup->getDataMap());
}

Result<> HDF5::WriteDataMap(DataStructureWriter& dataStructureWriter, complex::HDF5::GroupWriter& h5Group, const DataMap& dataMap)
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
} // namespace complex
