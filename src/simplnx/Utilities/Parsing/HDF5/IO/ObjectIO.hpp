#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <H5Apublic.h>
#include <H5Epublic.h>
#include <H5Ipublic.h>
#include <H5Opublic.h>
#include <H5Ppublic.h>
#include <H5Spublic.h>
#include <H5Tpublic.h>

#include <fmt/format.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/**
 * @def HDF_ERROR_HANDLER_OFF
 * @brief Saves and disables the current HDF5 error callback in the active scope.
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
 * @namespace nx::core::HDF5
 * @brief Provides process-locked HDF5 object wrappers.
 */
namespace nx::core::HDF5
{
class FileIO;
class GroupIO;

/**
 * @namespace Support
 * @brief Provides shared HDF5 synchronization support.
 */
namespace Support
{
/**
 * @brief Returns the process-wide nonrecursive HDF5 API mutex.
 * @return Shared mutex.
 *
 * This forward declaration avoids an H5Support include cycle.
 */
SIMPLNX_EXPORT std::mutex& ApiLock();
} // namespace Support

/**
 * @class ObjectIO
 * @brief Base wrapper for one HDF5 file, group, or dataset object.
 *
 * Public HDF5 operations use Support::ApiLock(). A public wrapper method must
 * not run while the caller holds that nonrecursive lock.
 */
class SIMPLNX_EXPORT ObjectIO
{
public:
  /**
   * @enum ObjectType
   * @brief Identifies the wrapped HDF5 object category.
   */
  enum class ObjectType
  {
    File,    ///< Identifies a file wrapper.
    Group,   ///< Identifies a group wrapper.
    Dataset, ///< Identifies a dataset wrapper.
    Unknown  ///< Identifies no known wrapper category.
  };

  /**
   * @typedef DimsType
   * @brief Defines an HDF5 dimension vector.
   */
  using DimsType = std::vector<usize>;

  /**
   * @brief Constructs an invalid ObjectIO.
   */
  ObjectIO();

  ObjectIO(const ObjectIO& other) = delete;
  /**
   * @brief Moves wrapped object ownership.
   * @param other Provides object state.
   */
  ObjectIO(ObjectIO&& other) noexcept;

  ObjectIO& operator=(const ObjectIO& other) = delete;
  /**
   * @brief Replaces this wrapper with moved object ownership.
   * @param other Provides object state.
   * @return This wrapper.
   */
  ObjectIO& operator=(ObjectIO&& other) noexcept;

  /**
   * @brief Releases the wrapped HDF5 object.
   */
  virtual ~ObjectIO() noexcept;

  /**
   * @brief Tests whether this wrapper has a valid target.
   * @return True for a valid target.
   */
  virtual bool isValid() const;

  /**
   * @brief Returns the HDF5 object name.
   * @return Name, or empty string for an invalid object.
   */
  virtual std::string getName() const;

  /**
   * @brief Returns the HDF5 path from the parent ID.
   * @return Path, or empty string for an invalid object.
   */
  virtual std::string getNamePath() const;

  /**
   * @brief Returns the stored HDF5 object path.
   * @return Object path.
   */
  virtual std::string getObjectPath() const;

  /**
   * @brief Returns the wrapper category.
   * @return Object type.
   */
  ObjectType getObjectType() const;

  /**
   * @brief Returns the parent object name.
   * @return Name, or empty string for an invalid object.
   */
  std::string getParentName() const;

  /**
   * @brief Returns object attribute count.
   * @return Attribute count, or 0 for an invalid object.
   */
  usize getNumAttributes() const;

  /**
   * @brief Returns all attribute names.
   * @return Names, or empty vector for an invalid object.
   */
  std::vector<std::string> getAttributeNames() const;

  /**
   * @brief Returns one attribute name by index.
   * @param id Specifies the zero-based attribute index.
   * @return Name, or empty string for an invalid index.
   */
  std::string getAttributeNameByIndex(int64 id) const;

  /**
   * @brief Deletes one attribute.
   * @param name Specifies the attribute.
   */
  void deleteAttribute(const std::string& name);

  /**
   * @brief Tests whether one attribute exists.
   * @param name Specifies the attribute.
   * @return True if the attribute exists.
   */
  bool hasAttribute(const std::string& name) const;

  /**
   * @brief Deletes all object attributes.
   */
  void deleteAttributes();

  /**
   * @brief Reads one string attribute.
   * @param attributeName Specifies the attribute.
   * @return String value or HDF5 read error.
   */
  Result<std::string> readStringAttribute(const std::string& attributeName) const;

