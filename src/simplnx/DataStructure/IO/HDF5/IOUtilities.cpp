#include "IOUtilities.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataMap.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/ObjectIO.hpp"

#include "fmt/format.h"

namespace nx::core
{
Result<> HDF5::WriteObjectAttributes(DataStructureWriter& dataStructureWriter, nx::core::HDF5::ObjectIO& objectWriter, const DataObject* dataObject, bool importable)
{
  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addWriter(objectWriter, dataObject->getId());

  int32 importablei32 = (importable ? 1 : 0);
  objectWriter.writeStringAttribute(Constants::k_ObjectTypeTag, dataObject->getTypeName());
  objectWriter.writeScalarAttribute(Constants::k_ObjectIdTag, dataObject->getId());
  objectWriter.writeScalarAttribute(Constants::k_ImportableTag, importablei32);

  return {};
}

Result<> HDF5::ReadBaseGroup(DataStructureReader& dataStructureReader, const nx::core::HDF5::GroupIO& groupReader, BaseGroup* baseGroup, bool useEmptyDataStores)
{
  return ReadDataMap(dataStructureReader, baseGroup->getDataMap(), groupReader, baseGroup->getId(), useEmptyDataStores);
}

Result<> HDF5::ReadDataMap(DataStructureReader& dataStructureReader, DataMap& dataMap, const nx::core::HDF5::GroupIO& groupReader, DataObject::IdType parentId, bool useEmptyDataStore)
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

Result<> HDF5::WriteBaseGroup(DataStructureWriter& dataStructureWriter, nx::core::HDF5::GroupIO& parentGroupIO, const BaseGroup* baseGroup, bool importable)
{
  auto groupWriterResult = parentGroupIO.createGroup(baseGroup->getName());
  if (groupWriterResult.invalid())
  {
    return ConvertResult(std::move(groupWriterResult));
  }
  auto groupWriter = std::move(groupWriterResult.value());

  Result<> error = WriteObjectAttributes(dataStructureWriter, groupWriter, baseGroup, importable);
  if(error.invalid())
  {
    return error;
  }

  return WriteDataMap(dataStructureWriter, groupWriter, baseGroup->getDataMap());
}

Result<> HDF5::WriteDataMap(DataStructureWriter& dataStructureWriter, nx::core::HDF5::GroupIO& h5Group, const DataMap& dataMap)
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
