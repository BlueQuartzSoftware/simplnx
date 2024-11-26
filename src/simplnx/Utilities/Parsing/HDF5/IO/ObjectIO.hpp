#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <H5Opublic.h>
#include <H5Apublic.h>
#include <H5Spublic.h>
#include <H5Epublic.h>
#include <H5Tpublic.h>
#include <H5Ppublic.h>

#include "fmt/format.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define HDF_ERROR_HANDLER_OFF                                                                                                                                                                          \
  herr_t (*_oldHDF_error_func)(hid_t, void*);                                                                                                                                                          \
  void* _oldHDF_error_client_data;                                                                                                                                                                     \
  H5Eget_auto(H5E_DEFAULT, &_oldHDF_error_func, &_oldHDF_error_client_data);                                                                                                                           \
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

#define HDF_ERROR_HANDLER_ON H5Eset_auto(H5E_DEFAULT, _oldHDF_error_func, _oldHDF_error_client_data);

namespace nx::core::HDF5
{
class FileIO;
class GroupIO;

class SIMPLNX_EXPORT ObjectIO
{
public:
  enum class ObjectType
  {
    File,
    Group,
    Dataset,
    Unknown
  };

  using DimsType = std::vector<usize>;

  /**
   * @brief Constructs an invalid ObjectIO.
   */
  ObjectIO();

  ObjectIO(const ObjectIO& other) = delete;
  ObjectIO(ObjectIO&& other) noexcept;

  ObjectIO& operator=(const ObjectIO& other) = delete;
  ObjectIO& operator=(ObjectIO&& other) noexcept;

  /**
   * @brief Releases the wrapped HDF5 object.
   */
  virtual ~ObjectIO() noexcept;

  /**
   * @brief Returns true if the ObjectIO has a valid target. Otherwise,
   * this method returns false.
   * @return bool
   */
  virtual bool isValid() const;

  /**
   * @brief Returns the HDF5 object name. Returns an empty string if the file
   * is invalid.
   * @return std::string
   */
  virtual std::string getName() const;

  /**
   * Returns the HDF5 object path.
   * @return std::string
   */
  virtual std::string getObjectPath() const;

  /**
   * @brief Returns the IO Classes ObjectType
   * @return ObjectType
   */
  ObjectType getObjectType() const;

  /**
   * @brief Returns the name of the parent object. Returns an empty string if
   * the writer is invalid.
   * @return std::string
   */
  std::string getParentName() const;

  /**
   * @brief Returns the number of attributes in the object. Returns 0 if the
   * object is not valid.
   * @return usize
   */
  usize getNumAttributes() const;

  /**
   * @brief Returns a vector with each attribute name.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getAttributeNames() const;

  std::string getAttributeNameByIndex(int64 id) const;

  /**
   * @brief Deletes the attribute with the specified name.
   * @param name
   */
  void deleteAttribute(const std::string& name);

  bool hasAttribute(const std::string& name) const;

  /**
   * Deletes all attributes.
   */
  void deleteAttributes();

  /**
   * @brief Reads a string attribute with the specified name.
   * @param attributeName
   * @param value
   */
  Result<std::string> readStringAttribute(const std::string& attributeName) const;

