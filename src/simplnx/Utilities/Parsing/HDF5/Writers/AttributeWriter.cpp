#include "AttributeWriter.hpp"

#include <iostream>

#include <H5Apublic.h>

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "fmt/format.h"

namespace nx::core::HDF5
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

nx::core::Result<> AttributeWriter::findAndDeleteAttribute()
{
  hsize_t attributeNum = 0;
  int32_t hasAttribute = H5Aiterate(getObjectId(), H5_INDEX_NAME, H5_ITER_INC, &attributeNum, Support::FindAttr, const_cast<char*>(getAttributeName().c_str()));

  /* The attribute already exists, delete it */
  if(hasAttribute == 1)
  {
    herr_t error = H5Adelete(getObjectId(), getAttributeName().c_str());
    if(error < 0)
    {
      std::string ss = fmt::format("Error Deleting Attribute '{}' from Object '{}'", getAttributeName(), getObjectName());
      return MakeErrorResult(error, ss);
    }
  }
  return {};
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

Result<> AttributeWriter::writeString(const std::string& text)
{
  if(!isValid())
  {
    return MakeErrorResult(-1, "Cannot write to invalid AttributeWriter");
  }

  Result<> returnError = {};
  size_t size = text.size();

  /* Get the type of object */
  // H5O_info_t objectInfo{};
  // returnError = H5Oget_info_by_name(getObjectId(),
  // getAttributeName().c_str(), &objectInfo, H5P_DEFAULT);
  if(returnError.valid())
  {
    /* Create the attribute */
    hid_t attributeType = H5Tcopy(H5T_C_S1);
    if(attributeType >= 0)
    {
      size_t attributeSize = size; /* extra null term */
      herr_t error = H5Tset_size(attributeType, attributeSize);
      if(error < 0)
      {
        returnError = MakeErrorResult(error, "Error Setting H5T Size");
      }
      if(error >= 0)
      {
        error = H5Tset_strpad(attributeType, H5T_STR_NULLTERM);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error adding a null terminator");
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
                  returnError = MakeErrorResult(error, "Error Writing String Attribute");
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
template Result<> AttributeWriter::writeValue<int8_t>(int8_t value);
template Result<> AttributeWriter::writeValue<int16_t>(int16_t value);
template Result<> AttributeWriter::writeValue<int32_t>(int32_t value);
template Result<> AttributeWriter::writeValue<int64_t>(int64_t value);
template Result<> AttributeWriter::writeValue<uint8_t>(uint8_t value);
template Result<> AttributeWriter::writeValue<uint16_t>(uint16_t value);
template Result<> AttributeWriter::writeValue<uint32_t>(uint32_t value);
template Result<> AttributeWriter::writeValue<uint64_t>(uint64_t value);
template Result<> AttributeWriter::writeValue<float>(float value);
template Result<> AttributeWriter::writeValue<double>(double value);

// declare writeVector
template Result<> AttributeWriter::writeVector<int8_t>(const DimsVector& dims, const std::vector<int8_t>& vector);
template Result<> AttributeWriter::writeVector<int16_t>(const DimsVector& dims, const std::vector<int16_t>& vector);
template Result<> AttributeWriter::writeVector<int32_t>(const DimsVector& dims, const std::vector<int32_t>& vector);
template Result<> AttributeWriter::writeVector<int64_t>(const DimsVector& dims, const std::vector<int64_t>& vector);
template Result<> AttributeWriter::writeVector<uint8_t>(const DimsVector& dims, const std::vector<uint8_t>& vector);
template Result<> AttributeWriter::writeVector<uint16_t>(const DimsVector& dims, const std::vector<uint16_t>& vector);
template Result<> AttributeWriter::writeVector<uint32_t>(const DimsVector& dims, const std::vector<uint32_t>& vector);
template Result<> AttributeWriter::writeVector<uint64_t>(const DimsVector& dims, const std::vector<uint64_t>& vector);
template Result<> AttributeWriter::writeVector<float>(const DimsVector& dims, const std::vector<float>& vector);
template Result<> AttributeWriter::writeVector<double>(const DimsVector& dims, const std::vector<double>& vector);

} // namespace nx::core::HDF5
