#include "ObjectIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5AutoClosers.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

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

  herr_t error = 1;
  H5O_info2_t objectInfo{};

  error = H5Oget_info_by_name3(getParentId(), getName().c_str(), &objectInfo, H5O_INFO_BASIC, H5P_DEFAULT);
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
  H5AttributeCloser attrIdCloser(H5Aopen_idx(getId(), idx));
  const size_t size = 1024;
  char buffer[size];
  H5Aget_name(attrIdCloser.id, size, buffer);
  return GetNameFromBuffer(buffer);
}

void ObjectIO::deleteAttribute(const std::string& name)
{
  if(H5Aexists(getId(), name.c_str()))
  {
    H5Adelete(getId(), name.c_str());
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

  open();
  if(!hasAttribute(attributeName))
  {
    return MakeErrorResult<std::string>(-445, fmt::format("Attribute '{}' does not exist in Object '{}'", attributeName, getName()));
  }

  H5AttributeCloser attrIdCloser(H5Aopen(getId(), attributeName.c_str(), H5P_DEFAULT));
  H5DatatypeCloser attrTypeIdCloser(H5Aget_type(attrIdCloser.id));
  auto isVariableString = H5Tis_variable_str(attrTypeIdCloser.id); // Test if the string is variable length
  if(isVariableString == 1)
  {
    data.clear();
    std::string ss = fmt::format("Cannot read attribute '{}'. Invalid string type.", attributeName);
    return MakeErrorResult<std::string>(-440, ss);
  }
  if(attrIdCloser.valid())
  {
    hsize_t size = H5Aget_storage_size(attrIdCloser.id);
    attributeOutput.resize(static_cast<size_t>(size)); // Resize the vector to the proper length
    if(attrTypeIdCloser.valid())
    {
      herr_t error = H5Aread(attrIdCloser.id, attrTypeIdCloser.id, attributeOutput.data());
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
  return returnResult;
}

Result<> ObjectIO::writeStringAttribute(const std::string& attributeName, const std::string& text)
{
  Result<> returnError = {};
  size_t size = text.size();

  deleteAttribute(attributeName);

  H5DatatypeCloser attributeTypeCloser(H5Tcopy(H5T_C_S1));
  H5Tset_size(attributeTypeCloser.id, size);
  H5Tset_strpad(attributeTypeCloser.id, H5T_STR_NULLTERM);
  H5DataspaceCloser attributeSpaceCloser(H5Screate(H5S_SCALAR));

  H5AttributeCloser attributeIdCloser(H5Acreate(getId(), attributeName.c_str(), attributeTypeCloser.id, attributeSpaceCloser.id, H5P_DEFAULT, H5P_DEFAULT));

  if(attributeIdCloser.invalid())
  {
    returnError = MakeErrorResult(attributeIdCloser.id, "Error Creating String Attribute");
  }

  herr_t error = H5Awrite(attributeIdCloser.id, attributeTypeCloser.id, text.c_str());
  if(error < 0)
  {
    returnError = MakeErrorResult(error, "Error Writing String Attribute");
  }

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
  return H5Aexists(getId(), attributeName.c_str()) > 0;
}

usize ObjectIO::getNumElementsInAttribute(hid_t attribId) const
{
  H5DatatypeCloser attrTypeIdCloser(H5Aget_type(attribId));

  size_t typeSize = H5Tget_size(attrTypeIdCloser.id);
  std::vector<hsize_t> dims;
  H5DataspaceCloser dataspaceIdCloser(H5Aget_space(attribId));
  if(dataspaceIdCloser.valid())
  {
    Type type = getTypeFromId(attrTypeIdCloser.id);
    if(type == Type::string)
    {
      size_t rank = 1;
      dims.resize(rank);
      dims[0] = typeSize;
    }
    else
    {
      size_t rank = H5Sget_simple_extent_ndims(dataspaceIdCloser.id);
      std::vector<hsize_t> hdims(rank, 0);
      /* Get dimensions */
      herr_t error = H5Sget_simple_extent_dims(dataspaceIdCloser.id, hdims.data(), nullptr);
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
