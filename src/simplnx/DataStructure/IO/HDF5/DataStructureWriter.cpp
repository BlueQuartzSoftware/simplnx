#include "DataStructureWriter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataIOManager.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <fmt/format.h>

namespace nx::core::HDF5
{
DataStructureWriter::DataStructureWriter()
{
  auto instance = Application::GetOrCreateInstance();
  m_IOManager = std::dynamic_pointer_cast<DataIOManager>(instance->getIOManager("HDF5"));
}

DataStructureWriter::~DataStructureWriter() noexcept = default;

const DataStructureWriter::WriteOptions& DataStructureWriter::getWriteOptions() const noexcept
{
  return m_WriteOptions;
}

void DataStructureWriter::setWriteOptions(const WriteOptions& options) noexcept
{
  m_WriteOptions = options;
}

Result<> DataStructureWriter::WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath)
{
  return WriteFile(dataStructure, filepath, WriteOptions{});
}

Result<> DataStructureWriter::WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath, const WriteOptions& options)
{
  auto fileWriter = nx::core::HDF5::FileIO::WriteFile(filepath);
  if(fileWriter.isValid() == false)
  {
    return MakeErrorResult(-8054, fmt::format("Failed to create file at path {}", filepath.string()));
  }
  return WriteFile(dataStructure, fileWriter, options);
}

Result<> DataStructureWriter::WriteFile(const DataStructure& dataStructure, nx::core::HDF5::FileIO& FileIO)
{
  return WriteFile(dataStructure, FileIO, WriteOptions{});
}

Result<> DataStructureWriter::WriteFile(const DataStructure& dataStructure, nx::core::HDF5::FileIO& FileIO, const WriteOptions& options)
{
  HDF5::DataStructureWriter dataStructureWriter;
  dataStructureWriter.setWriteOptions(options);
  auto groupIO = FileIO.createGroup(Constants::k_DataStructureTag);
  return dataStructureWriter.writeDataStructure(dataStructure, groupIO);
}

Result<> DataStructureWriter::AppendFile(FileIO& file, const DataStructure& dataStructure, const DataPath& dataPath)
{
  return AppendFile(file, dataStructure, dataPath, WriteOptions{});
}

Result<> DataStructureWriter::AppendFile(FileIO& file, const DataStructure& dataStructure, const DataPath& dataPath, const WriteOptions& options)
{
  if(dataPath.empty())
  {
    return MakeErrorResult(-1, "DataPath must be non empty");
  }

  if(dataPath.getLength() != 1)
  {
    return MakeErrorResult(-2, "Object to append must be at the top level of the DataStructure");
  }

  if(!dataStructure.containsData(dataPath))
  {
    return MakeErrorResult(-3, fmt::format("Object doesn't exist at path '{}'", dataPath.toString()));
  }

  GroupIO dataStructureGroup = file.openGroup(Constants::k_DataStructureTag);
  if(!dataStructureGroup.isValid())
  {
    return MakeErrorResult(-5, fmt::format("Failed to open top-level DataStructure group in file '{}'", file.getFilePath().string()));
  }

  const std::string& targetName = dataPath[0];

  if(dataStructureGroup.exists(targetName))
  {
    return MakeErrorResult(-6, fmt::format("Cannot append because object '{}' already exists", targetName));
  }

  auto idResult = dataStructureGroup.readScalarAttribute<DataObject::IdType>(Constants::k_NextIdTag);
  if(idResult.invalid())
  {
    return ConvertResult(std::move(idResult));
  }
  DataObject::IdType nextObjectId = idResult.value();

  DataStructure dataStructureShallowCopy = dataStructure;

  for(DataObject* topLevelObject : dataStructureShallowCopy.getTopLevelData())
  {
    if(topLevelObject->getName() != targetName)
    {
      dataStructureShallowCopy.removeData(topLevelObject->getId());
    }
  }

  dataStructureShallowCopy.resetIds(nextObjectId);

  auto writeNextIdResult = dataStructureGroup.writeScalarAttribute(Constants::k_NextIdTag, dataStructureShallowCopy.getNextId());
  if(writeNextIdResult.invalid())
  {
    return writeNextIdResult;
  }

  const DataObject& dataObject = dataStructureShallowCopy.getDataRef(dataPath);
  HDF5::DataStructureWriter dataStructureWriter;
  dataStructureWriter.setWriteOptions(options);
  return dataStructureWriter.writeDataObject(&dataObject, dataStructureGroup);
}

Result<> DataStructureWriter::AppendFile(const std::filesystem::path& filepath, const DataStructure& dataStructure, const DataPath& dataPath)
{
  return AppendFile(filepath, dataStructure, dataPath, WriteOptions{});
}

