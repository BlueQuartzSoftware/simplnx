#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include "highfive/H5DataType.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace nx::core::HDF5
{
class FileIO;
class GroupIO;

class SIMPLNX_EXPORT ObjectIO
{
public:
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
   * @brief Returns the IO Classes HighFive ObjectType
   * @return HighFive::ObjectType
   */
  virtual HighFive::ObjectType getObjectType() const = 0;

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
  virtual usize getNumAttributes() const = 0;

  /**
   * @brief Returns a vector with each attribute name.
   * @return std::vector<std::string>
   */
  virtual std::vector<std::string> getAttributeNames() const = 0;

  /**
   * @brief Deletes the attribute with the specified name.
   * @param name
   */
  virtual void deleteAttribute(const std::string& name) = 0;

  /**
   * Deletes all attributes.
   */
  void deleteAttributes();

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
  GroupIO* parentGroup() const;

  /**
   * @brief Returns the HighFive::File for the current IO handler.
   * Returns an empty optional if the file could not be determined.
   *
   * This method should only be called by simplnx HDF5 IO wrapper classes.
   * @return std::optional<HighFive::File>
   */
  virtual std::optional<HighFive::File> h5File() const;

  /**
  * @param Returns the HDF5 object id.
  * Should only be called by HDF5 IO wrapper classes.
  * @return hid_t
  */
  virtual hid_t getH5Id() const = 0;

protected:
  /**
   * @brief Constructs an ObjectIO that wraps a target HDF5 object
   * belonging to the specified filepath and target data path.
   * @param filepath
   * @param objectPath
   */
  ObjectIO(const std::filesystem::path& filepath, const std::string& objectPath);
  ObjectIO(GroupIO& group, const std::string& objectName);

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

private:
  std::filesystem::path m_FilePath;
  std::string m_ObjectName;
  std::optional<GroupIO*> m_ParentGroup;
};
} // namespace nx::core::HDF5