  /**
   * @brief Reads one scalar attribute.
   * @tparam T Specifies the scalar type.
   * @param attributeName Specifies the attribute.
   * @return Scalar value, shape error, or HDF5 read error.
   */
  template <typename T>
  Result<T> readScalarAttribute(const std::string& attributeName) const
  {
    if(getId() <= 0)
    {
      return MakeErrorResult<T>(-970, fmt::format("Cannot read attribute '{}'. Object '{}' is invalid", attributeName, getName()));
    }

    auto vectorResult = readVectorAttribute<T>(attributeName);
    if(vectorResult.invalid())
    {
      return ConvertInvalidResult<T>(std::move(vectorResult));
    }
    auto vector = std::move(vectorResult.value());
    if(vector.size() != 1)
    {
      std::string ss = fmt::format("Attribute values of unexpected size. One value expected. {} values read", std::to_string(vector.size()));
      std::cout << ss << std::endl;
      return MakeErrorResult<T>(-972, ss);
    }

    return {vector[0]};
  }

  /**
   * @brief Reads one vector attribute.
   * @tparam T Specifies the scalar type.
   * @param attributeName Specifies the attribute.
   * @return Values or HDF5 read error.
   */
  template <typename T>
  Result<std::vector<T>> readVectorAttribute(const std::string& attributeName) const
  {
    // getId() self-locks; resolve it before the leaf locks below. HDF5 is not thread-safe, so
    // every bare H5A* call must run under Support::ApiLock().
    const hid_t objectId = getId();
    if(objectId <= 0)
    {
      return MakeErrorResult<std::vector<T>>(-1, fmt::format("Cannot Read Attribute '{}' within Invalid Object '{}'", attributeName, getName()));
    }

    hid_t attribId = H5I_INVALID_HID;
    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      HDF_ERROR_HANDLER_OFF
      attribId = H5Aopen(objectId, attributeName.c_str(), H5P_DEFAULT);
      HDF_ERROR_HANDLER_ON
    }
    if(attribId < 0)
    {
      return MakeErrorResult<std::vector<T>>(attribId, fmt::format("Error Opening Attribute '{}' within '{}'", attributeName, getName()));
    }

    // getNumElementsInAttribute() self-locks, so it runs between the leaf locks (holding the open
    // attribId across it needs no lock — only the H5 calls do).
    std::vector<T> values(getNumElementsInAttribute(attribId));

    herr_t error = 0;
    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      const hid_t typeId = H5Aget_type(attribId);
      error = H5Aread(attribId, typeId, values.data());
      H5Aclose(attribId);
      H5Tclose(typeId);
    }
    if(error != 0)
    {
      std::string ss = fmt::format("Error Reading Vector Attribute '{}'.", attributeName);
      return MakeErrorResult<std::vector<T>>(error, ss);
    }

    return {values};
  }

  /**
   * @brief Creates or replaces one string attribute.
   * @param attributeName Specifies the attribute.
   * @param value Specifies the string value.
   * @return HDF5 create/write error, or success.
   */
  Result<> writeStringAttribute(const std::string& attributeName, const std::string& value);

  /**
   * @brief Creates or replaces one scalar attribute.
   * @tparam T Specifies the scalar type.
   * @param attributeName Specifies the attribute.
   * @param value Specifies the scalar value.
   * @return HDF5 create/write/close error, or success.
   */
  template <typename T>
  Result<> writeScalarAttribute(const std::string& attributeName, const T& value)
  {
    herr_t error = 0;
    Result<> returnError = {};

    hid_t dataType = HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      return MakeErrorResult(-101, "Cannot write specified data type");
    }

    /* Create the data space for the attribute. */
    int32_t rank = 1;
    hsize_t dims = 1;
    hid_t dataspaceId = H5I_INVALID_HID;
    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      dataspaceId = H5Screate_simple(rank, &dims, nullptr);
    }
    if(dataspaceId >= 0)
    {
      // Delete existing attribute
      deleteAttribute(attributeName);
      const hid_t selfId = getId();
      {
        std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
        /* Create the attribute. */
        hid_t attributeId = H5Acreate(selfId, attributeName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
        if(attributeId >= 0)
        {
          /* Write the attribute data. */
          error = H5Awrite(attributeId, dataType, &value);
          if(error < 0)
          {
            returnError = MakeErrorResult(error, "Error Writing Attribute");
          }
        }
        /* Close the attribute. */
        error = H5Aclose(attributeId);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Closing Attribute");
        }
        /* Close the dataspace. */
        error = H5Sclose(dataspaceId);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Closing Dataspace");
        }
      }
    }
    else
    {
      returnError = MakeErrorResult(dataspaceId, "Invalid Dataspace ID");
    }

    return returnError;
  }

  /**
   * @brief Creates or replaces one vector attribute.
   * @tparam T Specifies the scalar type.
   * @param attributeName Specifies the attribute.
   * @param value Specifies vector values.
   * @return HDF5 create/write/close error, or success.
   */
  template <typename T>
  Result<> writeVectorAttribute(const std::string& attributeName, const std::vector<T>& value)
  {
    Result<> returnError = {};
    herr_t error = 0;

    std::vector<usize> dims = {value.size()};
    int32_t rank = static_cast<int32_t>(dims.size());

    hid_t dataType = HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      return MakeErrorResult(-101, "Unknown data type");
    }
    std::vector<hsize_t> hDims(dims.size());
    std::transform(dims.begin(), dims.end(), hDims.begin(), [](usize x) { return static_cast<hsize_t>(x); });
    hid_t dataspaceId = H5I_INVALID_HID;
    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);
    }
    if(dataspaceId >= 0)
    {
      // Delete any existing attribute
      deleteAttribute(attributeName);
      const hid_t selfId = getId();
      {
        std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
        /* Create the attribute. */
        hid_t attributeId = H5Acreate(selfId, attributeName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
        if(attributeId >= 0)
        {
          /* Write the attribute data. */
          error = H5Awrite(attributeId, dataType, static_cast<const void*>(value.data()));
          if(error < 0)
          {
            returnError = MakeErrorResult(error, "Error Writing Attribute");
          }
        }
        /* Close the attribute. */
        error = H5Aclose(attributeId);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Closing Attribute");
        }
        /* Close the dataspace. */
        error = H5Sclose(dataspaceId);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Closing Dataspace");
        }
      }
    }
    else
    {
      returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace ID");
    }

    return returnError;
  }

  /**
   * @brief Returns the HDF5 file path.
   * @return File path, or empty path when unavailable.
   */
  std::filesystem::path getFilePath() const;

  /**
   * @brief Returns the parent FileIO.
   * @return Parent pointer, or null when this object is a file.
   */
  virtual FileIO* parentFile() const;

  /**
   * @brief Returns a pointer to the parent GroupIO. Returns null if there is no known parent.
   * @return GroupIO*
   */
  // GroupIO* parentGroup() const;

