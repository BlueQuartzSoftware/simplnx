#include "ObjectIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <H5Apublic.h>
#include <H5Spublic.h>
#include <H5Tpublic.h>

namespace nx::core::HDF5
{
ObjectIO::ObjectIO() = default;

ObjectIO::ObjectIO(GroupIO& group, const std::string& objectName)
: m_FilePath(group.getFilePath())
, m_ParentGroup(&group)
, m_ObjectName(objectName)
{
}

ObjectIO::ObjectIO(const std::filesystem::path& filepath, const std::string& objectName)
: m_FilePath(filepath)
, m_ObjectName(objectName)
{
}

ObjectIO::ObjectIO(ObjectIO&& other) noexcept
: m_FilePath(std::move(other.m_FilePath))
, m_ObjectName(std::move(other.m_ObjectName))
, m_ParentGroup(std::move(other.m_ParentGroup))
{
}

ObjectIO& ObjectIO::operator=(ObjectIO&& other) noexcept
{
  m_FilePath = std::move(other.m_FilePath);
  m_ObjectName = std::move(other.m_ObjectName);
  m_ParentGroup = std::move(other.m_ParentGroup);
  return *this;
}

ObjectIO::~ObjectIO() noexcept
{
}

bool ObjectIO::isValid() const
{
  return std::filesystem::exists(m_FilePath);
}

std::string ObjectIO::getName() const
{
  return m_ObjectName;
}

std::string ObjectIO::getObjectPath() const
{
  std::string parentPath = "";
  if(m_ParentGroup.has_value())
  {
    parentPath = m_ParentGroup.value()->getObjectPath();
  }
  return parentPath + "/" + m_ObjectName;
}

std::string ObjectIO::getParentName() const
{
  if(m_ParentGroup.has_value())
  {
    return m_ParentGroup.value()->getName();
  }
  return "";
}

void ObjectIO::setFilePath(const std::filesystem::path& filepath)
{
  m_FilePath = filepath;
}

void ObjectIO::setName(const std::string& name)
{
  m_ObjectName = name;
}

void ObjectIO::moveObj(ObjectIO&& rhs) noexcept
{
  m_FilePath = std::move(rhs.m_FilePath);
  m_ObjectName = std::move(rhs.m_ObjectName);
  m_ParentGroup = std::move(rhs.m_ParentGroup);
}

void ObjectIO::deleteAttributes()
{
  auto attributeNames = getAttributeNames();
  for (const auto& attributeName : attributeNames)
  {
    deleteAttribute(attributeName);
  }
}

std::filesystem::path ObjectIO::getFilePath() const
{
  return m_FilePath;
}

FileIO* ObjectIO::parentFile() const
{
  if (m_ParentGroup.has_value())
  {
    return m_ParentGroup.value()->parentFile();
  }
  return dynamic_cast<FileIO*>(const_cast<ObjectIO*>(this));
}

GroupIO* ObjectIO::parentGroup() const
{
  if (m_ParentGroup.has_value())
  {
    return m_ParentGroup.value();
  }
  return nullptr;
}

std::optional<HighFive::File> ObjectIO::h5File() const
{
  FileIO* fileIO = parentFile();
  if (fileIO == nullptr)
  {
    return {};
  }
  return fileIO->h5File();
}

std::string ObjectIO::readStringAttribute(int64 id) const
{
  std::string data;
  std::vector<char> attributeOutput;

  hid_t attrTypeId = H5Aget_type(id);
  htri_t isVariableString = H5Tis_variable_str(attrTypeId); // Test if the string is variable length
  if(isVariableString == 1)
  {
    data.clear();
    return data;
  }
  if(id >= 0)
  {
    hsize_t size = H5Aget_storage_size(id);
    attributeOutput.resize(static_cast<size_t>(size)); // Resize the vector to the proper length
    if(attrTypeId >= 0)
    {
      herr_t error = H5Aread(id, attrTypeId, attributeOutput.data());
      if(error < 0)
      {
        std::cout << "Error Reading Attribute." << std::endl;
      }
      else
      {
        if(attributeOutput[size - 1] == 0) // null Terminated string
        {
          size -= 1;
        }
        data.append(attributeOutput.data(),
                    size); // Append the data to the passed in string
      }
    }
  }

  return data;
}

Result<> ObjectIO::writeStringAttribute(int64 objectId, const std::string& attributeName, const std::string& text)
{
  Result<> returnError = {};
  size_t size = text.size();

  hid_t attributeType = H5Tcopy(H5T_C_S1);
  H5Tset_size(attributeType, size);
  H5Tset_strpad(attributeType, H5T_STR_NULLTERM);
  hid_t attributeSpaceID = H5Screate(H5S_SCALAR);
  hid_t attributeId = H5Acreate(objectId, attributeName.c_str(), attributeType, attributeSpaceID, H5P_DEFAULT, H5P_DEFAULT);
  herr_t error = H5Awrite(attributeId, attributeType, text.c_str());
  if(error < 0)
  {
    returnError = MakeErrorResult(error, "Error Writing String Attribute");
  }
  H5Aclose(attributeId);
  H5Sclose(attributeSpaceID);
  H5Tclose(attributeType);

  return returnError;
}
} // namespace nx::core::HDF5
