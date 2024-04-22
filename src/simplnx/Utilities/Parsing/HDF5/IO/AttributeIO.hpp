#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/simplnx_export.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT AttributeIO
{
public:
  using DimsVector = std::vector<SizeType>;

  /**
   * @brief Constructs an invalid AttributeIO.
   */
  AttributeIO();

  /**
   * @brief Constructs an AttributeIO wrapping a target HDF5 attribute
   * belonging to the specified object at the target index.
   * @param objectId
   * @param attrIdx
   */
  AttributeIO(IdType objectId, size_t attrIdx);

  /**
   * @brief Constructs an AttributeIO wrapping a target HDF5 attribute
   * belonging to the specified object with the target name.
   * @param objectId
   * @param attrName
   */
  AttributeIO(IdType objectId, const std::string& attrName);

  AttributeIO(const AttributeIO& other) = delete;
  AttributeIO(AttributeIO&& other) noexcept;

  AttributeIO& operator=(const AttributeIO& other) = delete;
  AttributeIO& operator=(AttributeIO&& other) noexcept;

  /**
   * @brief Releases the wrapped HDF5 attribute.
   */
  ~AttributeIO() noexcept;

  /**
   * @brief Returns true if the AttributeIO has a valid target.
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
   * @brief Returns the attribute's HDF5 ID. Returns 0 if the attribute is
   * invalid.
   * @return IdType
   */
  IdType getAttributeId() const;

  /**
   * @brief Returns the dataspace's HDF5 ID. Returns 0 if the attribute is
   * invalid.
   * @return IdType
   */
  IdType getDataspaceId() const;

  /**
   * @brief Returns the HDF5 attribute name. Returns an empty string if the
   * attribute is invalid.
   * @return std::string
   */
  std::string getName() const;

  /**
   * @brief Returns the HDF5 object name. Returns an empty string if the object
   * is invalid.
   * @return std::string
   */
  std::string getObjectName() const;

  /**
   * @brief Returns an enum representation of the attribute's type.
   * @return Type
   */
  Type getType() const;

  /**
   * @brief Returns an H5T_class_t enum representation of the attribute's class
   * type.
   * @return Type
   */
  IdType getClassType() const;

  /**
   * @brief Returns the HDF5 type ID for the attribute. Returns 0 if the
   * attribute is invalid.
   * @return TypeId
   */
  IdType getTypeId() const;

  /**
   * @brief Returns the number of elements in the attribute.
   * @return size_t
   */
  size_t getNumElements() const;

  /**
   * @brief Returns the value for the attribute.
   * Returns the type's default value if no attribute exists or the attribute
   * is not of the specified type.
   * @tparam T
   * @return T
   */
  template <typename T>
  T readAsValue() const;

  /**
   * @brief Returns a vector of values for the attribute.
   * Returns an empty vector if no attribute exists or the attribute is not of
   * the specified type.
   * @tparam T
   * @return std::vector<T>
   */
  template <typename T>
  std::vector<T> readAsVector() const;

  /**
   * @brief Returns the value for the attribute as a string. Returns an empty
   * string if the AttributeIO is not valid or the type is not a string.
   * @return std::string
   */
  std::string readAsString() const;

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
  Result<> writeValue(T value);

  /**
   * @brief Writes a vector of values to the HDF5 attribute.
   * @tparam T
   * @param dims HDF5 data dimensions
   * @param vector std::vector of data
   * @return Result<>
   */
  template <typename T>
  Result<> writeVector(const DimsVector& dims, const std::vector<T>& vector);

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5();

  /**
   * @brief Finds and deletes any existing attribute with the current name.
   * Returns any error that might occur when deleting the attribute.
   * @return Result<>
   */
  Result<> findAndDeleteAttribute();

private:
  IdType m_ObjectId = 0;
  IdType m_AttributeId = 0;
  std::string m_AttributeName;
};

