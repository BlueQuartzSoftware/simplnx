#pragma once

#include <any>
#include <string>
#include <vector>

#include "simplnx/Common/Result.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "simplnx/simplnx_export.hpp"

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT AttributeWriter
{
public:
  using DimsVector = std::vector<SizeType>;

  /**
   * @brief Constructs an invalid AttributeWriter.
   */
  AttributeWriter();

  /**
   * @brief Constructs an AttributeWriter for an attribute within the specified
   * HDF5 object.
   * @param objectId Target HDF5 object to create / write the attribute
   * @param attributeName Name of the HDF5 attribute
   */
  AttributeWriter(IdType objectId, const std::string& attributeName);

  /**
   * @brief Default destructor
   */
  virtual ~AttributeWriter();

  /**
   * @brief Returns true if the AttributeReader has a valid target.
   * Otherwise, this method returns false.
   * @return bool
   */
  bool isValid() const;

  /**
   * @brief Returns the parent object's ID. Returns 0 if no object was provided.
   * @return H5:IdType
   */
  IdType getObjectId() const;

  /**
   * @brief Returns the HDF5 object name. Returns an empty string if the object
   * is invalid.
   * @return std::string
   */
  std::string getObjectName() const;

  /**
   * @brief Returns the HDF5 attribute name. Returns an empty string if the
   * attribute is invalid.
   * @return std::string
   */
  std::string getAttributeName() const;

  /**
   * @brief Writes the specified string to the HDF5 attribute.
   * Returns the resulting HDF5 error code if any occurred.
   * @param text
   * @return Result<>
   */
  Result<> writeString(const std::string& text);

  /**
   * @brief Writes the specified value to the HDF5 attribute.
   * Returns the resulting HDF5 error code, should an error occur.
   * @tparam T
   * @param value
   * @return Result<>
   */
  template <typename T>
  Result<> writeValue(T value)
  {
    if(!isValid())
    {
      return MakeErrorResult(-100, "Cannot write to invalid AttributeWriter");
    }

    herr_t error = 0;
    Result<> returnError = {};

    hid_t dataType = Support::HdfTypeForPrimitive<T>();
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
      auto result = findAndDeleteAttribute();
      if(result.valid())
      {
        /* Create the attribute. */
        hid_t attributeId = H5Acreate(getObjectId(), getAttributeName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
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

  /**
   * @brief Writes a vector of values to the HDF5 attribute.
   * @tparam T
   * @param dims HDF5 data dimensions
   * @param vector std::vector of data
   * @return Result<>
   */
  template <typename T>
  Result<> writeVector(const DimsVector& dims, const std::vector<T>& vector)
  {
    if(!isValid())
    {
      return MakeErrorResult(-100, "Cannot write to invalid AttributeWriter");
    }

    Result<> returnError = {};
    ErrorType error = 0;
    int32_t rank = static_cast<int32_t>(dims.size());

    hid_t dataType = Support::HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      return MakeErrorResult(-101, "Unknown data type");
    }

    hid_t dataspaceId = H5Screate_simple(rank, dims.data(), nullptr);
    if(dataspaceId >= 0)
    {
      // Delete any existing attribute
      auto result = findAndDeleteAttribute();
      if(result.valid())
      {
        /* Create the attribute. */
        hid_t attributeId = H5Acreate(getObjectId(), getAttributeName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
        if(attributeId >= 0)
        {
          /* Write the attribute data. */
          error = H5Awrite(attributeId, dataType, static_cast<const void*>(vector.data()));
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

protected:
  /**
   * @brief Finds and deletes any existing attribute with the current name.
   * Returns any error that might occur when deleting the attribute.
   * @return Result<>
   */
  Result<> findAndDeleteAttribute();

private:
  const IdType m_ObjectId = 0;
  const std::string m_AttributeName;
};
} // namespace nx::core::HDF5
