#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#define H5_USE_110_API
#include <H5Opublic.h>
#include <hdf5.h>

#include "complex/complex_export.hpp"

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
herr_t COMPLEX_EXPORT closeId(hid_t objectID, int32_t objectType);

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
herr_t COMPLEX_EXPORT getDatasetInfo(hid_t locationID, const std::string& datasetName, std::vector<hsize_t>& dims, H5T_class_t& classType, size_t& sizeType);

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
} // namespace Support
} // namespace H5
} // namespace complex