#if 0
  /**
   * @brief Returns the HighFive::File for the current IO handler.
   * Returns an empty optional if the file could not be determined.
   *
   * This method should only be called by simplnx HDF5 IO wrapper classes.
   * @return std::optional<HighFive::File>
   */
  virtual std::optional<HighFive::File> h5File() const;
#endif

  /**
   * @brief Returns or opens the HDF5 object ID.
   * @return Object ID, or a negative HDF5 error value.
   * @note Use only from HDF5 wrapper classes.
   */
  hid_t getId() const;

  hid_t getParentId() const;

protected:
  /**
   * @brief Maps a C++ scalar type to its native HDF5 type.
   * @tparam T Specifies the scalar type.
   * @return Native HDF5 type ID.
   * @throws std::runtime_error If T is unsupported.
   */
  template <typename T>
  static inline hid_t HdfTypeForPrimitive()
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
   * @brief Creates a wrapper from file and object paths.
   * @param filepath Specifies the HDF5 file.
   * @param objectPath Specifies the object path.
   */
  ObjectIO(const std::filesystem::path& filepath, const std::string& objectPath);
  /**
   * @brief Creates a wrapper from a parent ID and child name.
   * @param parentId Specifies the parent HDF5 object.
   * @param objectName Specifies the child object name.
   */
  ObjectIO(hid_t parentId, const std::string& objectName);

  /**
   * @brief Sets the target HDF5 file path.
   * @param filepath Specifies the file.
   */
  void setFilePath(const std::filesystem::path& filepath);

  /**
   * @brief Sets the HDF5 object name.
   * @param name Specifies the name.
   */
  void setName(const std::string& name);

  /**
   * @brief Moves another wrapper's state into this object.
   * @param rhs Provides object state.
   */
  void moveObj(ObjectIO&& rhs) noexcept;

  /**
   * @brief Sets the wrapped HDF5 object ID.
   * @param id Specifies the object ID.
   */
  void setId(hid_t id) const;

  /**
   * @brief Sets the parent HDF5 object ID.
   * @param parentId Specifies the parent ID.
   */
  void setParentId(hid_t parentId);

  /**
   * @brief Reads a string attribute by HDF5 attribute ID.
   * @param id Specifies the attribute ID.
   * @return String value, or empty string after a read failure.
   */
  std::string readStringAttribute(int64 id) const;

  /**
   * @brief Creates or replaces a string attribute on one HDF5 object.
   * @param objectId Specifies the object ID.
   * @param attributeName Specifies the attribute.
   * @param str Specifies the value.
   * @return HDF5 create/write error, or success.
   */
  Result<> writeStringAttribute(int64 objectId, const std::string& attributeName, const std::string& str);

  /**
   * @brief Tests whether the target HDF5 object is open.
   * @return True for an open object.
   */
  bool isOpen() const;
  /**
   * @brief Opens the target HDF5 object.
   * @return Object ID, or a negative HDF5 error value.
   */
  virtual hid_t open() const = 0;
  /**
   * @brief Closes the wrapped HDF5 object.
   */
  virtual void close() = 0;

  /**
   * @brief Returns the element count for one open attribute.
   * @param attribId Specifies the attribute ID.
   * @return Element count, or 0 after a query failure.
   */
  usize getNumElementsInAttribute(hid_t attribId) const;

private:
  std::filesystem::path m_FilePath;
  std::string m_ObjectName;
  mutable hid_t m_Id = -1;
  hid_t m_ParentId = 0;
};
} // namespace nx::core::HDF5
