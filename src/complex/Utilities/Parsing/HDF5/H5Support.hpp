#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <numeric>

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"

#define HDF_ERROR_HANDLER_OFF                                                                                                                                                                          \
  herr_t (*_oldHDF_error_func)(hid_t, void*);                                                                                                                                                          \
  void* _oldHDF_error_client_data;                                                                                                                                                                     \
  H5Eget_auto(H5E_DEFAULT, &_oldHDF_error_func, &_oldHDF_error_client_data);                                                                                                                           \
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

#define HDF_ERROR_HANDLER_ON H5Eset_auto(H5E_DEFAULT, _oldHDF_error_func, _oldHDF_error_client_data);

#define CloseH5A(attributeID, error, returnError)                                                                                                                                                      \
  error = H5Aclose(attributeID);                                                                                                                                                                       \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): "                                                                                                                                      \
              << "Error Closing Attribute." << std::endl;                                                                                                                                              \
    returnError = error;                                                                                                                                                                               \
  }

#define CloseH5S(dataspaceID, error, returnError)                                                                                                                                                      \
  error = H5Sclose(dataspaceID);                                                                                                                                                                       \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): "                                                                                                                                      \
              << "Error closing Dataspace." << std::endl;                                                                                                                                              \
    returnError = error;                                                                                                                                                                               \
  }

#define CloseH5T(typeID, error, returnError)                                                                                                                                                           \
  error = H5Tclose(typeID);                                                                                                                                                                            \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): "                                                                                                                                      \
              << "Error closing DataType" << std::endl;                                                                                                                                                \
    returnError = error;                                                                                                                                                                               \
  }

#define CloseH5D(datasetID, error, returnError, datasetName)                                                                                                                                           \
  error = H5Dclose(datasetID);                                                                                                                                                                         \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): "                                                                                                                                      \
              << "Error Closing Dataset: " << datasetName << " datasetID=" << datasetID << " retError=" << returnError << std::endl;                                                                   \
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
 * @param locationID The parent object that holds the true object we want to open
 * @param objectName The string name of the object
 * @param objectType The HDF5_TYPE of object
 * @return Standard H5 Error Conditions
 */
hid_t COMPLEX_EXPORT openId(hid_t locationID, const std::string& objectName, H5O_type_t objectType);

/**
 * @brief Opens an H5 Object
 * @param objectID The Object id
 * @param objectType Basic Object Type
 * @return Standard H5 Error Conditions
 */
herr_t COMPLEX_EXPORT closeId(hid_t objectID, int32 objectType);

/**
 * @brief Finds a Data set given a data set name
 * @param locationID The location to search
 * @param datasetName The dataset to search for
 * @return Standard H5 Error condition. Negative=DataSet
 */
bool COMPLEX_EXPORT datasetExists(hid_t locationID, const std::string& datasetName);

/**
 * @brief Get the information about a dataset.
 *
 * @param locationID The parent location of the Dataset
 * @param datasetName The name of the dataset
 * @param dims A std::vector that will hold the sizes of the dimensions
 * @param typeClass The H5 class type
 * @param typeSize THe H5 getSize of the data
 * @return Negative value is Failure. Zero or Positive is success;
 */
herr_t COMPLEX_EXPORT getDatasetInfo(hid_t locationID, const std::string& datasetName, std::vector<hsize_t>& dims, H5T_class_t& classType, usize& sizeType);

herr_t COMPLEX_EXPORT find_attr(hid_t /*locationID*/, const char* name, const H5A_info_t* /*info*/, void* op_data);

/**
 * @brief Inquires if an attribute named attributeName exists attached to the object locationID.
 * @param locationID The location to search
 * @param attributeName The attribute to search for
 * @return Standard H5 Error condition
 */
