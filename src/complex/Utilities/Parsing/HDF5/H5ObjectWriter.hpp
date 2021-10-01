#pragma once

#include <string>

#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5AttributeWriter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
namespace H5
{
class COMPLEX_EXPORT ObjectWriter
{
public:
  /**
   * @brief Constructs an invalid ObjectWriter.
   */
  ObjectWriter();

  /**
   * @brief Constructs an ObjectWriter that wraps a target HDF5 object
   * belonging to the specified parent. Opening the target HDF5 object is
   * left to the derived class.
   * @param parentId
   */
  ObjectWriter(H5::IdType parentId);

  /**
   * @brief Releases the wrapped HDF5 object.
   */
  virtual ~ObjectWriter();

  /**
   * @brief Returns true if the ObjectWriter has a valid target. Otherwise,
   * this method returns false.
   * @return bool
   */
  virtual bool isValid() const = 0;

  /**
   * @brief Returns the parent object ID. Returns 0 if no parent was set.
   * @return H5::IdType
   */
  H5::IdType getParentId() const;

  /**
   * @brief Returns the name of the parent object. Returns an empty string if
   * the writer is invalid.
   * @return std::string
   */
  std::string getParentName() const;

  /**
   * @brief Returns the group's HDF5 ID. Returns 0 if the object is invalid.
   * @return H5::IdType
   */
  virtual H5::IdType getId() const = 0;

  /**
   * @brief Returns the HDF5 object name. Returns an empty string if the writer
   * is invalid.
   * @return std::string
   */
  virtual std::string getName() const;

  /**
   * @brief Returns an AttributeWriter for the target HDF5 attribute. Returns
   * an invalid AttributeWriter if the writer is invalid.
   * @param name
   * @return AttributeWriter
   */
  AttributeWriter createAttribute(const std::string& name);

private:
  H5::IdType m_ParentId = 0;
};
} // namespace H5
} // namespace complex
