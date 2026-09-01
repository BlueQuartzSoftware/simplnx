#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/simplnx_export.hpp"

#include <H5Ipublic.h>
#include <H5Ppublic.h>
#include <hdf5.h>

#include <iostream>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @def H5SUPPORT_MUTEX_LOCK
 * @brief Locks the process HDF5 mutex for the current scope when mutex support is enabled.
 */
#ifdef H5Support_USE_MUTEX
#define H5SUPPORT_MUTEX_LOCK() std::lock_guard<std::mutex> h5ApiLock(nx::core::HDF5::Support::ApiLock());
#else
#define H5SUPPORT_MUTEX_LOCK()
#endif

// Defined in CMake
// #define H5_USE_110_API

/**
 * @def HDF_ERROR_HANDLER_OFF
 * @brief Saves and disables the HDF5 error callback in the current scope.
 *
 * HDF_ERROR_HANDLER_ON must run in the same scope to restore the saved callback.
 */
#define HDF_ERROR_HANDLER_OFF                                                                                                                                                                          \
  herr_t (*_oldHDF_error_func)(hid_t, void*);                                                                                                                                                          \
  void* _oldHDF_error_client_data;                                                                                                                                                                     \
  H5Eget_auto(H5E_DEFAULT, &_oldHDF_error_func, &_oldHDF_error_client_data);                                                                                                                           \
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

/**
 * @def HDF_ERROR_HANDLER_ON
 * @brief Restores the HDF5 error callback saved by HDF_ERROR_HANDLER_OFF.
 */
#define HDF_ERROR_HANDLER_ON H5Eset_auto(H5E_DEFAULT, _oldHDF_error_func, _oldHDF_error_client_data);

/**
 * @def H5S_CLOSE_H5_ATTRIBUTE
 * @brief Closes an attribute and records a close failure.
 * @param attributeId HDF5 attribute identifier to close.
 * @param error Receives the HDF5 close status.
 * @param returnError Receives an error Result when close fails.
 */
#define H5S_CLOSE_H5_ATTRIBUTE(attributeId, error, returnError)                                                                                                                                        \
  error = H5Aclose(attributeId);                                                                                                                                                                       \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): " << "Error Closing Attribute." << std::endl;                                                                                          \
    returnError = MakeErrorResult(error, "Error Closing Attribute");                                                                                                                                   \
  }

/**
 * @def H5S_CLOSE_H5_DATASPACE
 * @brief Closes a data space and records a close failure.
 * @param dataspaceId HDF5 data-space identifier to close.
 * @param error Receives the HDF5 close status.
 * @param returnError Receives an error Result when close fails.
 */
#define H5S_CLOSE_H5_DATASPACE(dataspaceId, error, returnError)                                                                                                                                        \
  error = H5Sclose(dataspaceId);                                                                                                                                                                       \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): " << "Error closing Dataspace." << std::endl;                                                                                          \
    returnError = MakeErrorResult(error, "Error Closing Dataspace");                                                                                                                                   \
  }

/**
 * @def H5S_CLOSE_H5_TYPE
 * @brief Closes a data-type identifier and records a close failure.
 * @param typeId HDF5 data-type identifier to close.
 * @param error Receives the HDF5 close status.
 * @param returnError Receives an error Result when close fails.
 */
#define H5S_CLOSE_H5_TYPE(typeId, error, returnError)                                                                                                                                                  \
  error = H5Tclose(typeId);                                                                                                                                                                            \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): " << "Error closing DataType" << std::endl;                                                                                            \
    returnError = MakeErrorResult(error, "Error closing DataType");                                                                                                                                    \
  }

/**
 * @def H5_CLOSE_H5_DATASET
 * @brief Closes a dataset and records a close failure.
 * @param datasetId HDF5 dataset identifier to close.
 * @param error Receives the HDF5 close status.
 * @param returnError Receives the negative HDF5 status when close fails.
 * @param datasetName Dataset name included in the diagnostic.
 */
#define H5_CLOSE_H5_DATASET(datasetId, error, returnError, datasetName)                                                                                                                                \
  error = H5Dclose(datasetId);                                                                                                                                                                         \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): " << "Error Closing Dataset: " << datasetName << " datasetId=" << datasetId << " retError=" << returnError << std::endl;               \
    returnError = error;                                                                                                                                                                               \
  }

/**
 * @namespace nx::core::HDF5
 * @brief Contains HDF5 parsing and I/O utilities.
 */
namespace nx::core::HDF5
{
/**
 * @namespace nx::core::HDF5::Support
 * @brief Contains low-level HDF5 support functions.
 */
namespace Support
{
/**
 * @brief Tests whether a named child is an HDF5 group.
 * @param nodeId HDF5 location that contains objectName.
 * @param objectName Child object name.
 * @return True for a group. Returns false for another type or an HDF5 query error.
 */
bool SIMPLNX_EXPORT IsGroup(hid_t nodeId, const std::string& objectName);

/**
 * @brief Compares one iterated attribute name with a requested name.
 * @param locationId Unused HDF5 iteration location.
 * @param name Iterated attribute name.
 * @param info Unused attribute information.
 * @param opData Points to the requested null-terminated attribute name.
 * @return One for a match or zero to continue iteration.
 */
herr_t SIMPLNX_EXPORT FindAttr(hid_t /*locationId*/, const char* name, const H5A_info_t* /*info*/, void* opData);

/**
 * @brief Searches an HDF5 object for a named attribute.
 * @param locationId HDF5 object to search.
 * @param attributeName Attribute name to find.
 * @return Positive for a match, zero when absent, or a negative HDF5 error.
 */
herr_t SIMPLNX_EXPORT FindAttribute(hid_t locationId, const std::string& attributeName);

/**
 * @brief Returns the predefined HDF5 native type for a C++ primitive.
 * @tparam T Specifies the primitive type.
 * @return Predefined native type identifier that the caller must not close.
 * @throws std::runtime_error If T is not supported.
 */
template <typename T>
inline hid_t HdfTypeForPrimitive()
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
    throw std::runtime_error("HdfTypeForPrimitive does not support this type");
    return -1;
  }
}