Result<> DataStructureWriter::AppendFile(const std::filesystem::path& filepath, const DataStructure& dataStructure, const DataPath& dataPath, const WriteOptions& options)
{
  auto file = FileIO::AppendFile(filepath);
  if(!file.isValid())
  {
    return MakeErrorResult(-4, fmt::format("Unable to open file '{}'", filepath.string()));
  }

  return AppendFile(file, dataStructure, dataPath, options);
}

Result<> DataStructureWriter::writeDataObject(const DataObject* dataObject, nx::core::HDF5::GroupIO& parentGroup)
{
  if(hasDataBeenWritten(dataObject))
  {
    // Reuse the existing object through an HDF5 hard link.
    return writeDataObjectLink(dataObject, parentGroup);
  }

  // Recovery writers let registered managers replace disk-backed arrays with
  // placeholder datasets and backing metadata. This preserves recovery links
  // without copying data. An empty override uses the normal type factory.
  if(auto overrideResult = Application::GetOrCreateInstance()->getIOCollection().onRecoveryWrite(*this, dataObject, parentGroup); overrideResult.has_value())
  {
    return overrideResult.value();
  }

  auto factory = m_IOManager->getFactoryAs<IDataIO>(dataObject->getTypeName());
  if(factory == nullptr)
  {
    std::string ss = fmt::format("Could not find IO factory for datatype: {}", dataObject->getTypeName());
    return MakeErrorResult(-5, ss);
  }

  auto result = factory->writeDataObject(*this, dataObject, parentGroup);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

Result<> DataStructureWriter::writeDataMap(const DataMap& dataMap, nx::core::HDF5::GroupIO& parentGroup)
{
  for(const auto& [key, object] : dataMap)
  {
    Result<> result = writeDataObject(object.get(), parentGroup);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

Result<> DataStructureWriter::writeDataStructure(const DataStructure& dataStructure, nx::core::HDF5::GroupIO& groupIO)
{
  if(!groupIO.isValid())
  {
    std::string ss = "Failed to write DataStructure to HDF5 group";
    return MakeErrorResult(-700, ss);
  }
  groupIO.writeScalarAttribute(Constants::k_NextIdTag, dataStructure.getNextId());
  return writeDataMap(dataStructure.getDataMap(), groupIO);
}

Result<> DataStructureWriter::writeDataObjectLink(const DataObject* dataObject, nx::core::HDF5::GroupIO& parentGroup)
{
  auto objectPath = getPathForObjectId(dataObject->getId());
  auto result = parentGroup.createLink(objectPath);
  if(result.invalid())
  {
    return result;
  }

  // NeighborList links require the NumNeighbors companion array for reconstruction.
  if(const auto* neighborList = dynamic_cast<const INeighborList*>(dataObject))
  {
    auto numNeighborsName = neighborList->getNumNeighborsArrayName();
    auto dataPath = getPathForObjectSibling(dataObject->getId(), numNeighborsName);
    result = parentGroup.createLink(dataPath);
    if(result.invalid())
    {
      return result;
    }
  }
  return {};
}

bool DataStructureWriter::hasDataBeenWritten(const DataObject* targetObject) const
{
  if(targetObject == nullptr)
  {
    return false;
  }
  return hasDataBeenWritten(targetObject->getId());
}

bool DataStructureWriter::hasDataBeenWritten(DataObject::IdType targetId) const
{
  return m_IdMap.find(targetId) != m_IdMap.end();
}

std::string DataStructureWriter::getPathForObjectId(DataObject::IdType objectId) const
{
  if(!hasDataBeenWritten(objectId))
  {
    return "";
  }
  return m_IdMap.at(objectId);
}

std::string DataStructureWriter::getParentPathForObjectId(DataObject::IdType objectId) const
{
  auto objectPath = getPathForObjectId(objectId);
  if(objectPath.empty())
  {
    return objectPath;
  }
  auto lastIndex = objectPath.find_last_of('/');
  if(lastIndex < 0)
  {
    return objectPath;
  }
  return objectPath.substr(0, lastIndex);
}

std::string DataStructureWriter::getPathForObjectSibling(DataObject::IdType objectId, const std::string& siblingName) const
{
  auto objectPath = getParentPathForObjectId(objectId);
  if(!objectPath.empty())
  {
    objectPath += "/";
  }
  objectPath += siblingName;
  return objectPath;
}

void DataStructureWriter::clearIdMap()
{
  m_IdMap.clear();
}

void DataStructureWriter::addWriter(nx::core::HDF5::ObjectIO& objectWriter, DataObject::IdType objectId)
{
  m_IdMap[objectId] = objectWriter.getObjectPath();
}
} // namespace nx::core::HDF5
