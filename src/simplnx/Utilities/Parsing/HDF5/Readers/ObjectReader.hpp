#pragma once

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Readers/AttributeReader.hpp"

#include <string>
#include <vector>

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT ObjectReader
{
public:
  /**
   * @brief Constructs an invalid ObjectReader.
   */
  ObjectReader();

  /**
   * @brief Constructs an ObjectReader that wraps a target HDF5 object
   * belonging to the specified parent using the target name.
   * @param parentId
   * @param targetName
   */
  ObjectReader(IdType parentId, const std::string& targetName);

  ObjectReader(const ObjectReader& other) = delete;
  ObjectReader(ObjectReader&& other) noexcept;

  ObjectReader& operator=(const ObjectReader& other) = delete;
  ObjectReader& operator=(ObjectReader&& other) noexcept;

  /**
   * @brief Releases the wrapped HDF5 object.
   */
  virtual ~ObjectReader() noexcept;

  /**
   * @brief Returns true if the ObjectReader has a valid target. Otherwise,
   * this method returns false.
   * @return bool
   */
  virtual bool isValid() const;

  /**
   * @brief Returns the parent object ID. Returns 0 if no parent was set.
   * @return IdType
   */
  IdType getParentId() const;

  /**
   * @brief Returns the object's address ID. Returns 0 if the object is invalid.
   * @return IdType
   */
  haddr_t getObjectId() const;

  /**
   * @brief Returns the object's HDF5 ID. Returns 0 if the object is invalid.
   * @return IdType
   */
  IdType getId() const;

  /**
   * @brief Sets the object's HDF5 ID.
   * @param identifier
   */
  void setId(IdType identifier);

  /**
   * @brief Returns the HDF5 object name. Returns an empty string if the file
   * is invalid.
   * @return std::string
   */
  virtual std::string getName() const;

  /**
   * @brief Returns the number of attributes in the object. Returns 0 if the
   * object is not valid.
   * @return size_t
   */
  size_t getNumAttributes() const;

  /**
   * @brief Returns a vector with each attribute name.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getAttributeNames() const;

  /**
   * @brief Returns an AttributeReader for the target HDF5 attribute. Returns
   * an invalid AttributeReader if no matching attribute exists.
   * @param name
   * @return AttributeReader
   */
  AttributeReader getAttribute(const std::string& name) const;

  /**
   * @brief Returns an AttributeReader for the target HDF5 attribute. Returns
   * an invalid AttributeReader if no matching attribute exists.
   * @param idx
   * @return AttributeReader
   */
  AttributeReader getAttributeByIdx(size_t idx) const;

protected:
  /**
   * @brief Constructs an ObjectReader for use in derived classes. This
   * constructor only accepts the parent ID. Derived classes are required to
   * open their own target and provide the ID via getId().
   * @param parentId
   */
  ObjectReader(IdType parentId);

  /**
   * @brief Constructs an ObjectReader for use in derived classes. This
   * constructor only accepts the parent ID and the (object) ID. Derived classes
   * are required to open their own target and provide the ID.
   * @param parentId
   * @param objectId
   */
  ObjectReader(IdType parentId, IdType objectId);

  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  virtual void closeHdf5();

private:
  IdType m_ParentId = 0;
  IdType m_Id = 0; // the object, group, file, or dataset identifier
};
} // namespace nx::core::HDF5
