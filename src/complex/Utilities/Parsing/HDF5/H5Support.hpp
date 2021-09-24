#pragma once

#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/complex_export.hpp"

#define HDF_ERROR_HANDLER_OFF                                                                                                                                                                          \
  herr_t (*_oldHDF_error_func)(hid_t, void*);                                                                                                                                                          \
  void* _oldHDF_error_client_data;                                                                                                                                                                     \
  H5Eget_auto(H5E_DEFAULT, &_oldHDF_error_func, &_oldHDF_error_client_data);                                                                                                                           \
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

#define HDF_ERROR_HANDLER_ON H5Eset_auto(H5E_DEFAULT, _oldHDF_error_func, _oldHDF_error_client_data);

#define CloseH5A(attributeId, error, returnError)                                                                                                                                                      \
  error = H5Aclose(attributeId);                                                                                                                                                                       \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): "                                                                                                                                      \
              << "Error Closing Attribute." << std::endl;                                                                                                                                              \
    returnError = error;                                                                                                                                                                               \
  }

#define CloseH5S(dataspaceId, error, returnError)                                                                                                                                                      \
  error = H5Sclose(dataspaceId);                                                                                                                                                                       \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): "                                                                                                                                      \
              << "Error closing Dataspace." << std::endl;                                                                                                                                              \
    returnError = error;                                                                                                                                                                               \
  }

#define CloseH5T(typeId, error, returnError)                                                                                                                                                           \
  error = H5Tclose(typeId);                                                                                                                                                                            \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): "                                                                                                                                      \
              << "Error closing DataType" << std::endl;                                                                                                                                                \
    returnError = error;                                                                                                                                                                               \
  }

#define CloseH5D(datasetId, error, returnError, datasetName)                                                                                                                                           \
  error = H5Dclose(datasetId);                                                                                                                                                                         \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): "                                                                                                                                      \
              << "Error Closing Dataset: " << datasetName << " datasetId=" << datasetId << " retError=" << returnError << std::endl;                                                                   \
    returnError = error;                                                                                                                                                                               \
  }

