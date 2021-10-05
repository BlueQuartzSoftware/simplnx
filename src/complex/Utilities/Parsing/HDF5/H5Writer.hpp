#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"
#include "complex/complex_export.hpp"

#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace complex
{
namespace H5
{
namespace Writer
{
namespace Generic
{
/**
 * @brief
 * @tparam T
 * @param locationID
 * @param objectName
 * @param attributeName
 * @param data
 * @return
 */
template <typename T>
H5::ErrorType writeScalarAttribute(H5::IdType locationID, const std::string& objectName, const std::string& attributeName, T data)
{
  hid_t objectID, dataspaceID, attributeID;
  H5O_info_t objectInfo;
  herr_t returnError = 0;
  hsize_t dims = 1;
  int32 rank = 1;
  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  /* Get the type of object */
  herr_t error = H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    std::cout << "Error getting object info at locationID (" << locationID << ") with object name (" << objectName << ")" << std::endl;
    return error;
  }
  /* Open the object */
  objectID = Support::OpenId(locationID, objectName, objectInfo.type);
  if(objectID < 0)
  {
    std::cout << "Error opening Object for Attribute operations." << std::endl;
    return static_cast<herr_t>(objectID);
  }

  /* Create the data space for the attribute. */
  dataspaceID = H5Screate_simple(rank, &dims, nullptr);
  if(dataspaceID >= 0)
  {
    /* Verify if the attribute already exists */
    herr_t hasAttribute = Support::FindAttribute(objectID, attributeName);

    /* The attribute already exists, delete it */
    if(hasAttribute == 1)
    {
      error = H5Adelete(objectID, attributeName.c_str());
      if(error < 0)
      {
        std::cout << "Error Deleting Existing Attribute" << std::endl;
        returnError = error;
      }
    }

    if(error >= 0)
    {
      /* Create the attribute. */
      attributeID = H5Acreate(objectID, attributeName.c_str(), dataType, dataspaceID, H5P_DEFAULT, H5P_DEFAULT);
      if(attributeID >= 0)
      {
        /* Write the attribute data. */
        error = H5Awrite(attributeID, dataType, &data);
        if(error < 0)
        {
          std::cout << "Error Writing Attribute" << std::endl;
          returnError = error;
        }
      }
      /* Close the attribute. */
      error = H5Aclose(attributeID);
      if(error < 0)
      {
        std::cout << "Error Closing Attribute" << std::endl;
        returnError = error;
      }
    }
    /* Close the dataspace. */
    error = H5Sclose(dataspaceID);
    if(error < 0)
    {
      std::cout << "Error Closing Dataspace" << std::endl;
      returnError = error;
    }
  }
  else
  {
    returnError = static_cast<herr_t>(dataspaceID);
  }

  /* Close the object */
  error = Support::CloseId(objectID, objectInfo.type);
  if(error < 0)
  {
    std::cout << "Error Closing HDF5 Object ID" << std::endl;
    returnError = error;
  }
  return returnError;
}

/**
 * @brief
 * @param locationID
 * @param objectName
 * @param attributeName
 * @param data
 * @return
 */
H5::ErrorType COMPLEX_EXPORT writeStringAttribute(H5::IdType locationID, const std::string& objectName, const std::string& attributeName, const std::string& data);

/**
 * @brief
 * @tparam T
 * @param locationID
 * @param objectName
 * @param attributeName
 * @param rank
 * @param dims
 * @param data
 * @return
 */
template <typename T>
H5::ErrorType writePointerAttribute(H5::IdType locationID, const std::string& objectName, const std::string& attributeName, int32 rank, const H5::SizeType* dims, const T* data)
{
  hid_t objectID, dataspaceID, attributeID;
  herr_t hasAttribute;
  H5O_info_t objectInfo;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    std::cout << "dataType was unknown" << std::endl;
    return -1;
  }
  /* Get the type of object */

  if(H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT) < 0)
  {
    std::cout << "Error getting object info at locationID (" << locationID << ") with object name (" << objectName << ")" << std::endl;
    return -1;
  }
  /* Open the object */
  objectID = Support::OpenId(locationID, objectName, objectInfo.type);
  if(objectID < 0)
  {
    std::cout << "Error opening Object for Attribute operations." << std::endl;
    return -1;
  }

  dataspaceID = H5Screate_simple(rank, dims, nullptr);
  if(dataspaceID >= 0)
  {
    /* Verify if the attribute already exists */
    hasAttribute = Support::FindAttribute(objectID, attributeName);

    /* The attribute already exists, delete it */
    if(hasAttribute == 1)
    {
      error = H5Adelete(objectID, attributeName.c_str());
      if(error < 0)
      {
        std::cout << "Error Deleting Existing Attribute" << std::endl;
        returnError = error;
      }
    }

    if(error >= 0)
    {
      /* Create the attribute. */
      attributeID = H5Acreate(objectID, attributeName.c_str(), dataType, dataspaceID, H5P_DEFAULT, H5P_DEFAULT);
      if(attributeID >= 0)
      {
        /* Write the attribute data. */
        error = H5Awrite(attributeID, dataType, data);
        if(error < 0)
        {
          std::cout << "Error Writing Attribute" << std::endl;
          returnError = error;
        }
      }
      /* Close the attribute. */
      error = H5Aclose(attributeID);
      if(error < 0)
      {
        std::cout << "Error Closing Attribute" << std::endl;
        returnError = error;
      }
    }
    /* Close the dataspace. */
    error = H5Sclose(dataspaceID);
    if(error < 0)
    {
      std::cout << "Error Closing Dataspace" << std::endl;
      returnError = error;
    }
  }
  else
  {
    returnError = static_cast<herr_t>(dataspaceID);
  }
  /* Close the object */
  error = Support::CloseId(objectID, objectInfo.type);
  if(error < 0)
  {
    std::cout << "Error Closing HDF5 Object ID" << std::endl;
    returnError = error;
  }
  return returnError;
}
} // namespace Generic
} // namespace Writer
} // namespace H5
} // namespace complex