  template <typename T>
  Result<T> readScalarAttribute(const std::string& attributeName) const
  {
    if(getId() <= 0)
    {
      return MakeErrorResult<T>(-970, fmt::format("Cannot read attribute '{}'. Object '{}' is invalid", attributeName, getName()));
    }

    auto vectorResult = readVectorAttribute<T>(attributeName);
    if (vectorResult.invalid())
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

  template <typename T>
  Result<std::vector<T>> readVectorAttribute(const std::string& attributeName) const
  {
    if(getId() <= 0)
    {
      return MakeErrorResult<std::vector<T>>(-1, fmt::format("Cannot Read Attribute '{}' within Invalid Object '{}'", attributeName, getName()));
    }

    hid_t attribId = H5Aopen(getId(), attributeName.c_str(), H5P_DEFAULT);
    if (attribId < 0)
    {
      return MakeErrorResult<std::vector<T>>(attribId, fmt::format("Error Opening Attribute '{}' within '{}'", attributeName, getName()));
    }
    hid_t typeId = H5Aget_type(attribId);
    std::vector<T> values(getNumElementsInAttribute(attribId));

    herr_t error = H5Aread(attribId, typeId, values.data());
    H5Aclose(attribId);
    if(error != 0)
    {
      std::string ss = fmt::format("Error Reading Vector Attribute '{}'.", attributeName);
      return MakeErrorResult<std::vector<T>>(error, ss);
    }
    
    return {values};
  }

  Result<> writeStringAttribute(const std::string& attributeName, const std::string& value);

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
    hid_t dataspaceId = H5Screate_simple(rank, &dims, nullptr);
    if(dataspaceId >= 0)
    {
      // Delete existing attribute
      deleteAttribute(attributeName);
      {
        /* Create the attribute. */
        hid_t attributeId = H5Acreate(getId(), attributeName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
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
      }
      /* Close the dataspace. */
      error = H5Sclose(dataspaceId);
      if(error < 0)
      {
        returnError = MakeErrorResult(error, "Error Closing Dataspace");
      }
    }
    else
    {
      returnError = MakeErrorResult(dataspaceId, "Invalid Dataspace ID");
    }

    return returnError;
  }

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
    hid_t dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);
    if(dataspaceId >= 0)
    {
      // Delete any existing attribute
      deleteAttribute(attributeName);
      {
        /* Create the attribute. */
        hid_t attributeId = H5Acreate(getId(), attributeName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
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
      }
      /* Close the dataspace. */
      error = H5Sclose(dataspaceId);
      if(error < 0)
      {
        returnError = MakeErrorResult(error, "Error Closing Dataspace");
      }
    }
    else
    {
      returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace ID");
    }

    return returnError;
  }

  /**
   * @brief Returns the HDF5 filepath
   */
  std::filesystem::path getFilePath() const;

  /**
   * @brief Returns a pointer to the parent FileIO. Returns null if the object is a file.
   * @return FileIO*
   */
  virtual FileIO* parentFile() const;

  /**
   * @brief Returns a pointer to the parent GroupIO. Returns null if there is no known parent.
   * @return GroupIO*
   */
  //GroupIO* parentGroup() const;

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
  * @param Returns the HDF5 object id.
  * Should only be called by HDF5 IO wrapper classes.
  * @return hid_t
  */
  hid_t getId() const;

  hid_t getParentId() const;

protected:
  /**
   * @brief Returns the HDF Type for a given primitive value.
   * Cannot include H5Support due to circular reverences.
   * @return The H5 native type for the value
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
   * @brief Constructs an ObjectIO that wraps a target HDF5 object
   * belonging to the specified filepath and target data path.
   * @param filepath
   * @param objectPath
   */
  ObjectIO(const std::filesystem::path& filepath, const std::string& objectPath);
  ObjectIO(hid_t parentId, const std::string& objectName);

  /**
   * @brief Overwrites the filepath to the target HDF5 file.
   * @param filepath
   */
  void setFilePath(const std::filesystem::path& filepath);

  /**
   * @brief Sets the HDF5 object's name.
   * @param name.
   */
  void setName(const std::string& name);

  /**
   * @brief Moves the HDF5 object's values.
   * @param rhs
   */
  void moveObj(ObjectIO&& rhs) noexcept;

  void setId(hid_t id) const;

  //void setParentGroup(GroupIO* group);
  void setParentId(hid_t parentId);

  /**
   * @brief Reads and returns the string attribute with the target ID.
   * @param id
   * @return
   */
  std::string readStringAttribute(int64 id) const;

  /**
   * @brief Attempts to write a string attribute. Returns a Result<> with any errors encountered.
   * @param objectId
   * @param attributeName
   * @param str
   * @return Result<>
   */
  Result<> writeStringAttribute(int64 objectId, const std::string& attributeName, const std::string& str);

  bool isOpen() const;
  virtual hid_t open() const = 0;
  virtual void close() = 0;

  usize getNumElementsInAttribute(hid_t attribId) const;

private:
  std::filesystem::path m_FilePath;
  std::string m_ObjectName;
  mutable hid_t m_Id = -1;
  //GroupIO* m_ParentGroup = nullptr;
  hid_t m_ParentId = 0;
};
} // namespace nx::core::HDF5