herr_t COMPLEX_EXPORT findAttribute(hid_t locationID, const std::string& attributeName);

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
  else if constexpr(std::is_same_v<T, int8>)
  {
    return H5T_NATIVE_INT8;
  }
  else if constexpr(std::is_same_v<T, uint8>)
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
  else if constexpr(std::is_same_v<T, int16>)
  {
    return H5T_NATIVE_INT16;
  }
  else if constexpr(std::is_same_v<T, uint16>)
  {
    return H5T_NATIVE_UINT16;
  }
  else if constexpr(std::is_same_v<T, int32>)
  {
    return H5T_NATIVE_INT32;
  }
  else if constexpr(std::is_same_v<T, uint32>)
  {
    return H5T_NATIVE_UINT32;
  }
  else if constexpr(std::is_same_v<T, int64>)
  {
    return H5T_NATIVE_INT64;
  }
  else if constexpr(std::is_same_v<T, uint64>)
  {
    return H5T_NATIVE_UINT64;
  }
  else if constexpr(std::is_same_v<T, bool>)
  {
    return H5T_NATIVE_UINT8;
  }
  else if constexpr(std::is_same_v<T, usize>)
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
 * Returns the type of data stored in the dataset. You MUST use H5Tclose(typeID)
 * on the returned value or resource leaks will occur.
 * @param locationID A Valid H5 file or group id.
 * @param datasetName Path to the dataset
 * @return
 */
hid_t COMPLEX_EXPORT getDatasetType(hid_t locationID, const std::string& datasetName);
#endif

/**
 * @brief Reads a dataset of multiple strings into a std::vector<std::string>
 * @param locationID
 * @param datasetName
 * @param data
 * @return
 */
herr_t COMPLEX_EXPORT readVectorOfStringDataset(hid_t locationID, const std::string& datasetName, std::vector<std::string>& data);

/**
 * @brief Reads a string dataset into the supplied string. Any data currently in the 'data' variable
 * is cleared first before the new data is read into the string.
 * @param locationID The parent group that holds the data object to read
 * @param datasetName The name of the dataset.
 * @param data The std::string to hold the data
 * @return Standard HDF error condition
 */
herr_t COMPLEX_EXPORT readStringDataset(hid_t locationID, const std::string& datasetName, std::string& data);

/**
 * @brief Returns the path to an object
 * @param objectID The HDF5 id of the object
 * @return  The path to the object relative to the objectID
 */
inline std::string getObjectPath(hid_t locationID)
{
  H5SUPPORT_MUTEX_LOCK()

  size_t nameSize = 1 + H5Iget_name(locationID, nullptr, 0);
  std::vector<char> objectName(nameSize, 0);
  H5Iget_name(locationID, objectName.data(), nameSize);
  std::string objectPath(objectName.data());

  if((objectPath != "/") && (objectPath.at(0) == '/'))
  {
    objectPath.erase(0, 1);
  }

  return objectPath;
}


/**
 * @brief Writes the data of a pointer to an HDF5 file
 * @param locationID The hdf5 object id of the parent
 * @param datasetName The name of the dataset to write to. This can be a name of Path
 * @param rank The number of dimensions
 * @param dims The sizes of each dimension
 * @param data The data to be written.
 * @return Standard hdf5 error condition.
 */
