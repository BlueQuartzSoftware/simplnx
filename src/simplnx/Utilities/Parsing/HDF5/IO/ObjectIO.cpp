#include "ObjectIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::HDF5
{
ObjectIO::ObjectIO() = default;

ObjectIO::ObjectIO(GroupIO* parentObj, const std::string& objectName)
: m_FilePath(parentObj->getFilePath())
, m_ParentGroup(parentObj)
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
, m_Id(std::move(other.m_Id))
{
}

ObjectIO& ObjectIO::operator=(ObjectIO&& other) noexcept
{
  m_Id = std::move(other.m_Id);
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
  if(m_ParentGroup != nullptr)
  {
    parentPath = m_ParentGroup->getObjectPath();
  }
  return parentPath + "/" + m_ObjectName;
}

std::string ObjectIO::getParentName() const
{
  if(m_ParentGroup == nullptr)
  {
    return "";
  }
  return m_ParentGroup->getName();
}

void ObjectIO::setFilePath(const std::filesystem::path& filepath)
{
  m_FilePath = filepath;
}

void ObjectIO::setName(const std::string& name)
{
  m_ObjectName = name;
}

hid_t ObjectIO::getId() const
{
  return m_Id;
}

void ObjectIO::setId(hid_t id) const
{
  m_Id = id;
}

void ObjectIO::setParentGroup(GroupIO* parent)
{
  m_ParentGroup = parent;
}

hid_t ObjectIO::getParentId() const
{
  if(m_ParentGroup == nullptr)
  {
    return -1;
  }
  return m_ParentGroup->getId();
}

ObjectIO::ObjectType ObjectIO::getObjectType() const
{
  if(!isValid())
  {
    return ObjectType::Unknown;
  }

  herr_t error = 1;
  H5O_info_t objectInfo{};

  error = H5Oget_info_by_name(getParentId(), getName().c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return ObjectType::Unknown;
  }

  int32 objectType = objectInfo.type;
  switch(objectType)
  {
  case H5O_TYPE_GROUP:
    return ObjectType::Group;
    break;
  case H5O_TYPE_DATASET:
    return ObjectType::Dataset;
    break;
  case H5O_TYPE_NAMED_DATATYPE:
    break;
  default:
    break;
  }

  return ObjectType::Unknown;
}

void ObjectIO::moveObj(ObjectIO&& rhs) noexcept
{
  m_FilePath = std::move(rhs.m_FilePath);
  m_ObjectName = std::move(rhs.m_ObjectName);
  m_ParentGroup = std::move(rhs.m_ParentGroup);
  m_Id = std::move(rhs.m_Id);
}

usize ObjectIO::getNumAttributes() const
{
  if(!isValid())
  {
    return 0;
  }

  return H5Aget_num_attrs(getId());
}

std::vector<std::string> ObjectIO::getAttributeNames() const
{
  auto numAttrib = getNumAttributes();
  std::vector<std::string> names(numAttrib);
  for(usize i = 0; i < numAttrib; i++)
  {
    names[i] = getAttributeNameByIndex(i);
  }
  return names;
}

std::string ObjectIO::getAttributeNameByIndex(int64 idx) const
{
  hid_t attrId = H5Aopen_idx(getId(), idx);
  const size_t size = 1024;
  char buffer[size];
  H5Aget_name(attrId, size, buffer);
  H5Aclose(attrId);
  return GetNameFromBuffer(buffer);
}

void ObjectIO::deleteAttribute(const std::string& name)
{
  H5Adelete(getId(), name.c_str());
}

void ObjectIO::deleteAttributes()
{
  auto attributeNames = getAttributeNames();
  for(const auto& attributeName : attributeNames)
  {
    deleteAttribute(attributeName);
  }
}

Result<std::string> ObjectIO::readStringAttribute(const std::string& attributeName) const
{
  std::string data;
  std::vector<char> attributeOutput;
  Result<std::string> returnResult = {};

  hid_t attribId = H5Aopen(getId(), attributeName.c_str(), H5P_DEFAULT);
  hid_t attrTypeId = H5Aget_type(attribId);
  auto isVariableString = H5Tis_variable_str(attrTypeId); // Test if the string is variable length
  if(isVariableString == 1)
  {
    data.clear();
    std::string ss = fmt::format("Cannot read attribute '{}'. Invalid string type.", attributeName);
    return MakeErrorResult<std::string>(-440, ss);
  }
  if(attribId >= 0)
  {
    hsize_t size = H5Aget_storage_size(attribId);
    attributeOutput.resize(static_cast<size_t>(size)); // Resize the vector to the proper length
    if(attrTypeId >= 0)
    {
      herr_t error = H5Aread(attribId, attrTypeId, attributeOutput.data());
      if(error < 0)
      {
        std::string ss = fmt::format("Error reading attribute: '{}'", attributeName);
        returnResult = MakeErrorResult<std::string>(-450, ss);
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
        returnResult = {data};
      }
    }
  }
  H5Aclose(attribId);
  return returnResult;
}

Result<> ObjectIO::writeStringAttribute(const std::string& attributeName, const std::string& text)
{
  Result<> returnError = {};
  size_t size = text.size();

  hid_t attributeType = H5Tcopy(H5T_C_S1);
  H5Tset_size(attributeType, size);
  H5Tset_strpad(attributeType, H5T_STR_NULLTERM);
  hid_t attributeSpaceID = H5Screate(H5S_SCALAR);
  hid_t attributeId = H5Acreate(getId(), attributeName.c_str(), attributeType, attributeSpaceID, H5P_DEFAULT, H5P_DEFAULT);
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

std::filesystem::path ObjectIO::getFilePath() const
{
  return m_FilePath;
}

FileIO* ObjectIO::parentFile() const
{
  if(m_ParentGroup != nullptr)
  {
    return m_ParentGroup->parentFile();
  }
  return dynamic_cast<FileIO*>(const_cast<ObjectIO*>(this));
}

GroupIO* ObjectIO::parentGroup() const
{
  return m_ParentGroup;
}

bool ObjectIO::hasAttribute(const std::string& attributeName) const
{
  return H5Aexists(getId(), attributeName.c_str()) > 0;
}

// std::optional<HighFive::File> ObjectIO::h5File() const
//{
//   FileIO* fileIO = parentFile();
//   if (fileIO == nullptr)
//   {
//     return {};
//   }
//   return fileIO->h5File();
// }

#if 0
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
#endif

#if 0
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
#endif

usize ObjectIO::getNumElementsInAttribute(hid_t attribId) const
{
  size_t typeSize = H5Tget_size(H5Aget_type(attribId));
  std::vector<hsize_t> dims;
  hid_t dataspaceId = H5Aget_space(attribId);
  if(dataspaceId >= 0)
  {
    Type type = getTypeFromId(H5Aget_type(attribId));
    if(type == Type::string)
    {
      size_t rank = 1;
      dims.resize(rank);
      dims[0] = typeSize;
    }
    else
    {
      size_t rank = H5Sget_simple_extent_ndims(dataspaceId);
      std::vector<hsize_t> hdims(rank, 0);
      /* Get dimensions */
      herr_t error = H5Sget_simple_extent_dims(dataspaceId, hdims.data(), nullptr);
      if(error < 0)
      {
        std::cout << "Error Getting Attribute dims" << std::endl;
        return 0;
      }
      // Copy the dimensions into the dims vector
      dims.clear(); // Erase everything in the Vector
      dims.resize(rank);
      std::copy(hdims.cbegin(), hdims.cend(), dims.begin());
    }
  }

  hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());
  return numElements;
}
} // namespace nx::core::HDF5
