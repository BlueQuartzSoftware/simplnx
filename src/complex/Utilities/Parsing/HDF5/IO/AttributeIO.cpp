#include "AttributeIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include <H5Apublic.h>

#include <iostream>
#include <numeric>
#include <vector>

namespace complex::HDF5
{
AttributeIO::AttributeIO() = default;

AttributeIO::AttributeIO(IdType objectId, size_t attrIdx)
: m_ObjectId(objectId)
{
  m_AttributeId = H5Aopen_idx(objectId, attrIdx);
}

AttributeIO::AttributeIO(IdType objectId, const std::string& attrName)
: m_ObjectId(objectId)
{
  m_AttributeId = H5Aopen(objectId, attrName.c_str(), H5P_DEFAULT);
}

AttributeIO::~AttributeIO()
{
  closeHdf5();
}

void AttributeIO::closeHdf5()
{
  if(isValid())
  {
    H5Aclose(m_AttributeId);
    m_AttributeId = 0;
  }
}

ErrorType AttributeIO::findAndDeleteAttribute()
{
  hsize_t attributeNum = 0;
  int32_t hasAttribute = H5Aiterate(getObjectId(), H5_INDEX_NAME, H5_ITER_INC, &attributeNum, Support::FindAttr, const_cast<char*>(getName().c_str()));

  /* The attribute already exists, delete it */
  if(hasAttribute == 1)
  {
    herr_t error = H5Adelete(getObjectId(), getName().c_str());
    if(error < 0)
    {
      std::cout << "Error Deleting Attribute '" << getName() << "' from Object '" << getObjectName() << "'" << std::endl;
      return error;
    }
  }
  return 0;
}

bool AttributeIO::isValid() const
{
  return getAttributeId() > 0;
}

IdType AttributeIO::getObjectId() const
{
  return m_ObjectId;
}

IdType AttributeIO::getAttributeId() const
{
  return m_AttributeId;
}

IdType AttributeIO::getDataspaceId() const
{
  return H5Aget_space(getAttributeId());
}

std::string AttributeIO::getName() const
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

std::string AttributeIO::getObjectName() const
{
  if(!isValid())
  {
    return "";
  }

  if(!isValid())
  {
    return "";
  }

  std::string path = GetNameFromId(getObjectId());

  return path;
}

Type AttributeIO::getType() const
{
  return getTypeFromId(getTypeId());
}

IdType AttributeIO::getClassType() const
{
  auto typeId = getTypeId();
  return H5Tget_class(typeId);
}

IdType AttributeIO::getTypeId() const
{
  return H5Aget_type(getAttributeId());
}

size_t AttributeIO::getNumElements() const
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
T AttributeIO::readAsValue() const
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

std::string AttributeIO::readAsString() const
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

herr_t AttributeIO::writeString(const std::string& text)
{
  if(!isValid())
  {
    return -1;
  }

  herr_t returnError = 0;
  size_t size = text.size();

  /* Get the type of object */
  // H5O_info_t objectInfo{};
  // returnError = H5Oget_info_by_name(getObjectId(),
  // getAttributeName().c_str(), &objectInfo, H5P_DEFAULT);
  if(returnError >= 0)
  {
    /* Create the attribute */
    hid_t attributeType = H5Tcopy(H5T_C_S1);
    if(attributeType >= 0)
    {
      size_t attributeSize = size; /* extra null term */
      herr_t error = H5Tset_size(attributeType, attributeSize);
      if(error < 0)
      {
        std::cout << "Error Setting H5T Size" << std::endl;
        returnError = error;
      }
      if(error >= 0)
      {
        error = H5Tset_strpad(attributeType, H5T_STR_NULLTERM);
        if(error < 0)
        {
          std::cout << "Error adding a null terminator." << std::endl;
          returnError = error;
        }
        if(error >= 0)
        {
          hid_t attributeSpaceID = H5Screate(H5S_SCALAR);
          if(attributeSpaceID >= 0)
          {
            /* Verify if the attribute already exists */
            returnError = findAndDeleteAttribute();
            if(error >= 0)
            {
              /* Create and write the attribute */
              hid_t attributeId = H5Acreate(getObjectId(), getName().c_str(), attributeType, attributeSpaceID, H5P_DEFAULT, H5P_DEFAULT);
              if(attributeId >= 0)
              {
                error = H5Awrite(attributeId, attributeType, text.c_str());
                if(error < 0)
                {
                  std::cout << "Error Writing String attribute." << std::endl;
                  returnError = error;
                }
              }
              H5S_CLOSE_H5_ATTRIBUTE(attributeId, error, returnError)
            }
            H5S_CLOSE_H5_DATASPACE(attributeSpaceID, error, returnError)
          }
        }
      }
      H5S_CLOSE_H5_TYPE(attributeType, error, returnError)
    }
  }

  return returnError;
}

template <typename T>
herr_t AttributeIO::writeValue(T value)
{
  if(!isValid())
  {
    return -1;
  }

  herr_t error = 0;
  herr_t returnError = 0;

  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  /* Get the type of object */
  // H5O_info_t objectInfo;
  // error = H5Oget_info_by_name(getObjectId(), getObjectName().c_str(),
  // &objectInfo, H5P_DEFAULT); if(error < 0)
  //{
  //  std::cout << "Error getting object info at locationId (" << getObjectId()
  //  << ") with object name (" << getObjectName() << ")" << std::endl; return
  //  error;
  //}

  /* Create the data space for the attribute. */
  int32_t rank = 1;
  hsize_t dims = 1;
  hid_t dataspaceId = H5Screate_simple(rank, &dims, nullptr);
  if(dataspaceId >= 0)
  {
    // Delete existing attribute
    error = findAndDeleteAttribute();
    if(error >= 0)
    {
      /* Create the attribute. */
      hid_t attributeId = H5Acreate(getObjectId(), getName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
      if(attributeId >= 0)
      {
        /* Write the attribute data. */
        error = H5Awrite(attributeId, dataType, &value);
        if(error < 0)
        {
          std::cout << "Error Writing Attribute" << std::endl;
          returnError = error;
        }
      }
      /* Close the attribute. */
      error = H5Aclose(attributeId);
      if(error < 0)
      {
        std::cout << "Error Closing Attribute" << std::endl;
        returnError = error;
      }
    }
    /* Close the dataspace. */
    error = H5Sclose(dataspaceId);
    if(error < 0)
    {
      std::cout << "Error Closing Dataspace" << std::endl;
      returnError = error;
    }
  }
  else
  {
    returnError = static_cast<herr_t>(dataspaceId);
  }

  return returnError;
}

template <typename T>
std::vector<T> AttributeIO::readAsVector() const
{
  if(!isValid())
  {
    return {};
  }

  const size_t count = getNumElements();
  T* values = new T[getNumElements()];
  IdType typeId = getTypeId();

  ErrorType error = H5Aread(getAttributeId(), typeId, &values[0]);
  if(error != 0)
  {
    std::cout << "Error Reading Attribute." << error << std::endl;
    return {};
  }

  std::vector<T> vector(count);
  for(size_t i = 0; i < count; i++)
  {
    vector[i] = values[i];
  }
  delete[] values;
  return vector;
}

template <typename T>
ErrorType AttributeIO::writeVector(const DimsVector& dims, const std::vector<T>& vector)
{
  if(!isValid())
  {
    return -1;
  }

  herr_t returnError = 0;
  int32_t rank = static_cast<int32_t>(dims.size());

  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    std::cout << "dataType was unknown" << std::endl;
    return -1;
  }

  hid_t dataspaceId = H5Screate_simple(rank, dims.data(), nullptr);
  if(dataspaceId >= 0)
  {
    // Delete any existing attribute
    herr_t error = findAndDeleteAttribute();

    if(error >= 0)
    {
      /* Create the attribute. */
      hid_t attributeId = H5Acreate(getObjectId(), getName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
      if(attributeId >= 0)
      {
        /* Write the attribute data. */
        error = H5Awrite(attributeId, dataType, static_cast<const void*>(vector.data()));
        if(error < 0)
        {
          std::cout << "Error Writing Attribute" << std::endl;
          returnError = error;
        }
      }
      /* Close the attribute. */
      error = H5Aclose(attributeId);
      if(error < 0)
      {
        std::cout << "Error Closing Attribute" << std::endl;
        returnError = error;
      }
    }
    /* Close the dataspace. */
    error = H5Sclose(dataspaceId);
    if(error < 0)
    {
      std::cout << "Error Closing Dataspace" << std::endl;
      returnError = error;
    }
  }
  else
  {
    returnError = static_cast<herr_t>(dataspaceId);
  }

  return returnError;
}

// declare readAsValue
template int8_t AttributeIO::readAsValue<int8_t>() const;
template int16_t AttributeIO::readAsValue<int16_t>() const;
template int32_t AttributeIO::readAsValue<int32_t>() const;
template int64_t AttributeIO::readAsValue<int64_t>() const;
template uint8_t AttributeIO::readAsValue<uint8_t>() const;
template uint16_t AttributeIO::readAsValue<uint16_t>() const;
template uint32_t AttributeIO::readAsValue<uint32_t>() const;
template uint64_t AttributeIO::readAsValue<uint64_t>() const;
template float AttributeIO::readAsValue<float>() const;
template double AttributeIO::readAsValue<double>() const;
template bool AttributeIO::readAsValue<bool>() const;

// declare writeValue
template ErrorType AttributeIO::writeValue<int8_t>(int8_t value);
template ErrorType AttributeIO::writeValue<int16_t>(int16_t value);
template ErrorType AttributeIO::writeValue<int32_t>(int32_t value);
template ErrorType AttributeIO::writeValue<int64_t>(int64_t value);
template ErrorType AttributeIO::writeValue<uint8_t>(uint8_t value);
template ErrorType AttributeIO::writeValue<uint16_t>(uint16_t value);
template ErrorType AttributeIO::writeValue<uint32_t>(uint32_t value);
template ErrorType AttributeIO::writeValue<uint64_t>(uint64_t value);
template ErrorType AttributeIO::writeValue<float>(float value);
template ErrorType AttributeIO::writeValue<double>(double value);
template ErrorType AttributeIO::writeValue<bool>(bool value);

// declare writeVector
template ErrorType AttributeIO::writeVector<int8_t>(const DimsVector& dims, const std::vector<int8_t>& vector);
template ErrorType AttributeIO::writeVector<int16_t>(const DimsVector& dims, const std::vector<int16_t>& vector);
template ErrorType AttributeIO::writeVector<int32_t>(const DimsVector& dims, const std::vector<int32_t>& vector);
template ErrorType AttributeIO::writeVector<int64_t>(const DimsVector& dims, const std::vector<int64_t>& vector);
template ErrorType AttributeIO::writeVector<uint8_t>(const DimsVector& dims, const std::vector<uint8_t>& vector);
template ErrorType AttributeIO::writeVector<uint16_t>(const DimsVector& dims, const std::vector<uint16_t>& vector);
template ErrorType AttributeIO::writeVector<uint32_t>(const DimsVector& dims, const std::vector<uint32_t>& vector);
template ErrorType AttributeIO::writeVector<uint64_t>(const DimsVector& dims, const std::vector<uint64_t>& vector);
template ErrorType AttributeIO::writeVector<float>(const DimsVector& dims, const std::vector<float>& vector);
template ErrorType AttributeIO::writeVector<double>(const DimsVector& dims, const std::vector<double>& vector);

template std::vector<int8_t> AttributeIO::readAsVector<int8_t>() const;
template std::vector<int16_t> AttributeIO::readAsVector<int16_t>() const;
template std::vector<int32_t> AttributeIO::readAsVector<int32_t>() const;
template std::vector<int64_t> AttributeIO::readAsVector<int64_t>() const;
template std::vector<uint8_t> AttributeIO::readAsVector<uint8_t>() const;
template std::vector<uint16_t> AttributeIO::readAsVector<uint16_t>() const;
template std::vector<uint32_t> AttributeIO::readAsVector<uint32_t>() const;
template std::vector<uint64_t> AttributeIO::readAsVector<uint64_t>() const;
template std::vector<float> AttributeIO::readAsVector<float>() const;
template std::vector<double> AttributeIO::readAsVector<double>() const;
template std::vector<bool> AttributeIO::readAsVector<bool>() const;
} // namespace complex::HDF5
