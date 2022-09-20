#include "IOUtilities.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataMap.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectWriter.hpp"

#include "fmt/format.h"

namespace complex::HDF5
{
Result<> WriteObjectAttributes(DataStructureWriter& dataStructureWriter, H5::ObjectWriter& objectWriter, const DataObject* dataObject, bool importable)
{
  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addWriter(objectWriter, dataObject->getId());

  auto typeAttributeWriter = objectWriter.createAttribute(complex::Constants::k_ObjectTypeTag);
  Result<> error = typeAttributeWriter.writeString(dataObject->getTypeName());
  if(error.invalid())
  {
    return error;
  }

  auto idAttributeWriter = objectWriter.createAttribute(complex::Constants::k_ObjectIdTag);
  error = idAttributeWriter.writeValue(dataObject->getId());
  if(error.invalid())
  {
    return error;
  }

  auto importableAttributeWriter = objectWriter.createAttribute(complex::Constants::k_ImportableTag);
  error = importableAttributeWriter.writeValue<int32>(importable ? 1 : 0);
  if(error.invalid())
  {
    return error;
  }

  return {};
}

Result<> ReadBaseGroup(DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, BaseGroup* baseGroup, bool useEmptyDataStores)
{
  return ReadDataMap(dataStructureReader, groupReader, baseGroup->getId(), useEmptyDataStores);
}

Result<> ReadDataMap(DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, DataObject::IdType parentId, bool useEmptyDataStore)
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

Result<> WriteBaseGroup(DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, const BaseGroup* baseGroup, bool importable)
{
  auto groupWriter = parentGroupWriter.createGroupWriter(baseGroup->getName());
  Result<> error = WriteObjectAttributes(dataStructureWriter, groupWriter, baseGroup, importable);
  if(error.invalid())
  {
    return error;
  }

  return WriteDataMap(dataStructureWriter, groupWriter, baseGroup->getDataMap());
}

Result<> WriteDataMap(DataStructureWriter& dataStructureWriter, H5::GroupWriter& h5Group, const DataMap& dataMap)
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
} // namespace complex::HDF5
