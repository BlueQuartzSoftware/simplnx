#include "AttributeWriter.hpp"

#include <iostream>

#include <H5Apublic.h>

#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

namespace complex::HDF5
{
AttributeWriter::AttributeWriter()
{
}

AttributeWriter::AttributeWriter(IdType objectId, const std::string& attributeName)
: m_ObjectId(objectId)
, m_AttributeName(attributeName)
{
}

AttributeWriter::~AttributeWriter() = default;

ErrorType AttributeWriter::findAndDeleteAttribute()
{
  hsize_t attributeNum = 0;
  int32_t hasAttribute = H5Aiterate(getObjectId(), H5_INDEX_NAME, H5_ITER_INC, &attributeNum, Support::FindAttr, const_cast<char*>(getAttributeName().c_str()));

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

bool AttributeWriter::isValid() const
{
  return (getObjectId() > 0) && (m_AttributeName.empty() != true);
}

IdType AttributeWriter::getObjectId() const
{
  return m_ObjectId;
}

std::string AttributeWriter::getObjectName() const
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

std::string AttributeWriter::getAttributeName() const
{
  if(!isValid())
  {
    return "";
  }

  return m_AttributeName;
}

herr_t AttributeWriter::writeString(const std::string& text)
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

// declare writeValue
template ErrorType AttributeWriter::writeValue<int8_t>(int8_t value);
template ErrorType AttributeWriter::writeValue<int16_t>(int16_t value);
template ErrorType AttributeWriter::writeValue<int32_t>(int32_t value);
template ErrorType AttributeWriter::writeValue<int64_t>(int64_t value);
template ErrorType AttributeWriter::writeValue<uint8_t>(uint8_t value);
template ErrorType AttributeWriter::writeValue<uint16_t>(uint16_t value);
template ErrorType AttributeWriter::writeValue<uint32_t>(uint32_t value);
template ErrorType AttributeWriter::writeValue<uint64_t>(uint64_t value);
template ErrorType AttributeWriter::writeValue<float>(float value);
template ErrorType AttributeWriter::writeValue<double>(double value);

// declare writeVector
template ErrorType AttributeWriter::writeVector<int8_t>(const DimsVector& dims, const std::vector<int8_t>& vector);
template ErrorType AttributeWriter::writeVector<int16_t>(const DimsVector& dims, const std::vector<int16_t>& vector);
template ErrorType AttributeWriter::writeVector<int32_t>(const DimsVector& dims, const std::vector<int32_t>& vector);
template ErrorType AttributeWriter::writeVector<int64_t>(const DimsVector& dims, const std::vector<int64_t>& vector);
template ErrorType AttributeWriter::writeVector<uint8_t>(const DimsVector& dims, const std::vector<uint8_t>& vector);
template ErrorType AttributeWriter::writeVector<uint16_t>(const DimsVector& dims, const std::vector<uint16_t>& vector);
template ErrorType AttributeWriter::writeVector<uint32_t>(const DimsVector& dims, const std::vector<uint32_t>& vector);
template ErrorType AttributeWriter::writeVector<uint64_t>(const DimsVector& dims, const std::vector<uint64_t>& vector);
template ErrorType AttributeWriter::writeVector<float>(const DimsVector& dims, const std::vector<float>& vector);
template ErrorType AttributeWriter::writeVector<double>(const DimsVector& dims, const std::vector<double>& vector);

} // namespace complex::HDF5