/**
 * @brief Returns the symbolic name for an HDF5 type class.
 * @param classType HDF5 type-class value.
 * @return HDF5 class name, or OTHER for an unknown value.
 */
std::string SIMPLNX_EXPORT HdfClassTypeAsStr(hid_t classType);

/**
 * @brief Returns the symbolic HDF5 native type name for a C++ primitive.
 * @tparam T Specifies the primitive type.
 * @return Native type name.
 * @throws std::runtime_error If T is not supported.
 */
template <typename T>
inline std::string HdfTypeForPrimitiveAsStr()
{
  if constexpr(std::is_same_v<T, float>)
  {
    return "H5T_NATIVE_FLOAT";
  }
  else if constexpr(std::is_same_v<T, double>)
  {
    return "H5T_NATIVE_DOUBLE";
  }
  else if constexpr(std::is_same_v<T, int8>)
  {
    return "H5T_NATIVE_INT8";
  }
  else if constexpr(std::is_same_v<T, uint8>)
  {
    return "H5T_NATIVE_UINT8";
  }
  else if constexpr(std::is_same_v<T, char>)
  {
    if constexpr(std::is_signed_v<char>)
    {
      return "H5T_NATIVE_INT8";
    }
    else
    {
      return "H5T_NATIVE_UINT8";
    }
  }
  else if constexpr(std::is_same_v<T, int16>)
  {
    return "H5T_NATIVE_INT16";
  }
  else if constexpr(std::is_same_v<T, uint16>)
  {
    return "H5T_NATIVE_UINT16";
  }
  else if constexpr(std::is_same_v<T, int32>)
  {
    return "H5T_NATIVE_INT32";
  }
  else if constexpr(std::is_same_v<T, uint32>)
  {
    return "H5T_NATIVE_UINT32";
  }
  else if constexpr(std::is_same_v<T, int64>)
  {
    return "H5T_NATIVE_INT64";
  }
  else if constexpr(std::is_same_v<T, uint64>)
  {
    return "H5T_NATIVE_UINT64";
  }
  else if constexpr(std::is_same_v<T, bool>)
  {
    return "H5T_NATIVE_UINT8";
  }
  else if constexpr(std::is_same_v<T, usize>)
  {
    return "H5T_NATIVE_UINT64";
  }
  else
  {
    throw std::runtime_error("HdfTypeForPrimitive does not support this type");
    return "";
  }
}

#if 0
/**
 * @brief Returns the H5T value for a given dataset.
 *
 * Returns the type of data stored in the dataset. You MUST use H5Tclose(typeId)
 * on the returned value or resource leaks will occur.
 * @param locationId A Valid H5 file or group identifier.
 * @param datasetName Path to the dataset
 * @return
 */
hid_t SIMPLNX_EXPORT getDatasetType(hid_t locationId, const std::string& datasetName);
#endif

/**
 * @brief Returns an HDF5 object's path.
 * @param locationId HDF5 object identifier.
 * @return Path without a leading slash, except the root path remains "/".
 */
std::string SIMPLNX_EXPORT GetObjectPath(hid_t locationId);

/**
 * @brief Opens a dataset and returns its data-type identifier.
 *
 * @param locationId Valid HDF5 file or group identifier.
 * @param datasetName Dataset path.
 * @return Owned data-type identifier, or a negative HDF5 error.
 *
 * The caller must close a nonnegative result with H5Tclose().
 */
hid_t SIMPLNX_EXPORT GetDatasetType(hid_t locationId, const std::string& datasetName);

/**
 * @brief Returns the symbolic native name for an HDF5 data type.
 * @param dataTypeIdentifier HDF5 data-type identifier.
 * @return Native type name, or Unknown when no supported type matches.
 */
std::string SIMPLNX_EXPORT StringForHDFType(hid_t dataTypeIdentifier);

/**
 * @brief Returns the symbolic name for an HDF5 filter.
 * @param id HDF5 filter identifier.
 * @return Filter name, NONE for no filter or an error value, or UNKNOWN for another identifier.
 */
std::string SIMPLNX_EXPORT GetNameFromFilterType(H5Z_filter_t id);

/**
 * @brief Returns the process-wide HDF5 API mutex.
 * @return Immortal non-recursive mutex shared by all simplnx HDF5 call sites.
 *
 * The configured HDF5 library is not thread-safe. Threads serialize HDF5 C API
 * calls with this single lock. A second lock would not protect the same library state.
 *
 * H5SUPPORT_MUTEX_LOCK() uses this mutex when mutex support is enabled. Direct
 * HDF5 call sites must explicitly acquire the same mutex.
 *
 * The mutex is non-recursive. Hold it only around leaf HDF5 calls. Do not call a
 * helper that acquires it or perform compression while the lock is held.
 *
 * Heap allocation intentionally keeps the mutex alive during static destruction,
 * when an HDF5 handle can still close.
 */
SIMPLNX_EXPORT std::mutex& ApiLock();

} // namespace Support
} // namespace nx::core::HDF5
