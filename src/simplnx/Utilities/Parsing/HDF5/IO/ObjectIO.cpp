#include "ObjectIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <mutex>

namespace nx::core::HDF5
{
ObjectIO::ObjectIO() = default;

ObjectIO::ObjectIO(hid_t parentId, const std::string& objectName)
: m_ParentId(parentId)
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
, m_ParentId(std::move(other.m_ParentId))
, m_Id(std::move(other.m_Id))
{
  other.m_Id = -1;
}

ObjectIO& ObjectIO::operator=(ObjectIO&& other) noexcept
{
  m_Id = std::move(other.m_Id);
  m_FilePath = std::move(other.m_FilePath);
  m_ObjectName = std::move(other.m_ObjectName);
  m_ParentId = std::move(other.m_ParentId);

  other.m_Id = -1;
  return *this;
}

ObjectIO::~ObjectIO() noexcept
{
}

bool ObjectIO::isValid() const
{
  return m_Id > 0;
}

std::string ObjectIO::getName() const
{
  if(m_ObjectName.empty())
  {
    return {};
  }
  open();
  return nx::core::HDF5::GetNameFromBuffer(m_ObjectName);
}

std::string ObjectIO::getNamePath() const
{
  return m_ObjectName;
}

std::string ObjectIO::getObjectPath() const
{
  if(!isValid())
  {
    return getName();
  }
  std::string path = "/";
  path += Support::GetObjectPath(getId());
  return path;
}

std::string ObjectIO::getParentName() const
{
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

bool ObjectIO::isOpen() const
{
  return m_Id > 0;
}

hid_t ObjectIO::getId() const
{
  if(m_Id <= 0)
  {
    return open();
  }
  return m_Id;
}

void ObjectIO::setId(hid_t id) const
{
  m_Id = id;
}

void ObjectIO::setParentId(hid_t parentId)
{
  m_ParentId = parentId;
}

hid_t ObjectIO::getParentId() const
{
  return m_ParentId;
}

ObjectIO::ObjectType ObjectIO::getObjectType() const
{
  if(!isValid())
  {
    return ObjectType::Unknown;
  }

  // getName() self-locks (it calls open()); resolve the parent id and name before the
  // leaf-locked bare H5Oget_info_by_name3.
  const hid_t parentId = getParentId();
  const std::string name = getName();
  herr_t error = 1;
  H5O_info2_t objectInfo{};
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    error = H5Oget_info_by_name3(parentId, name.c_str(), &objectInfo, H5O_INFO_BASIC, H5P_DEFAULT);
  }
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
  m_ParentId = std::move(rhs.m_ParentId);
  m_Id = std::move(rhs.m_Id);
}

usize ObjectIO::getNumAttributes() const
{
  if(!isValid())
  {
    return 0;
  }

  // getId() self-locks; resolve it before the leaf-locked bare H5Aget_num_attrs.
  const hid_t selfId = getId();
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  return H5Aget_num_attrs(selfId);
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
  // getId() self-locks; resolve it before the leaf-locked bare H5A* calls, which form one
  // leaf critical section (open, query name, close).
  const hid_t selfId = getId();
  const size_t size = 1024;
  char buffer[size];
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    hid_t attrId = H5Aopen_idx(selfId, idx);
    H5Aget_name(attrId, size, buffer);
    H5Aclose(attrId);
  }
  return GetNameFromBuffer(buffer);
}

