#include "AttributeReader.hpp"

#include <iostream>
#include <numeric>
#include <vector>

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

namespace nx::core::HDF5
{
AttributeReader::AttributeReader() = default;

AttributeReader::AttributeReader(IdType objectId, size_t attrIdx)
: m_ObjectId(objectId)
{
  HDF_ERROR_HANDLER_OFF
  m_AttributeId = H5Aopen_idx(objectId, attrIdx);
  HDF_ERROR_HANDLER_ON
}

AttributeReader::AttributeReader(IdType objectId, const std::string& attrName)
: m_ObjectId(objectId)
{
  HDF_ERROR_HANDLER_OFF
  m_AttributeId = H5Aopen(objectId, attrName.c_str(), H5P_DEFAULT);
  HDF_ERROR_HANDLER_ON
}

AttributeReader::~AttributeReader() noexcept
{
  closeHdf5();
}

void AttributeReader::closeHdf5()
{
  if(isValid())
  {
    H5Aclose(m_AttributeId);
    m_AttributeId = 0;
  }
}

bool AttributeReader::isValid() const
{
  return getAttributeId() > 0;
}

IdType AttributeReader::getObjectId() const
{
  return m_ObjectId;
}

IdType AttributeReader::getAttributeId() const
{
  return m_AttributeId;
}

IdType AttributeReader::getDataspaceId() const
{
  return H5Aget_space(getAttributeId());
}

std::string AttributeReader::getName() const
{
  if(!isValid())
  {
    return "";
  }

  const size_t size = 1024;
  char buffer[size];
  H5Aget_name(getAttributeId(), size, buffer);

  return GetNameFromBuffer(buffer);
}

Type AttributeReader::getType() const
{
  return getTypeFromId(getTypeId());
}

IdType AttributeReader::getClassType() const
{
  auto typeId = getTypeId();
  return H5Tget_class(typeId);
}

IdType AttributeReader::getTypeId() const
{
  return H5Aget_type(getAttributeId());
}

size_t AttributeReader::getNumElements() const
{
  size_t typeSize = H5Tget_size(getTypeId());
  std::vector<hsize_t> dims;
  hid_t dataspaceId = getDataspaceId();
  if(dataspaceId >= 0)
  {
    if(getType() == Type::string)
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

template <typename T>
T AttributeReader::readAsValue() const
{
  if(!isValid())
  {
    return 0;
  }

  if(getNumElements() != 1)
  {
    return 0;
  }

  auto vector = readAsVector<T>();
  if(vector.size() != 1)
  {
    std::cout << "Attribute values of unexpected size. One value expected. " << std::to_string(vector.size()) << " values read." << std::endl;
    return 0;
  }

  return vector[0];
}

std::string AttributeReader::readAsString() const
{
  if(!isValid())
  {
    return "";
  }

  std::string data;
  std::vector<char> attributeOutput;

  hid_t attrTypeId = getTypeId();
  htri_t isVariableString = H5Tis_variable_str(attrTypeId); // Test if the string is variable length
  // H5Tclose(attrTypeId);
  if(isVariableString == 1)
  {
    data.clear();
    return data;
  }
  if(getAttributeId() >= 0)
  {
    hsize_t size = H5Aget_storage_size(getAttributeId());
    attributeOutput.resize(static_cast<size_t>(size)); // Resize the vector to the proper length
    hid_t attributeType = getTypeId();
    if(attributeType >= 0)
    {
      herr_t error = H5Aread(getAttributeId(), attributeType, attributeOutput.data());
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
      // H5Tclose(attributeType);
    }
  }

  return data;
}

// declare readAsValue
template SIMPLNX_EXPORT int8_t AttributeReader::readAsValue<int8_t>() const;
template SIMPLNX_EXPORT int16_t AttributeReader::readAsValue<int16_t>() const;
template SIMPLNX_EXPORT int32_t AttributeReader::readAsValue<int32_t>() const;
template SIMPLNX_EXPORT int64_t AttributeReader::readAsValue<int64_t>() const;
template SIMPLNX_EXPORT uint8_t AttributeReader::readAsValue<uint8_t>() const;
template SIMPLNX_EXPORT uint16_t AttributeReader::readAsValue<uint16_t>() const;
template SIMPLNX_EXPORT uint32_t AttributeReader::readAsValue<uint32_t>() const;
template SIMPLNX_EXPORT uint64_t AttributeReader::readAsValue<uint64_t>() const;
template SIMPLNX_EXPORT float AttributeReader::readAsValue<float>() const;
template SIMPLNX_EXPORT double AttributeReader::readAsValue<double>() const;
} // namespace nx::core::HDF5
