#include "H5AttributeWriter.hpp"

#include <iostream>

#include <H5Apublic.h>

#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::AttributeWriter::AttributeWriter()
{
}

H5::AttributeWriter::AttributeWriter(H5::IdType objectId, const std::string& attributeName)
: m_ObjectId(objectId)
, m_AttributeName(attributeName)
{
}

H5::AttributeWriter::~AttributeWriter() = default;

H5::ErrorType H5::AttributeWriter::findAndDeleteAttribute()
{
  hsize_t attributeNum = 0;
  int32_t hasAttribute = H5Aiterate(getObjectId(), H5_INDEX_NAME, H5_ITER_INC, &attributeNum, H5::Support::FindAttr, const_cast<char*>(getAttributeName().c_str()));

  /* The attribute already exists, delete it */
  if(hasAttribute == 1)
  {
    herr_t error = H5Adelete(getObjectId(), getAttributeName().c_str());
    if(error < 0)
    {
      std::cout << "Error Deleting Attribute '" << getAttributeName() << "' from Object '" << getObjectName() << "'" << std::endl;
      return error;
    }
  }
  return 0;
}

bool H5::AttributeWriter::isValid() const
{
  return (getObjectId() > 0) && (m_AttributeName.empty() != true);
}

H5::IdType H5::AttributeWriter::getObjectId() const
{
  return m_ObjectId;
}

std::string H5::AttributeWriter::getObjectName() const
{
  if(!isValid())
  {
    return "";
  }

  if(!isValid())
  {
    return "";
  }

  const size_t size = 1024;
  char buffer[size];
  H5Iget_name(getObjectId(), buffer, size);

  return H5::GetNameFromBuffer(buffer);
}

std::string H5::AttributeWriter::getAttributeName() const
{
  if(!isValid())
  {
    return "";
  }

  return m_AttributeName;
}

herr_t H5::AttributeWriter::writeString(const std::string& text)
{
  if(!isValid())
  {
    return -1;
  }

  herr_t returnError = 0;
  size_t size = text.size();

  /* Get the type of object */
  // H5O_info_t objectInfo{};
  // returnError = H5Oget_info_by_name(getObjectId(), getAttributeName().c_str(), &objectInfo, H5P_DEFAULT);
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
              hid_t attributeId = H5Acreate(getObjectId(), getAttributeName().c_str(), attributeType, attributeSpaceID, H5P_DEFAULT, H5P_DEFAULT);
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
herr_t H5::AttributeWriter::writeValue(T value)
{
  if(!isValid())
  {
    return -1;
  }

  herr_t error = 0;
  herr_t returnError = 0;

  hid_t dataType = H5::Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  /* Get the type of object */
  // H5O_info_t objectInfo;
  // error = H5Oget_info_by_name(getObjectId(), getObjectName().c_str(), &objectInfo, H5P_DEFAULT);
  // if(error < 0)
  //{
  //  std::cout << "Error getting object info at locationId (" << getObjectId() << ") with object name (" << getObjectName() << ")" << std::endl;
  //  return error;
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
      hid_t attributeId = H5Acreate(getObjectId(), getAttributeName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
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

#if 0
template <typename T>
herr_t H5::AttributeWriter::writeVector(const DimsVector& dims, const std::vector<T>& vector)
{
  if(!isValid())
  {
    return -1;
  }

  herr_t returnError = 0;
  int32_t rank = static_cast<int32_t>(dims.size());

  hid_t dataType = H5::Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    std::cout << "dataType was unknown" << std::endl;
    return -1;
  }

  /* Get the type of object */
  H5O_info_t objectInfo;
  if(H5Oget_info_by_name(getObjectId(), getObjectName().c_str(), &objectInfo, H5P_DEFAULT) < 0)
  {
    std::cout << "Error getting object info at locationId (" << getObjectId() << ") with object name (" << getObjectName() << ")" << std::endl;
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
      hid_t attributeId = H5Acreate(getObjectId(), getAttributeName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
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
#endif

// declare writeValue
template H5::ErrorType H5::AttributeWriter::writeValue<int8_t>(int8_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<int16_t>(int16_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<int32_t>(int32_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<int64_t>(int64_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<uint8_t>(uint8_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<uint16_t>(uint16_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<uint32_t>(uint32_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<uint64_t>(uint64_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<float>(float value);
template H5::ErrorType H5::AttributeWriter::writeValue<double>(double value);

// declare writeVector
template H5::ErrorType H5::AttributeWriter::writeVector<int8_t>(const DimsVector& dims, const std::vector<int8_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<int16_t>(const DimsVector& dims, const std::vector<int16_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<int32_t>(const DimsVector& dims, const std::vector<int32_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<int64_t>(const DimsVector& dims, const std::vector<int64_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<uint8_t>(const DimsVector& dims, const std::vector<uint8_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<uint16_t>(const DimsVector& dims, const std::vector<uint16_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<uint32_t>(const DimsVector& dims, const std::vector<uint32_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<uint64_t>(const DimsVector& dims, const std::vector<uint64_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<float>(const DimsVector& dims, const std::vector<float>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<double>(const DimsVector& dims, const std::vector<double>& vector);