void ObjectIO::deleteAttribute(const std::string& name)
{
  // getId() self-locks; resolve it before the leaf-locked existence-check + delete.
  const hid_t selfId = getId();
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  if(H5Aexists(selfId, name.c_str()))
  {
    H5Adelete(selfId, name.c_str());
  }
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

  // Resolve self-locking accessors before the attribute leaf critical section.
  open();
  if(!hasAttribute(attributeName))
  {
    return MakeErrorResult<std::string>(-445, fmt::format("Attribute '{}' does not exist in Object '{}'", attributeName, getName()));
  }

  const hid_t selfId = getId();
  bool isVariableString = false;
  herr_t readError = 0;
  bool attributeOpened = false;
  {
    // One leaf critical section spanning the bare H5A*/H5T* open, read and close calls.
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    hid_t attribId = H5Aopen(selfId, attributeName.c_str(), H5P_DEFAULT);
    hid_t attrTypeId = H5Aget_type(attribId);
    if(H5Tis_variable_str(attrTypeId) == 1) // Test if the string is variable length
    {
      H5Aclose(attribId);
      H5Tclose(attrTypeId);
      isVariableString = true;
    }
    else
    {
      if(attribId >= 0)
      {
        attributeOpened = true;
        hsize_t size = H5Aget_storage_size(attribId);
        attributeOutput.resize(static_cast<size_t>(size)); // Resize the vector to the proper length
        if(attrTypeId >= 0)
        {
          readError = H5Aread(attribId, attrTypeId, attributeOutput.data());
          if(readError >= 0)
          {
            if(size > 0 && attributeOutput[size - 1] == 0) // null Terminated string
            {
              size -= 1;
            }
            data.append(attributeOutput.data(), size); // Append the data to the passed in string
          }
        }
      }
      H5Aclose(attribId);
      H5Tclose(attrTypeId);
    }
  }

  if(isVariableString)
  {
    return MakeErrorResult<std::string>(-440, fmt::format("Cannot read attribute '{}'. Invalid string type.", attributeName));
  }
  if(attributeOpened && readError < 0)
  {
    std::cout << "Error Reading Attribute." << std::endl;
    return MakeErrorResult<std::string>(-450, fmt::format("Error reading attribute: '{}'", attributeName));
  }
  returnResult = {data};
  return returnResult;
}

Result<> ObjectIO::writeStringAttribute(const std::string& attributeName, const std::string& text)
{
  Result<> returnError = {};
  size_t size = text.size();

  // Resolve self-locking deletion and ID access before the attribute leaf section.
  deleteAttribute(attributeName);
  const hid_t selfId = getId();

  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  hid_t attributeType = H5Tcopy(H5T_C_S1);
  H5Tset_size(attributeType, size);
  H5Tset_strpad(attributeType, H5T_STR_NULLTERM);
  hid_t attributeSpaceID = H5Screate(H5S_SCALAR);
  hid_t attributeId = H5Acreate(selfId, attributeName.c_str(), attributeType, attributeSpaceID, H5P_DEFAULT, H5P_DEFAULT);
  if(attributeId < 0)
  {
    returnError = MakeErrorResult(attributeId, "Error Creating String Attribute");
  }

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
  return nullptr;
}

bool ObjectIO::hasAttribute(const std::string& attributeName) const
{
  // getId() self-locks; resolve it before the leaf-locked bare H5Aexists.
  const hid_t selfId = getId();
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  return H5Aexists(selfId, attributeName.c_str()) > 0;
}

usize ObjectIO::getNumElementsInAttribute(hid_t attribId) const
{
  // getTypeFromId() self-locks, so retain the local HDF5 handles across the
  // lock boundary and call it without holding the non-recursive ApiLock.
  std::vector<hsize_t> dims;
  hid_t attrType = -1;
  hid_t dataspaceId = -1;
  size_t typeSize = 0;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    attrType = H5Aget_type(attribId);
    typeSize = H5Tget_size(attrType);
    dataspaceId = H5Aget_space(attribId);
  }
  const Type type = getTypeFromId(attrType);
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    if(dataspaceId >= 0)
    {
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
          H5Sclose(dataspaceId);
          H5Tclose(attrType);
          return 0;
        }
        // Copy the dimensions into the dims vector
        dims.clear(); // Erase everything in the Vector
        dims.resize(rank);
        std::copy(hdims.cbegin(), hdims.cend(), dims.begin());
      }
    }
    H5Sclose(dataspaceId);
    H5Tclose(attrType);
  }

  hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());
  return numElements;
}
} // namespace nx::core::HDF5
