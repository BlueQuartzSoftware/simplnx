#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <H5Apublic.h>

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"

#include "simplnx/simplnx_export.hpp"

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT AttributeReader
{
public:
  /**
   * @brief Constructs an invalid AttributeReader.
   */
  AttributeReader();

  /**
   * @brief Constructs an AttributeReader wrapping a target HDF5 attribute
   * belonging to the specified object at the target index.
   * @param objectId
   * @param attrIdx
   */
  AttributeReader(IdType objectId, size_t attrIdx);

  /**
   * @brief Constructs an AttributeReader wrapping a target HDF5 attribute
   * belonging to the specified object with the target name.
   * @param objectId
   * @param attrName
   */
  AttributeReader(IdType objectId, const std::string& attrName);

  /**
   * @brief Releases the wrapped HDF5 attribute.
   */
  virtual ~AttributeReader() noexcept;

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
  std::vector<T> readAsVector() const
  {
    if(!isValid())
    {
      return {};
    }

    std::vector<T> values(getNumElements());
    IdType typeId = getTypeId();

    ErrorType error = H5Aread(getAttributeId(), typeId, values.data());
    if(error != 0)
    {
      std::cout << "Error Reading Attribute." << error << std::endl;
      return {};
    }

    return values;
  }

  /**
   * @brief Returns the value for the attribute as a string. Returns an empty
   * string if the AttributeReader is not valid or the type is not a string.
   * @return std::string
   */
  std::string readAsString() const;

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5();

private:
  IdType m_ObjectId = 0;
  IdType m_AttributeId = 0;
};
} // namespace nx::core::HDF5
