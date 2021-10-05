#pragma once

#include <any>
#include <string>
#include <vector>

#include "complex/Common/Array.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
namespace H5
{
class COMPLEX_EXPORT AttributeWriter
{
public:
  using DimsVector = std::vector<H5::SizeType>;

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
  AttributeWriter(H5::IdType objectId, const std::string& attributeName);

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
  H5::IdType getObjectId() const;

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
   * @return H5::ErrorType
   */
  H5::ErrorType writeString(const std::string& text);

  /**
   * @brief Writes the specified value to the HDF5 attribute.
   * Returns the resulting HDF5 error code, should an error occur.
   * @tparam T
   * @param value
   * @return H5::ErrorType
   */
  template <typename T>
  H5::ErrorType writeValue(T value);

  /**
   * @brief Writes a vector of values to the HDF5 attribute.
   * @tparam T
   * @param dims HDF5 data dimensions
   * @param vector std::vector of data
   */
  template <typename T>
  H5::ErrorType writeVector(const DimsVector& dims, const std::vector<T>& vector)
  {
    if(!isValid())
    {
      return -1;
    }

    herr_t returnError = 0;
    int32_t rank = static_cast<int32_t>(dims.size());

    hid_t dataType = H5::Support::HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      std::cout << "dataType was unknown" << std::endl;
      return -1;
    }

    /* Get the type of object */
    //H5O_info_t objectInfo;
    //if(H5Oget_info_by_name(getObjectId(), getObjectName().c_str(), &objectInfo, H5P_DEFAULT) < 0)
    //{
    //  std::cout << "Error getting object info at locationId (" << getObjectId() << ") with object name (" << getObjectName() << ")" << std::endl;
    //  return -1;
    //}

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

#if 0
  /**
   * @brief Writes a complex::Array of values to the HDF5 attribute.
   * @tparam T
   * @tparam Dimesnions
   * @param arr complex::Array written to HDF5.
   */
  template <typename T, size_t Dimensions>
  H5::ErrorType writeArray(const complex::Array<T, Dimensions>& arr)
  {
    DimsVector dims = {Dimensions};
    std::vector<T> data(Dimensions);
    for(size_t i = 0; i < Dimensions; i++)
    {
      data[i] = arr[i];
    }
    return AttributeWriter::writeVector<T>(dims, data);
  }
#endif

protected:
  /**
   * @brief Finds and deletes any existing attribute with the current name.
   * Returns any error that might occur when deleting the attribute.
   * @return H5::ErrorType
   */
  H5::ErrorType findAndDeleteAttribute();

private:
  const H5::IdType m_ObjectId = 0;
  const std::string m_AttributeName;
};
} // namespace H5
} // namespace complex