template <typename T>
inline herr_t writePointerDataset(hid_t locationID, const std::string& datasetName, int32_t rank, const hsize_t* dims, const T* data)
{
  H5SUPPORT_MUTEX_LOCK()

  herr_t error = -1;
  hid_t datasetID = -1;
  hid_t dataspaceID = -1;
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
  dataspaceID = H5Screate_simple(rank, dims, nullptr);
  if(dataspaceID < 0)
  {
    return static_cast<herr_t>(dataspaceID);
  }
  // Create the Dataset
  // This will fail if datasetName contains a "/"!
  datasetID = H5Dcreate(locationID, datasetName.c_str(), dataType, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if(datasetID >= 0)
  {
    error = H5Dwrite(datasetID, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
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
    error = H5Dclose(datasetID);
    if(error < 0)
    {
      std::cout << "Error Closing Dataset." << std::endl;
      returnError = error;
    }
  }
  else
  {
    returnError = static_cast<herr_t>(datasetID);
  }
  /* Terminate access to the data space. */
  error = H5Sclose(dataspaceID);
  if(error < 0)
  {
    std::cout << "Error Closing Dataspace" << std::endl;
    returnError = error;
  }
  return returnError;
}


/**
 * @brief Writes an Attribute to an HDF5 Object
 * @param locationID The Parent Location of the HDFobject that is getting the attribute
 * @param objectName The Name of Object to write the attribute into.
 * @param attributeName The Name of the Attribute
 * @param rank The number of dimensions in the attribute data
 * @param dims The Dimensions of the attribute data
 * @param data The Attribute Data to write as a pointer
 * @return Standard HDF Error Condition
 */
template <typename T>
inline herr_t writePointerAttribute(hid_t locationID, const std::string& objectName, const std::string& attributeName, int32_t rank, const hsize_t* dims, const T* data)
{
  H5SUPPORT_MUTEX_LOCK()

  hid_t objectID, dataspaceID, attributeID;
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
  if(H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT) < 0)
  {
    std::cout << "Error getting object info at locationID (" << locationID << ") with object name (" << objectName << ")" << std::endl;
    return -1;
  }
  /* Open the object */
  objectID = openId(locationID, objectName, objectInfo.type);
  if(objectID < 0)
  {
    std::cout << "Error opening Object for Attribute operations." << std::endl;
    return -1;
  }

  dataspaceID = H5Screate_simple(rank, dims, nullptr);
  if(dataspaceID >= 0)
  {
    /* Verify if the attribute already exists */
    hasAttribute = findAttribute(objectID, attributeName);

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
  error = closeId(objectID, objectInfo.type);
  if(error < 0)
  {
    std::cout << "Error Closing HDF5 Object ID" << std::endl;
    returnError = error;
  }
  return returnError;
}

/**
 * @brief Writes an Attribute to an HDF5 Object
 * @param locationID The Parent Location of the HDFobject that is getting the attribute
 * @param objectName The Name of Object to write the attribute into.
 * @param attributeName The Name of the Attribute
 * @param dims The Dimensions of the data set
 * @param data The Attribute Data to write
 * @return Standard HDF Error Condition
 */
template <typename T>
inline herr_t writeVectorAttribute(hid_t locationID, const std::string& objectName, const std::string& attributeName, const std::vector<hsize_t>& dims, const std::vector<T>& data)
{
  return writePointerAttribute(locationID, objectName, attributeName, static_cast<int32_t>(dims.size()), dims.data(), data.data());
}


/**
 * @brief Returns the information about an attribute.
 * You must close the typeID argument or resource leaks will occur. Use
 *  H5Tclose(typeID); after your call to this method if you do not need the id for
 *   anything.
 * @param locationID The parent location of the Dataset
 * @param objectName The name of the dataset
 * @param attributeName The name of the attribute
 * @param dims A std::vector that will hold the sizes of the dimensions
 * @param typeClass The HDF5 class type
 * @param typeSize THe HDF5 size of the data
 * @param typeID The Attribute ID - which needs to be closed after you are finished with the data
 * @return
 */
inline herr_t getAttributeInfo(hid_t locationID, const std::string& objectName, const std::string& attributeName, std::vector<hsize_t>& dims, H5T_class_t& typeClass, size_t& typeSize, hid_t& typeID)
{
  H5SUPPORT_MUTEX_LOCK()

  /* identifiers */
  hid_t objectID;
  H5O_info_t objectInfo{};
  hid_t attributeID;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t dataspaceID;
  hid_t rank = -1;

  error = H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return error;
  }

  /* Open the object */
  objectID = openId(locationID, objectName, objectInfo.type);
  if(objectID >= 0)
  {
    attributeID = H5Aopen_by_name(locationID, objectName.c_str(), attributeName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    if(attributeID >= 0)
    {
      /* Get an identifier for the datatype. */
      typeID = H5Aget_type(attributeID);
      if(typeID > 0)
      {
        /* Get the class. */
        typeClass = H5Tget_class(typeID);
        /* Get the size. */
        typeSize = H5Tget_size(typeID);
        dataspaceID = H5Aget_space(attributeID);
        if(dataspaceID >= 0)
        {
          if(typeClass == H5T_STRING)
          {
            rank = 1;
            dims.resize(rank);
            dims[0] = typeSize;
          }
          else
          {
            rank = H5Sget_simple_extent_ndims(dataspaceID);
            std::vector<hsize_t> _dims(rank, 0);
            /* Get dimensions */
            error = H5Sget_simple_extent_dims(dataspaceID, _dims.data(), nullptr);
            if(error < 0)
            {
              std::cout << "Error Getting Attribute dims" << std::endl;
              returnError = error;
            }
            // Copy the dimensions into the dims vector
            dims.clear(); // Erase everything in the Vector
            dims.resize(rank);
            std::copy(_dims.cbegin(), _dims.cend(), dims.begin());
          }
          CloseH5S(dataspaceID, error, returnError);
          dataspaceID = 0;
        }
      }
      CloseH5A(attributeID, error, returnError);
      attributeID = 0;
    }
    else
    {
      returnError = -1;
    }
    error = closeId(objectID, objectInfo.type);
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
 * @param locationID The Parent object that holds the object to which you want to read an attribute
 * @param objectName The name of the object to which the attribute is to be read
 * @param attributeName The name of the Attribute to read
 * @param data The memory to store the data
 * @return Standard HDF Error condition
 */
template <typename T>
inline herr_t readVectorAttribute(hid_t locationID, const std::string& objectName, const std::string& attributeName, std::vector<T>& data)
{
  H5SUPPORT_MUTEX_LOCK()

  /* identifiers */
  hid_t objectID;
  H5O_info_t objectInfo;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t attributeID;
  hid_t typeID;
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  // std::cout << "   Reading Vector Attribute at Path '" << objectName << "' with Key: '" << attributeName << "'" << std::endl;
  /* Get the type of object */
  error = H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return error;
  }
  /* Open the object */
  objectID = openId(locationID, objectName, objectInfo.type);
  if(objectID >= 0)
  {
    attributeID = H5Aopen_by_name(locationID, objectName.c_str(), attributeName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    if(attributeID >= 0)
    {
      // Need to allocate the array size
      H5T_class_t typeClass;
      size_t typeSize;
      std::vector<hsize_t> dims;
      error = getAttributeInfo(locationID, objectName, attributeName, dims, typeClass, typeSize, typeID);
      hsize_t numElements  = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<>());
      // std::cout << "    Vector Attribute has " << numElements << " elements." << std::endl;
      data.resize(numElements);
      error = H5Aread(attributeID, dataType, data.data());
      if(error < 0)
      {
        std::cout << "Error Reading Attribute." << error << std::endl;
        returnError = error;
      }
      error = H5Aclose(attributeID);
      if(error < 0)
      {
        std::cout << "Error Closing Attribute" << std::endl;
        returnError = error;
      }
    }
    else
    {
      returnError = static_cast<herr_t>(attributeID);
    }
    error = closeId(objectID, objectInfo.type);
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
 * @param locationID The parent location that contains the dataset to read
 * @param datasetName The name of the dataset to read
 * @param data A Pointer to the PreAllocated Array of Data
 * @return Standard HDF error condition
 */
template <typename T>
inline herr_t readPointerDataset(hid_t locationID, const std::string& datasetName, T* data)
{
  H5SUPPORT_MUTEX_LOCK()

  hid_t datasetID;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    std::cout << "dataType was not supported." << std::endl;
    return -10;
  }
  if(locationID < 0)
  {
    std::cout << "locationID was Negative: This is not allowed." << std::endl;
    return -2;
  }
  if(nullptr == data)
  {
    std::cout << "The Pointer to hold the data is nullptr. This is NOT allowed." << std::endl;
    return -3;
  }
  datasetID = H5Dopen(locationID, datasetName.c_str(), H5P_DEFAULT);
  if(datasetID < 0)
  {
    std::cout << " Error opening Dataset: " << datasetID << std::endl;
    return -1;
  }
  if(datasetID >= 0)
  {
    error = H5Dread(datasetID, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if(error < 0)
    {
      std::cout << "Error Reading Data." << std::endl;
      returnError = error;
    }
    error = H5Dclose(datasetID);
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
 * @param locationID The parent location that contains the dataset to read
 * @param datasetName The name of the dataset to read
 * @param data A std::vector<T>. Note the vector WILL be resized to fit the data.
 * The best idea is to just allocate the vector but not to size it. The method
 * will size it for you.
 * @return Standard HDF error condition
 */
template <typename T>
inline herr_t readVectorDataset(hid_t locationID, const std::string& datasetName, std::vector<T>& data)
{
  H5SUPPORT_MUTEX_LOCK()

  hid_t datasetID;
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t spaceId;
  hid_t dataType = HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  datasetID = H5Dopen(locationID, datasetName.c_str(), H5P_DEFAULT);
  if(datasetID < 0)
  {
    std::cout << "H5Lite.h::readVectorDataset(" << __LINE__ << ") Error opening Dataset at locationID (" << locationID << ") with object name (" << datasetName << ")" << std::endl;
    return -1;
  }
  if(datasetID >= 0)
  {
    spaceId = H5Dget_space(datasetID);
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
        error = H5Dread(datasetID, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
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
    error = H5Dclose(datasetID);
    if(error < 0)
    {
      std::cout << "Error Closing Dataset" << std::endl;
      returnError = error;
    }
  }
  return returnError;
}

} // namespace Support
} // namespace H5
} // namespace complex
