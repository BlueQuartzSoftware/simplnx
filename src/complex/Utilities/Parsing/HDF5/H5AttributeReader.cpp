#include "H5AttributeReader.hpp"

#include <iostream>
#include <numeric>
#include <vector>

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::AttributeReader::AttributeReader() = default;

H5::AttributeReader::AttributeReader(H5::IdType objectId, size_t attrIdx)
: m_ObjectId(objectId)
{
  m_AttributeId = H5Aopen_idx(objectId, attrIdx);
}

H5::AttributeReader::AttributeReader(H5::IdType objectId, const std::string& attrName)
: m_ObjectId(objectId)
{
  m_AttributeId = H5Aopen(objectId, attrName.c_str(), H5P_DEFAULT);
}

H5::AttributeReader::~AttributeReader()
{
  closeHdf5();
}

void H5::AttributeReader::closeHdf5()
{
  if(isValid())
  {
    H5Aclose(m_AttributeId);
    m_AttributeId = 0;
  }
}

bool H5::AttributeReader::isValid() const
{
  return getAttributeId() > 0;
}

H5::IdType H5::AttributeReader::getObjectId() const
{
  return m_ObjectId;
}

H5::IdType H5::AttributeReader::getAttributeId() const
{
  return m_AttributeId;
}

H5::IdType H5::AttributeReader::getDataspaceId() const
{
  return H5Aget_space(getAttributeId());
}

std::string H5::AttributeReader::getName() const
{
  if(!isValid())
  {
    return "";
  }

  const size_t size = 1024;
  char buffer[size];
  H5Aget_name(getAttributeId(), size, buffer);

  return H5::GetNameFromBuffer(buffer);
}

H5::Type H5::AttributeReader::getType() const
{
  return H5::getTypeFromId(getTypeId());
}

H5::IdType H5::AttributeReader::getTypeId() const
{
  return H5Aget_type(getAttributeId());
}

size_t H5::AttributeReader::getNumElements() const
{
  size_t typeSize = H5Tget_size(getTypeId());
  std::vector<hsize_t> dims;
  hid_t dataspaceId = getDataspaceId();
  if(dataspaceId >= 0)
  {
    if(getType() == H5::Type::string)
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
T H5::AttributeReader::readAsValue() const
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

std::string H5::AttributeReader::readAsString() const
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
        data.append(attributeOutput.data(), size); // Append the data to the passed in string
      }
      // H5Tclose(attributeType);
    }
  }

  return data;
}

// declare readAsValue
template int8_t H5::AttributeReader::readAsValue<int8_t>() const;
template int16_t H5::AttributeReader::readAsValue<int16_t>() const;
template int32_t H5::AttributeReader::readAsValue<int32_t>() const;
template int64_t H5::AttributeReader::readAsValue<int64_t>() const;
template uint8_t H5::AttributeReader::readAsValue<uint8_t>() const;
template uint16_t H5::AttributeReader::readAsValue<uint16_t>() const;
template uint32_t H5::AttributeReader::readAsValue<uint32_t>() const;
template uint64_t H5::AttributeReader::readAsValue<uint64_t>() const;
template float H5::AttributeReader::readAsValue<float>() const;
template double H5::AttributeReader::readAsValue<double>() const;
