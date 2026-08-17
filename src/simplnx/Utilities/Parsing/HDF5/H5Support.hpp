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



#ifdef H5Support_USE_MUTEX
#define SIMPLNX_H5SUPPORT_MUTEX_LOCK() std::lock_guard<std::mutex> h5ApiLock(nx::core::HDF5::Support::ApiLock());
#else
#define SIMPLNX_H5SUPPORT_MUTEX_LOCK()
#endif

#define HDF_ERROR_HANDLER_OFF                                                                                                                                                                          \
  herr_t (*_oldHDF_error_func)(hid_t, void*);                                                                                                                                                          \
  void* _oldHDF_error_client_data;                                                                                                                                                                     \
  H5Eget_auto(H5E_DEFAULT, &_oldHDF_error_func, &_oldHDF_error_client_data);                                                                                                                           \
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

#define HDF_ERROR_HANDLER_ON H5Eset_auto(H5E_DEFAULT, _oldHDF_error_func, _oldHDF_error_client_data);

#define H5S_CLOSE_H5_ATTRIBUTE(attributeId, error, returnError)                                                                                                                                        \
  error = H5Aclose(attributeId);                                                                                                                                                                       \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): " << "Error Closing Attribute." << std::endl;                                                                                          \
    returnError = MakeErrorResult(error, "Error Closing Attribute");                                                                                                                                   \
  }

#define H5S_CLOSE_H5_DATASPACE(dataspaceId, error, returnError)                                                                                                                                        \
  error = H5Sclose(dataspaceId);                                                                                                                                                                       \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): " << "Error closing Dataspace." << std::endl;                                                                                          \
    returnError = MakeErrorResult(error, "Error Closing Dataspace");                                                                                                                                   \
  }

#define H5S_CLOSE_H5_TYPE(typeId, error, returnError)                                                                                                                                                  \
  error = H5Tclose(typeId);                                                                                                                                                                            \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): " << "Error closing DataType" << std::endl;                                                                                            \
    returnError = MakeErrorResult(error, "Error closing DataType");                                                                                                                                    \
  }

#define H5_CLOSE_H5_DATASET(datasetId, error, returnError, datasetName)                                                                                                                                \
  error = H5Dclose(datasetId);                                                                                                                                                                         \
  if(error < 0)                                                                                                                                                                                        \
  {                                                                                                                                                                                                    \
    std::cout << "File: " << __FILE__ << "(" << __LINE__ << "): " << "Error Closing Dataset: " << datasetName << " datasetId=" << datasetId << " retError=" << returnError << std::endl;               \
    returnError = error;                                                                                                                                                                               \
  }

namespace nx::core::HDF5
{
namespace Support
{
/**
 * @brief Returns if a given hdf5 object is a group
 * @param objectId The hdf5 object that contains an object with name objectName
 * @param objectName The name of the object to check
 * @return True if the given hdf5 object identifier is a group
 */
bool SIMPLNX_EXPORT IsGroup(hid_t nodeId, const std::string& objectName);

/**
 *
 * @param name
 * @param opData
 * @return
 */
herr_t SIMPLNX_EXPORT FindAttr(hid_t /*locationId*/, const char* name, const H5A_info_t* /*info*/, void* opData);

/**
 * @brief Inquires if an attribute named attributeName exists attached to the
 * object locationId.
 * @param locationId The location to search
 * @param attributeName The attribute to search for
 * @return Standard H5 Error condition
 */
herr_t SIMPLNX_EXPORT FindAttribute(hid_t locationId, const std::string& attributeName);

/**
 * @brief Returns the HDF Type for a given primitive value.
 * @return The H5 native type for the value
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
 * @brief Returns the associated string for the given HDF class type.
 * @param classType
 * @return std::string
 */
std::string SIMPLNX_EXPORT HdfClassTypeAsStr(hid_t classType);

/**
 * @brief Returns the HDF Type as a string for a given primitive value.
 * @return The H5 native type as a string for the value
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
 * @brief Returns the path to an object
 * @param objectId The HDF5 identifier of the object
 * @return  The path to the object relative to the objectId
 */
std::string SIMPLNX_EXPORT GetObjectPath(hid_t locationId);

/**
 * @brief Returns the H5T value for a given dataset.
 *
 * Returns the type of data stored in the dataset. You MUST use H5Tclose(typeId)
 * on the returned value or resource leaks will occur.
 * @param locationId A Valid HDF5 file or group identifier.
 * @param datasetName Path to the dataset
 * @return
 */
hid_t SIMPLNX_EXPORT GetDatasetType(hid_t locationId, const std::string& datasetName);

/**
 * @brief Returns a string version of the HDF Type
 * @param type The HDF5 Type to query
 * @return
 */
std::string SIMPLNX_EXPORT StringForHDFType(hid_t dataTypeIdentifier);

/**
 * @brief Returns a std::string of the name of the given filter type.
 * @param id
 * @return std::string
 */
std::string SIMPLNX_EXPORT GetNameFromFilterType(H5Z_filter_t id);

/**
 * @brief Returns the process-wide HDF5 API mutex.
 *
 * The vcpkg HDF5 build is not thread-safe, so every thread that calls into the
 * HDF5 C library must serialize on this single lock. It is a Meyers singleton —
 * one instance for the whole process — so every translation unit linked into
 * libsimplnx shares the exact same mutex. It backs the H5SUPPORT_MUTEX_LOCK()
 * macro; any other code needing the HDF5 lock aliases it rather than defining a
 * second one (two locks guarding one non-thread-safe library would still race).
 *
 * SCOPE — the lock currently guards only the four macro-guarded helpers listed
 * below. The many other direct H5* calls scattered through FileIO, DatasetIO,
 * and Dream3dIO are NOT serialized by it, so this does not by itself make all
 * HDF5 access thread-safe; it makes these four helpers safe and gives the rest a
 * single lock to adopt as they are hardened.
 *
 * CONTRACT — this is a NON-recursive std::mutex. Lock it ONLY around leaf HDF5
 * C-API calls, and NEVER while calling a function that may re-acquire it — in
 * particular the macro-guarded helpers (IsGroup, GetObjectPath, GetDatasetType,
 * StringForHDFType), which lock it themselves. Re-entering on the same thread
 * would self-deadlock. Never hold it around heavy CPU work (e.g. (de)compression)
 * either — that must run off the lock.
 */
SIMPLNX_EXPORT std::mutex& ApiLock();

} // namespace Support
} // namespace nx::core::HDF5
