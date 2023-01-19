#pragma once

#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/complex_export.hpp"

#include <H5Apublic.h>

#include <iostream>
#include <string>
#include <vector>

namespace complex
{
namespace H5
{
class COMPLEX_EXPORT AttributeReader
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
  AttributeReader(H5::IdType objectId, size_t attrIdx);

  /**
   * @brief Constructs an AttributeReader wrapping a target HDF5 attribute
   * belonging to the specified object with the target name.
   * @param objectId
   * @param attrName
   */
  AttributeReader(H5::IdType objectId, const std::string& attrName);

  /**
   * @brief Releases the wrapped HDF5 attribute.
   */
  virtual ~AttributeReader();

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
   * @brief Returns the attribute's HDF5 ID. Returns 0 if the attribute is
   * invalid.
   * @return H5::IdType
   */
  H5::IdType getAttributeId() const;

  /**
   * @brief Returns the dataspace's HDF5 ID. Returns 0 if the attribute is
   * invalid.
   * @return IdType
   */
  H5::IdType getDataspaceId() const;

  /**
   * @brief Returns the HDF5 attribute name. Returns an empty string if the
   * attribute is invalid.
   * @return std::string
   */
  std::string getName() const;

  /**
   * @brief Returns an enum representation of the attribute's type.
   * @return H5::Type
   */
  H5::Type getType() const;

  /**
   * @brief Returns an H5T_class_t enum representation of the attribute's class type.
   * @return H5::Type
   */
  H5::IdType getClassType() const;

  /**
   * @brief Returns the HDF5 type ID for the attribute. Returns 0 if the
   * attribute is invalid.
   * @return H5::TypeId
   */
  H5::IdType getTypeId() const;

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
    H5::IdType typeId = getTypeId();

    H5::ErrorType error = H5Aread(getAttributeId(), typeId, values.data());
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
  H5::IdType m_ObjectId = 0;
  H5::IdType m_AttributeId = 0;
};
} // namespace H5
} // namespace complex
