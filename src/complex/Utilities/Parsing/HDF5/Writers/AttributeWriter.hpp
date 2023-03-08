#pragma once

#include <any>
#include <string>
#include <vector>

#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include "complex/complex_export.hpp"

namespace complex::HDF5
{
class COMPLEX_EXPORT AttributeWriter
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
   * @return ErrorType
   */
  ErrorType writeString(const std::string& text);

  /**
   * @brief Writes the specified value to the HDF5 attribute.
   * Returns the resulting HDF5 error code, should an error occur.
   * @tparam T
   * @param value
   * @return ErrorType
   */
  template <typename T>
  ErrorType writeValue(T value)
  {
    if(!isValid())
    {
      return -1;
    }

    herr_t error = 0;
    herr_t returnError = 0;

    hid_t dataType = Support::HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      return -1;
    }

    /* Create the data space for the attribute. */
    int32_t rank = 1;
    hsize_t dims = 1;
    hid_t dataspaceId = H5Screate_simple(rank, &dims, nullptr);
    if(dataspaceId >= 0)
    {
      // Delete existing attribute
      error = findAndDeleteAttribute();
      if(error >= 0)
      {
        /* Create the attribute. */
        hid_t attributeId = H5Acreate(getObjectId(), getAttributeName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
        if(attributeId >= 0)
        {
          /* Write the attribute data. */
          error = H5Awrite(attributeId, dataType, &value);
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

    return returnError;
  }

  /**
   * @brief Writes a vector of values to the HDF5 attribute.
   * @tparam T
   * @param dims HDF5 data dimensions
   * @param vector std::vector of data
   */
  template <typename T>
  ErrorType writeVector(const DimsVector& dims, const std::vector<T>& vector)
  {
    if(!isValid())
    {
      return -1;
    }

    herr_t returnError = 0;
    int32_t rank = static_cast<int32_t>(dims.size());

    hid_t dataType = Support::HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      std::cout << "dataType was unknown" << std::endl;
      return -1;
    }

    hid_t dataspaceId = H5Screate_simple(rank, dims.data(), nullptr);
    if(dataspaceId >= 0)
    {
      // Delete any existing attribute
      herr_t error = findAndDeleteAttribute();

      if(error >= 0)
      {
        /* Create the attribute. */
        hid_t attributeId = H5Acreate(getObjectId(), getAttributeName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
        if(attributeId >= 0)
        {
          /* Write the attribute data. */
          error = H5Awrite(attributeId, dataType, static_cast<const void*>(vector.data()));
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

    return returnError;
  }

protected:
  /**
   * @brief Finds and deletes any existing attribute with the current name.
   * Returns any error that might occur when deleting the attribute.
   * @return ErrorType
   */
  ErrorType findAndDeleteAttribute();

private:
  const IdType m_ObjectId = 0;
  const std::string m_AttributeName;
};
} // namespace complex::HDF5
