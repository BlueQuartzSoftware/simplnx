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
  H5::ErrorType writeVector(const DimsVector& dims, const std::vector<T>& vector);

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

// declare writeValue
template H5::ErrorType H5::AttributeWriter::writeValue<int8_t>(int8_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<int16_t>(int16_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<int32_t>(int32_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<int64_t>(int64_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<uint8_t>(uint8_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<uint16_t>(uint16_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<uint32_t>(uint32_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<uint64_t>(uint64_t value);
template H5::ErrorType H5::AttributeWriter::writeValue<float>(float value);
template H5::ErrorType H5::AttributeWriter::writeValue<double>(double value);

// declare writeVector
template H5::ErrorType H5::AttributeWriter::writeVector<int8_t>(const DimsVector& dims, const std::vector<int8_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<int16_t>(const DimsVector& dims, const std::vector<int16_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<int32_t>(const DimsVector& dims, const std::vector<int32_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<int64_t>(const DimsVector& dims, const std::vector<int64_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<uint8_t>(const DimsVector& dims, const std::vector<uint8_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<uint16_t>(const DimsVector& dims, const std::vector<uint16_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<uint32_t>(const DimsVector& dims, const std::vector<uint32_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<uint64_t>(const DimsVector& dims, const std::vector<uint64_t>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<float>(const DimsVector& dims, const std::vector<float>& vector);
template H5::ErrorType H5::AttributeWriter::writeVector<double>(const DimsVector& dims, const std::vector<double>& vector);
} // namespace complex