namespace complex
{
namespace H5
{
namespace Support
{
/**
 * @brief Opens an object for H5 operations
 * @param locationId The parent object that holds the true object we want to open
 * @param objectName The string name of the object
 * @param objectType The HDF5_TYPE of object
 * @return Standard H5 Error Conditions
 */
hid_t COMPLEX_EXPORT OpenId(hid_t locationId, const std::string& objectName, H5O_type_t objectType);

/**
 * @brief Opens an H5 Object
 * @param objectId The Object id
 * @param objectType Basic Object Type
 * @return Standard H5 Error Conditions
 */
herr_t COMPLEX_EXPORT CloseId(hid_t objectId, int32_t objectType);

/**
 * @brief Finds a Data set given a data set name
 * @param locationId The location to search
 * @param datasetName The dataset to search for
 * @return Standard H5 Error condition. Negative=DataSet
 */
bool COMPLEX_EXPORT DatasetExists(hid_t locationId, const std::string& datasetName);

/**
 * @brief Returns if a given hdf5 object is a group
 * @param objectId The hdf5 object that contains an object with name objectName
 * @param objectName The name of the object to check
 * @return True if the given hdf5 object id is a group
 */
inline bool IsGroup(hid_t nodeId, const std::string& objectName)
{
  H5SUPPORT_MUTEX_LOCK()

  bool isGroup = true;
  herr_t error = -1;
  H5O_info_t objectInfo{};
  error = H5Oget_info_by_name(nodeId, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    std::cout << "Error in methd H5Gget_objinfo" << std::endl;
    return false;
  }
  switch(objectInfo.type)
  {
  case H5O_TYPE_GROUP:
    isGroup = true;
    break;
  case H5O_TYPE_DATASET:
    isGroup = false;
    break;
  case H5O_TYPE_NAMED_DATATYPE:
    isGroup = false;
    break;
  default:
    isGroup = false;
  }
  return isGroup;
}

/**
 * @brief Get the information about a dataset.
 *
 * @param locationId The parent location of the Dataset
 * @param datasetName The name of the dataset
 * @param dims A std::vector that will hold the sizes of the dimensions
 * @param typeClass The H5 class type
 * @param typeSize THe H5 getSize of the data
 * @return Negative value is Failure. Zero or Positive is success;
 */
herr_t COMPLEX_EXPORT getDatasetInfo(hid_t locationId, const std::string& datasetName, std::vector<hsize_t>& dims, H5T_class_t& classType, size_t& sizeType);

herr_t COMPLEX_EXPORT find_attr(hid_t /*locationId*/, const char* name, const H5A_info_t* /*info*/, void* opData);

/**
 * @brief Inquires if an attribute named attributeName exists attached to the object locationId.
 * @param locationId The location to search
 * @param attributeName The attribute to search for
 * @return Standard H5 Error condition
 */
herr_t COMPLEX_EXPORT findAttribute(hid_t locationId, const std::string& attributeName);

/**
 * @brief Returns the HDF Type for a given primitive value.
 * @return The H5 native type for the value
 */
template <typename T>
inline hid_t COMPLEX_EXPORT HDFTypeForPrimitive()
{
  if constexpr(std::is_same_v<T, float>)
  {
    return H5T_NATIVE_FLOAT;
  }
  else if constexpr(std::is_same_v<T, double>)
  {
    return H5T_NATIVE_DOUBLE;
  }
  else if constexpr(std::is_same_v<T, int8_t>)
  {
    return H5T_NATIVE_INT8;
  }
  else if constexpr(std::is_same_v<T, uint8_t>)
  {
    return H5T_NATIVE_UINT8;
  }
  else if constexpr(std::is_same_v<T, char>)
  {
    if constexpr(std::is_signed_v<char>)
    {
      return H5T_NATIVE_INT8;
    }
    else
    {
      return H5T_NATIVE_UINT8;
    }
  }
  else if constexpr(std::is_same_v<T, int16_t>)
  {
    return H5T_NATIVE_INT16;
  }
  else if constexpr(std::is_same_v<T, uint16_t>)
  {
    return H5T_NATIVE_UINT16;
  }
  else if constexpr(std::is_same_v<T, int32_t>)
  {
    return H5T_NATIVE_INT32;
  }
  else if constexpr(std::is_same_v<T, uint32_t>)
  {
    return H5T_NATIVE_UINT32;
  }
  else if constexpr(std::is_same_v<T, int64_t>)
  {
    return H5T_NATIVE_INT64;
  }
  else if constexpr(std::is_same_v<T, uint64_t>)
  {
    return H5T_NATIVE_UINT64;
  }
  else if constexpr(std::is_same_v<T, bool>)
  {
    return H5T_NATIVE_UINT8;
  }
  else if constexpr(std::is_same_v<T, size_t>)
  {
    return H5T_NATIVE_UINT64;
  }
  else
  {
    throw std::runtime_error("HDFTypeForPrimitive does not support this type");
    return -1;
  }
}

/**
 * @brief Returns the associated string for the given HDF class type.
 * @param classType
 * @return std::string
 */
std::string COMPLEX_EXPORT HDFClassTypeAsStr(hid_t classType);

#if 0
/**
 * @brief Returns the H5T value for a given dataset.
 *
 * Returns the type of data stored in the dataset. You MUST use H5Tclose(typeId)
 * on the returned value or resource leaks will occur.
 * @param locationId A Valid H5 file or group id.
 * @param datasetName Path to the dataset
 * @return
 */
hid_t COMPLEX_EXPORT getDatasetType(hid_t locationId, const std::string& datasetName);
#endif

/**
 * @brief Reads a dataset of multiple strings into a std::vector<std::string>
 * @param locationId
 * @param datasetName
 * @param data
 * @return
 */
herr_t COMPLEX_EXPORT readVectorOfStringDataset(hid_t locationId, const std::string& datasetName, std::vector<std::string>& data);

/**
 * @brief Reads a string dataset into the supplied string. Any data currently in the 'data' variable
 * is cleared first before the new data is read into the string.
 * @param locationId The parent group that holds the data object to read
 * @param datasetName The name of the dataset.
 * @param data The std::string to hold the data
 * @return Standard HDF error condition
 */
herr_t COMPLEX_EXPORT readStringDataset(hid_t locationId, const std::string& datasetName, std::string& data);

/**
 * @brief Returns the path to an object
 * @param objectId The HDF5 id of the object
 * @return  The path to the object relative to the objectId
 */
inline std::string getObjectPath(hid_t locationId)
{
  H5SUPPORT_MUTEX_LOCK()

  size_t nameSize = 1 + H5Iget_name(locationId, nullptr, 0);
  std::vector<char> objectName(nameSize, 0);
  H5Iget_name(locationId, objectName.data(), nameSize);
  std::string objectPath(objectName.data());

  if((objectPath != "/") && (objectPath.at(0) == '/'))
  {
    objectPath.erase(0, 1);
  }

  return objectPath;
}

/**
 * @brief Writes the data of a pointer to an HDF5 file
 * @param locationId The hdf5 object id of the parent
 * @param datasetName The name of the dataset to write to. This can be a name of Path
 * @param rank The number of dimensions
 * @param dims The sizes of each dimension
 * @param data The data to be written.
 * @return Standard hdf5 error condition.
 */
template <typename T>
inline herr_t writePointerDataset(hid_t locationId, const std::string& datasetName, int32_t rank, const hsize_t* dims, const T* data)
{
  H5SUPPORT_MUTEX_LOCK()

  herr_t error = -1;
  hid_t datasetId = -1;
  hid_t dataspaceId = -1;
  herr_t returnError = 0;

  if(nullptr == data)
  {
    return -2;
  }
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  // Create the DataSpace
  dataspaceId = H5Screate_simple(rank, dims, nullptr);
  if(dataspaceId < 0)
  {
    return static_cast<herr_t>(dataspaceId);
  }
  // Create the Dataset
  // This will fail if datasetName contains a "/"!
  datasetId = H5Dcreate(locationId, datasetName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if(datasetId >= 0)
  {
    error = H5Dwrite(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if(error < 0)
    {
      std::cout << "Error Writing Data '" << datasetName << "'" << std::endl;
      std::cout << "    rank = " << rank << std::endl;
      uint64_t totalSize = 1;
      for(size_t i = 0; i < rank; ++i)
      {
        std::cout << "    dim[" << i << "] = " << dims[i] << std::endl;
        totalSize = totalSize * dims[i];
      }
      std::cout << "    Total Elements = " << totalSize << std::endl;
      std::cout << "    Size of Type (Bytes) = " << sizeof(T) << std::endl;
      std::cout << "    Total Bytes to Write =  " << (sizeof(T) * totalSize) << std::endl;
      returnError = error;
    }
    error = H5Dclose(datasetId);
    if(error < 0)
    {
      std::cout << "Error Closing Dataset." << std::endl;
      returnError = error;
    }
  }
  else
  {
    returnError = static_cast<herr_t>(datasetId);
  }
  /* Terminate access to the data space. */
  error = H5Sclose(dataspaceId);
  if(error < 0)
  {
    std::cout << "Error Closing Dataspace" << std::endl;
    returnError = error;
  }
  return returnError;
}

/**
 * @brief Writes an Attribute to an HDF5 Object
 * @param locationId The Parent Location of the HDFobject that is getting the attribute
 * @param objectName The Name of Object to write the attribute into.
 * @param attributeName The Name of the Attribute
 * @param rank The number of dimensions in the attribute data
 * @param dims The Dimensions of the attribute data
 * @param data The Attribute Data to write as a pointer
 * @return Standard HDF Error Condition
 */
template <typename T>
inline herr_t writePointerAttribute(hid_t locationId, const std::string& objectName, const std::string& attributeName, int32_t rank, const hsize_t* dims, const T* data)
{
  H5SUPPORT_MUTEX_LOCK()

  hid_t objectId, dataspaceId, attributeId;
  herr_t hasAttribute;
  H5O_info_t objectInfo;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    std::cout << "dataType was unknown" << std::endl;
    return -1;
  }

  /* Get the type of object */
  if(H5Oget_info_by_name(locationId, objectName.c_str(), &objectInfo, H5P_DEFAULT) < 0)
  {
    std::cout << "Error getting object info at locationId (" << locationId << ") with object name (" << objectName << ")" << std::endl;
    return -1;
  }
  /* Open the object */
  objectId = OpenId(locationId, objectName, objectInfo.type);
  if(objectId < 0)
  {
    std::cout << "Error opening Object for Attribute operations." << std::endl;
    return -1;
  }

  dataspaceId = H5Screate_simple(rank, dims, nullptr);
  if(dataspaceId >= 0)
  {
    /* Verify if the attribute already exists */
    hasAttribute = findAttribute(objectId, attributeName);

    /* The attribute already exists, delete it */
    if(hasAttribute == 1)
    {
      error = H5Adelete(objectId, attributeName.c_str());
      if(error < 0)
      {
        std::cout << "Error Deleting Existing Attribute" << std::endl;
        returnError = error;
      }
    }

    if(error >= 0)
    {
      /* Create the attribute. */
      attributeId = H5Acreate(objectId, attributeName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
      if(attributeId >= 0)
      {
        /* Write the attribute data. */
        error = H5Awrite(attributeId, dataType, data);
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
  /* Close the object */
  error = CloseId(objectId, objectInfo.type);
  if(error < 0)
  {
    std::cout << "Error Closing HDF5 Object ID" << std::endl;
    returnError = error;
  }
  return returnError;
}

/**
 * @brief Writes an Attribute to an HDF5 Object
 * @param locationId The Parent Location of the HDFobject that is getting the attribute
 * @param objectName The Name of Object to write the attribute into.
 * @param attributeName The Name of the Attribute
 * @param dims The Dimensions of the data set
 * @param data The Attribute Data to write
 * @return Standard HDF Error Condition
 */
template <typename T>
inline herr_t writeVectorAttribute(hid_t locationId, const std::string& objectName, const std::string& attributeName, const std::vector<hsize_t>& dims, const std::vector<T>& data)
{
  return writePointerAttribute(locationId, objectName, attributeName, static_cast<int32_t>(dims.size()), dims.data(), data.data());
}

/**
 * @brief Returns the information about an attribute.
 * You must close the typeId argument or resource leaks will occur. Use
 *  H5Tclose(typeId); after your call to this method if you do not need the id for
 *   anything.
 * @param locationId The parent location of the Dataset
 * @param objectName The name of the dataset
 * @param attributeName The name of the attribute
 * @param dims A std::vector that will hold the sizes of the dimensions
 * @param typeClass The HDF5 class type
 * @param typeSize THe HDF5 size of the data
 * @param typeId The Attribute ID - which needs to be closed after you are finished with the data
 * @return
 */
inline herr_t getAttributeInfo(hid_t locationId, const std::string& objectName, const std::string& attributeName, std::vector<hsize_t>& dims, H5T_class_t& typeClass, size_t& typeSize, hid_t& typeId)
{
  H5SUPPORT_MUTEX_LOCK()

  /* identifiers */
  hid_t objectId;
  H5O_info_t objectInfo{};
  hid_t attributeId;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t dataspaceId;
  hid_t rank = -1;

  error = H5Oget_info_by_name(locationId, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return error;
  }

  /* Open the object */
  objectId = OpenId(locationId, objectName, objectInfo.type);
  if(objectId >= 0)
  {
    attributeId = H5Aopen_by_name(locationId, objectName.c_str(), attributeName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    if(attributeId >= 0)
    {
      /* Get an identifier for the datatype. */
      typeId = H5Aget_type(attributeId);
      if(typeId > 0)
      {
        /* Get the class. */
        typeClass = H5Tget_class(typeId);
        /* Get the size. */
        typeSize = H5Tget_size(typeId);
        dataspaceId = H5Aget_space(attributeId);
        if(dataspaceId >= 0)
        {
          if(typeClass == H5T_STRING)
          {
            rank = 1;
            dims.resize(rank);
            dims[0] = typeSize;
          }
          else
          {
            rank = H5Sget_simple_extent_ndims(dataspaceId);
            std::vector<hsize_t> hdims(rank, 0);
            /* Get dimensions */
            error = H5Sget_simple_extent_dims(dataspaceId, hdims.data(), nullptr);
            if(error < 0)
            {
              std::cout << "Error Getting Attribute dims" << std::endl;
              returnError = error;
            }
            // Copy the dimensions into the dims vector
            dims.clear(); // Erase everything in the Vector
            dims.resize(rank);
            std::copy(hdims.cbegin(), hdims.cend(), dims.begin());
          }
          CloseH5S(dataspaceId, error, returnError)
          dataspaceId = 0;
        }
      }
      CloseH5A(attributeId, error, returnError)
      attributeId = 0;
    }
    else
    {
      returnError = -1;
    }
    error = CloseId(objectId, objectInfo.type);
    if(error < 0)
    {
      std::cout << "Error Closing Object ID" << std::endl;
      returnError = error;
    }
  }
  return returnError;
}

/**
 * @brief Reads an Attribute from an HDF5 Object.
 *
 * Use this method if you already know the datatype of the attribute. If you do
 * not know this already then use another form of this method.
 *
 * @param locationId The Parent object that holds the object to which you want to read an attribute
 * @param objectName The name of the object to which the attribute is to be read
 * @param attributeName The name of the Attribute to read
 * @param data The memory to store the data
 * @return Standard HDF Error condition
 */
template <typename T>
inline herr_t readVectorAttribute(hid_t locationId, const std::string& objectName, const std::string& attributeName, std::vector<T>& data)
{
  H5SUPPORT_MUTEX_LOCK()

  /* identifiers */
  hid_t objectId;
  H5O_info_t objectInfo;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t attributeId;
  hid_t typeId;
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  // std::cout << "   Reading Vector Attribute at Path '" << objectName << "' with Key: '" << attributeName << "'" << std::endl;
  /* Get the type of object */
  error = H5Oget_info_by_name(locationId, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return error;
  }
  /* Open the object */
  objectId = OpenId(locationId, objectName, objectInfo.type);
  if(objectId >= 0)
  {
    attributeId = H5Aopen_by_name(locationId, objectName.c_str(), attributeName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    if(attributeId >= 0)
    {
      // Need to allocate the array size
      H5T_class_t typeClass;
      size_t typeSize;
      std::vector<hsize_t> dims;
      error = getAttributeInfo(locationId, objectName, attributeName, dims, typeClass, typeSize, typeId);
      hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<>());
      // std::cout << "    Vector Attribute has " << numElements << " elements." << std::endl;
      data.resize(numElements);
      error = H5Aread(attributeId, dataType, data.data());
      if(error < 0)
      {
        std::cout << "Error Reading Attribute." << error << std::endl;
        returnError = error;
      }
      error = H5Aclose(attributeId);
      if(error < 0)
      {
        std::cout << "Error Closing Attribute" << std::endl;
        returnError = error;
      }
    }
    else
    {
      returnError = static_cast<herr_t>(attributeId);
    }
    error = CloseId(objectId, objectInfo.type);
    if(error < 0)
    {
      std::cout << "Error Closing Object" << std::endl;
      returnError = error;
    }
  }
  return returnError;
}

/**
 * @brief Reads data from the HDF5 File into a preallocated array.
 * @param locationId The parent location that contains the dataset to read
 * @param datasetName The name of the dataset to read
 * @param data A Pointer to the PreAllocated Array of Data
 * @return Standard HDF error condition
 */
template <typename T>
inline herr_t readPointerDataset(hid_t locationId, const std::string& datasetName, T* data)
{
  H5SUPPORT_MUTEX_LOCK()

  hid_t datasetId;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    std::cout << "dataType was not supported." << std::endl;
    return -10;
  }
  if(locationId < 0)
  {
    std::cout << "locationId was Negative: This is not allowed." << std::endl;
    return -2;
  }
  if(nullptr == data)
  {
    std::cout << "The Pointer to hold the data is nullptr. This is NOT allowed." << std::endl;
    return -3;
  }
  datasetId = H5Dopen(locationId, datasetName.c_str(), H5P_DEFAULT);
  if(datasetId < 0)
  {
    std::cout << " Error opening Dataset: " << datasetId << std::endl;
    return -1;
  }
  if(datasetId >= 0)
  {
    error = H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if(error < 0)
    {
      std::cout << "Error Reading Data." << std::endl;
      returnError = error;
    }
    error = H5Dclose(datasetId);
    if(error < 0)
    {
      std::cout << "Error Closing Dataset id" << std::endl;
      returnError = error;
    }
  }
  return returnError;
}

/**
 * @brief Reads data from the HDF5 File into an std::vector<T> object. If the dataset
 * is very large this can be an expensive method to use. It is here for convenience
 * using STL with hdf5.
 * @param locationId The parent location that contains the dataset to read
 * @param datasetName The name of the dataset to read
 * @param data A std::vector<T>. Note the vector WILL be resized to fit the data.
 * The best idea is to just allocate the vector but not to size it. The method
 * will size it for you.
 * @return Standard HDF error condition
 */
template <typename T>
inline herr_t readVectorDataset(hid_t locationId, const std::string& datasetName, std::vector<T>& data)
{
  H5SUPPORT_MUTEX_LOCK()

  hid_t datasetId;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t spaceId;
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  datasetId = H5Dopen(locationId, datasetName.c_str(), H5P_DEFAULT);
  if(datasetId < 0)
  {
    std::cout << "H5Lite.h::readVectorDataset(" << __LINE__ << ") Error opening Dataset at locationId (" << locationId << ") with object name (" << datasetName << ")" << std::endl;
    return -1;
  }
  if(datasetId >= 0)
  {
    spaceId = H5Dget_space(datasetId);
    if(spaceId > 0)
    {
      int32_t rank = H5Sget_simple_extent_ndims(spaceId);
      if(rank > 0)
      {
        std::vector<hsize_t> dims(rank, 0); // Allocate enough room for the dims
        error = H5Sget_simple_extent_dims(spaceId, dims.data(), nullptr);
        hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<>());
        // std::cout << "NumElements: " << numElements << std::endl;
        // Resize the vector
        data.resize(numElements);
        error = H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
        if(error < 0)
        {
          std::cout << "Error Reading Data.'" << datasetName << "'" << std::endl;
          returnError = error;
        }
      }
      error = H5Sclose(spaceId);
      if(error < 0)
      {
        std::cout << "Error Closing Data Space" << std::endl;
        returnError = error;
      }
    }
    else
    {
      std::cout << "Error Opening SpaceID" << std::endl;
      returnError = static_cast<herr_t>(spaceId);
    }
    error = H5Dclose(datasetId);
    if(error < 0)
    {
      std::cout << "Error Closing Dataset" << std::endl;
      returnError = error;
    }
  }
  return returnError;
}

/**
 * @brief Creates a Dataset with the given name at the location defined by locationId.
 * This version of writeDataset should be used with a single scalar value. If you
 * need to write an array of values, use the form that takes an std::vector<>
 *
 * @param locationId The Parent location to store the data
 * @param datasetName The name of the dataset
 * @param value The value to write to the HDF5 dataset
 * @return Standard HDF5 error conditions
 */
template <typename T>
inline herr_t writeScalarDataset(hid_t locationId, const std::string& datasetName, const T& value)
{
  H5SUPPORT_MUTEX_LOCK()

  herr_t error = -1;
  hid_t datasetId = -1;
  hid_t dataspaceId = -1;
  herr_t returnError = 0;
  hsize_t dims = 1;
  hid_t rank = 1;
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  // Create the DataSpace
  dataspaceId = H5Screate_simple(static_cast<int>(rank), &(dims), nullptr);
  if(dataspaceId < 0)
  {
    return static_cast<herr_t>(dataspaceId);
  }
  // Create the Dataset
  datasetId = H5Dcreate(locationId, datasetName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if(datasetId >= 0)
  {
    error = H5Dwrite(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
    if(error < 0)
    {
      std::cout << "Error Writing Data" << std::endl;
      returnError = error;
    }
    error = H5Dclose(datasetId);
    if(error < 0)
    {
      std::cout << "Error Closing Dataset." << std::endl;
      returnError = error;
    }
  }
  else
  {
    returnError = static_cast<herr_t>(datasetId);
  }
  /* Terminate access to the data space. */
  error = H5Sclose(dataspaceId);
  if(error < 0)
  {
    std::cout << "Error Closing Dataspace" << std::endl;
    returnError = error;
  }
  return returnError;
}

/**
 * @brief Writes a std::string as a HDF Dataset.
 * @param locationId The Parent location to write the dataset
 * @param datasetName The Name to use for the dataset
 * @param data The actual data to write as a null terminated string
 * @return Standard HDF5 error conditions
 */
inline herr_t writeStringDataset(hid_t locationId, const std::string& datasetName, const std::string& data)
{
  H5SUPPORT_MUTEX_LOCK()

  hid_t datasetId = -1;
  hid_t dataspaceId = -1;
  hid_t typeId = -1;
  size_t size = 0;
  herr_t error = -1;
  herr_t returnError = 0;

  /* create a string data type */
  if((typeId = H5Tcopy(H5T_C_S1)) >= 0)
  {
    size = data.size() + 1;
    if(H5Tset_size(typeId, size) >= 0)
    {
      if(H5Tset_strpad(typeId, H5T_STR_NULLTERM) >= 0)
      {
        /* Create the data space for the dataset. */
        if((dataspaceId = H5Screate(H5S_SCALAR)) >= 0)
        {
          /* Create or open the dataset. */
          HDF_ERROR_HANDLER_OFF
          datasetId = H5Dopen(locationId, datasetName.c_str(), H5P_DEFAULT);
          HDF_ERROR_HANDLER_ON
          if(datasetId < 0) // dataset does not exist so create it
          {
            datasetId = H5Dcreate(locationId, datasetName.c_str(), typeId, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
          }

          if(datasetId >= 0)
          {
            if(!data.empty())
            {
              error = H5Dwrite(datasetId, typeId, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.c_str());
              if(error < 0)
              {
                std::cout << "Error Writing String Data" << std::endl;
                returnError = error;
              }
            }
          }
          else
          {
            // returnError = datasetId;
            returnError = 0;
          }
          CloseH5D(datasetId, error, returnError, datasetName)
          //          error = H5Dclose(datasetId);
          //          if (error < 0) {
          //            std::cout << "Error Closing Dataset." << std::endl;
          //            returnError = error;
          //          }
        }
        CloseH5S(dataspaceId, error, returnError)
        //        error = H5Sclose(dataspaceId);
        //        if ( error < 0) {
        //          std::cout << "Error closing Dataspace." << std::endl;
        //          returnError = error;
        //        }
      }
    }
    CloseH5T(typeId, error, returnError)
    //    error = H5Tclose(typeId);
    //    if (error < 0 ) {
    //     std::cout << "Error closing DataType" << std::endl;
    //     returnError = error;
    //    }
  }
  return returnError;
}

/**
 * @brief Writes an attribute to the given object. This method is designed with
 * a Template parameter that represents a primitive value. If you need to write
 * an array, please use the other over loaded method that takes a vector.
 * @param locationId The location to look for objectName
 * @param objectName The Object to write the attribute to
 * @param attributeName The  name of the attribute
 * @param data The data to be written as the attribute
 * @return Standard HDF error condition
 */
template <typename T>
inline herr_t writeScalarAttribute(hid_t locationId, const std::string& objectName, const std::string& attributeName, T data)
{
  H5SUPPORT_MUTEX_LOCK()

  hid_t objectId, dataspaceId, attributeId;
  herr_t hasAttribute;
  H5O_info_t objectInfo;
  herr_t error = 0;
  herr_t returnError = 0;
  hsize_t dims = 1;
  int32_t rank = 1;
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  /* Get the type of object */
  error = H5Oget_info_by_name(locationId, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    std::cout << "Error getting object info at locationId (" << locationId << ") with object name (" << objectName << ")" << std::endl;
    return error;
  }
  /* Open the object */
  objectId = OpenId(locationId, objectName, objectInfo.type);
  if(objectId < 0)
  {
    std::cout << "Error opening Object for Attribute operations." << std::endl;
    return static_cast<herr_t>(objectId);
  }

  /* Create the data space for the attribute. */
  dataspaceId = H5Screate_simple(rank, &dims, nullptr);
  if(dataspaceId >= 0)
  {
    /* Verify if the attribute already exists */
    hasAttribute = findAttribute(objectId, attributeName);

    /* The attribute already exists, delete it */
    if(hasAttribute == 1)
    {
      error = H5Adelete(objectId, attributeName.c_str());
      if(error < 0)
      {
        std::cout << "Error Deleting Existing Attribute" << std::endl;
        returnError = error;
      }
    }

    if(error >= 0)
    {
      /* Create the attribute. */
      attributeId = H5Acreate(objectId, attributeName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
      if(attributeId >= 0)
      {
        /* Write the attribute data. */
        error = H5Awrite(attributeId, dataType, &data);
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

  /* Close the object */
  error = CloseId(objectId, objectInfo.type);
  if(error < 0)
  {
    std::cout << "Error Closing HDF5 Object ID" << std::endl;
    returnError = error;
  }
  return returnError;
}

/**
 * @brief Writes a null terminated string as an attribute
 * @param locationId The location to look for objectName
 * @param objectName The Object to write the attribute to
 * @param attributeName The name of the Attribute
 * @param size The number of characters  in the string
 * @param data pointer to a const char array
 * @return Standard HDF error conditions
 */
inline herr_t writeStringAttribute(hid_t locationId, const std::string& objectName, const std::string& attributeName, hsize_t size, const char* data)
{
  H5SUPPORT_MUTEX_LOCK()

  hid_t attributeType;
  hid_t attributeSpaceID;
  hid_t attributeId;
  hid_t objectId;
  int32_t hasAttribute;
  H5O_info_t objectInfo{};
  size_t attributeSize;
  herr_t error = 0;
  herr_t returnError = 0;

  /* Get the type of object */
  returnError = H5Oget_info_by_name(locationId, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(returnError >= 0)
  {
    /* Open the object */
    objectId = OpenId(locationId, objectName, objectInfo.type);
    if(objectId >= 0)
    {
      /* Create the attribute */
      attributeType = H5Tcopy(H5T_C_S1);
      if(attributeType >= 0)
      {
        attributeSize = size; /* extra null term */
        error = H5Tset_size(attributeType, attributeSize);
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
            attributeSpaceID = H5Screate(H5S_SCALAR);
            if(attributeSpaceID >= 0)
            {
              /* Verify if the attribute already exists */
              hasAttribute = findAttribute(objectId, attributeName);
              /* The attribute already exists, delete it */
              if(hasAttribute == 1)
              {
                error = H5Adelete(objectId, attributeName.c_str());
                if(error < 0)
                {
                  std::cout << "Error Deleting Attribute '" << attributeName << "' from Object '" << objectName << "'" << std::endl;
                  returnError = error;
                }
              }
              if(error >= 0)
              {
                /* Create and write the attribute */
                attributeId = H5Acreate(objectId, attributeName.c_str(), attributeType, attributeSpaceID, H5P_DEFAULT, H5P_DEFAULT);
                if(attributeId >= 0)
                {
                  error = H5Awrite(attributeId, attributeType, data);
                  if(error < 0)
                  {
                    std::cout << "Error Writing String attribute." << std::endl;

                    returnError = error;
                  }
                }
                CloseH5A(attributeId, error, returnError)
              }
              CloseH5S(attributeSpaceID, error, returnError)
            }
          }
        }
        CloseH5T(attributeType, error, returnError)
      }
      else
      {
        // returnError = attributeType;
      }
      /* Close the object */
      error = CloseId(objectId, objectInfo.type);
      if(error < 0)
      {
        std::cout << "Error Closing Object Id" << std::endl;
        returnError = error;
      }
    }
  }
  return returnError;
}

/**
 * @brief Writes a string as a null terminated attribute.
 * @param locationId The location to look for objectName
 * @param objectName The Object to write the attribute to
 * @param attributeName The name of the Attribute
 * @param data The string to write as the attribute
 * @return Standard HDF error conditions
 */
inline herr_t writeStringAttribute(hid_t locationId, const std::string& objectName, const std::string& attributeName, const std::string& data)
{
  return writeStringAttribute(locationId, objectName, attributeName, data.size() + 1, data.data());
}

/**
 * @brief Returns the H5T value for a given dataset.
 *
 * Returns the type of data stored in the dataset. You MUST use H5Tclose(typeId)
 * on the returned value or resource leaks will occur.
 * @param locationId A Valid HDF5 file or group id.
 * @param datasetName Path to the dataset
 * @return
 */
inline hid_t getDatasetType(hid_t locationId, const std::string& datasetName)
{
  H5SUPPORT_MUTEX_LOCK()

  herr_t error = 0;
  herr_t returnError = 0;
  hid_t datasetId = -1;
  /* Open the dataset. */
  if((datasetId = H5Dopen(locationId, datasetName.c_str(), H5P_DEFAULT)) < 0)
  {
    return -1;
  }
  /* Get an identifier for the datatype. */
  hid_t typeId = H5Dget_type(datasetId);
  CloseH5D(datasetId, error, returnError, datasetName)
  if(returnError < 0)
  {
    return static_cast<hid_t>(returnError);
  }
  return typeId;
}

} // namespace Support
} // namespace H5
} // namespace complex