extern template SIMPLNX_EXPORT int8_t AttributeIO::readAsValue<int8_t>() const;
extern template SIMPLNX_EXPORT int16_t AttributeIO::readAsValue<int16_t>() const;
extern template SIMPLNX_EXPORT int32_t AttributeIO::readAsValue<int32_t>() const;
extern template SIMPLNX_EXPORT int64_t AttributeIO::readAsValue<int64_t>() const;
extern template SIMPLNX_EXPORT uint8_t AttributeIO::readAsValue<uint8_t>() const;
extern template SIMPLNX_EXPORT uint16_t AttributeIO::readAsValue<uint16_t>() const;
extern template SIMPLNX_EXPORT uint32_t AttributeIO::readAsValue<uint32_t>() const;
extern template SIMPLNX_EXPORT uint64_t AttributeIO::readAsValue<uint64_t>() const;
extern template SIMPLNX_EXPORT float AttributeIO::readAsValue<float>() const;
extern template SIMPLNX_EXPORT double AttributeIO::readAsValue<double>() const;
extern template SIMPLNX_EXPORT bool AttributeIO::readAsValue<bool>() const;

extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<int8_t>(int8_t value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<int16_t>(int16_t value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<int32_t>(int32_t value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<int64_t>(int64_t value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<uint8_t>(uint8_t value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<uint16_t>(uint16_t value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<uint32_t>(uint32_t value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<uint64_t>(uint64_t value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<float>(float value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<double>(double value);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeValue<bool>(bool value);

extern template SIMPLNX_EXPORT std::vector<int8_t> AttributeIO::readAsVector<int8_t>() const;
extern template SIMPLNX_EXPORT std::vector<int16_t> AttributeIO::readAsVector<int16_t>() const;
extern template SIMPLNX_EXPORT std::vector<int32_t> AttributeIO::readAsVector<int32_t>() const;
extern template SIMPLNX_EXPORT std::vector<int64_t> AttributeIO::readAsVector<int64_t>() const;
extern template SIMPLNX_EXPORT std::vector<uint8_t> AttributeIO::readAsVector<uint8_t>() const;
extern template SIMPLNX_EXPORT std::vector<uint16_t> AttributeIO::readAsVector<uint16_t>() const;
extern template SIMPLNX_EXPORT std::vector<uint32_t> AttributeIO::readAsVector<uint32_t>() const;
extern template SIMPLNX_EXPORT std::vector<uint64_t> AttributeIO::readAsVector<uint64_t>() const;
extern template SIMPLNX_EXPORT std::vector<float> AttributeIO::readAsVector<float>() const;
extern template SIMPLNX_EXPORT std::vector<double> AttributeIO::readAsVector<double>() const;
extern template SIMPLNX_EXPORT std::vector<bool> AttributeIO::readAsVector<bool>() const;

extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<int8_t>(const AttributeIO::DimsVector& dims, const std::vector<int8_t>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<int16_t>(const AttributeIO::DimsVector& dims, const std::vector<int16_t>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<int32_t>(const AttributeIO::DimsVector& dims, const std::vector<int32_t>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<int64_t>(const AttributeIO::DimsVector& dims, const std::vector<int64_t>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<uint8_t>(const AttributeIO::DimsVector& dims, const std::vector<uint8_t>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<uint16_t>(const AttributeIO::DimsVector& dims, const std::vector<uint16_t>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<uint32_t>(const AttributeIO::DimsVector& dims, const std::vector<uint32_t>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<uint64_t>(const AttributeIO::DimsVector& dims, const std::vector<uint64_t>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<float>(const AttributeIO::DimsVector& dims, const std::vector<float>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<double>(const AttributeIO::DimsVector& dims, const std::vector<double>& vector);
extern template SIMPLNX_EXPORT Result<> AttributeIO::writeVector<bool>(const AttributeIO::DimsVector& dims, const std::vector<bool>& vector);
} // namespace nx::core::HDF5
