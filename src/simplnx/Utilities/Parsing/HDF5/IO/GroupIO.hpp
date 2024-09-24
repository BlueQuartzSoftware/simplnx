#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

#include "highfive/H5Attribute.hpp"
#include "highfive/H5File.hpp"
#include "highfive/H5Group.hpp"

#include "fmt/format.h"

#include <string>

namespace nx::core::HDF5
{
class FileIO;

class SIMPLNX_EXPORT GroupIO : public ObjectIO
{
public:
  static HighFive::Group Open(const std::filesystem::path& filepath, const std::string& objectPath);

  /**
   * @brief Constructs an invalid GroupIO.
   */
  GroupIO();

  /**
   * @brief Opens and wraps an HDF5 group.
   * @param filepath
   * @param groupPath
   */
  GroupIO(GroupIO& parentGroup, HighFive::Group&& group, const std::string& groupName);

  GroupIO(const GroupIO& other) = delete;
  GroupIO(GroupIO&& other) noexcept = default;

  GroupIO& operator=(const GroupIO& other) = delete;
  GroupIO& operator=(GroupIO&& other) noexcept = default;

  /**
   * @brief Releases the wrapped HDF5 group.
   */
  ~GroupIO() noexcept override;

  /**
   * @brief Attempts to open a nested HDF5 group with the specified name.
   * The created GroupIO is returned. If the process fails, the returned
   * GroupIO is invalid.
   * @param name
   * @return GroupIO
   */
  virtual Result<GroupIO> openGroup(const std::string& name) const;

  virtual std::shared_ptr<GroupIO> openGroupPtr(const std::string& name) const;

  /**
   * @brief Attempts to open a nested HDF5 dataset with the specified name.
   * The created DatasetReader is returned. If the process fails, the returned
   * DatasetReader is invalid.
   * @param name
   * @return DatasetReader
   */
  Result<DatasetIO> openDataset(const std::string& name) const;

  std::shared_ptr<DatasetIO> openDatasetPtr(const std::string& name) const;

  /**
   * @brief Creates a GroupIO for writing to a child group with the
   * target name. Returns an invalid GroupIO if the group cannot be
   * created.
   * @param childName
   * @return GroupIO
   */
  virtual Result<GroupIO> createGroup(const std::string& childName);

  virtual std::shared_ptr<GroupIO> createGroupPtr(const std::string& childName);

  /**
   * @brief Opens a DatasetIO for writing to a child group with the
   * target name. Returns an invalid DatasetIO Result if the dataset cannot be
   * created.
   * @param childName
   * @return Result<DatasetIO>
   */
  Result<DatasetIO> openDataset(const std::string& childName);

  /**
   * @brief Opens a DatasetIO for writing to a child group with the
   * target name. Returns a null pointer if the dataset cannot be
   * created.
   * @param childName
   * @return std::shared_ptr<DatasetIO>
   */
  std::shared_ptr<DatasetIO> openDatasetPtr(const std::string& childName);

  /**
   * @brief Creates a DatasetIO for writing to a child group with the
   * target name. Returns an invalid DatasetIO if the dataset cannot be
   * created.
   * @param childName
   * @return DatasetIO
   */
  Result<DatasetIO> createDataset(const std::string& childName);

  std::shared_ptr<DatasetIO> createDatasetPtr(const std::string& childName);

  /**
   * @brief Creates a link within the group to another HDF5 object specified
   * by an HDF5 object path.
   * Returns an error code if one occurs. Otherwise, this method returns 0.
   * @param objectPath
   * @return Result<>
   */
  Result<> createLink(const std::string& objectPath);

  /**
   * @brief Returns the IO Classes HighFive ObjectType
   * @return HighFive::ObjectType
   */
  HighFive::ObjectType getObjectType() const override;

  /**
   * @brief Returns the number of children objects within the group.
   *
   * Returns 0 if the GroupIO is invalid.
   * @return size_t
   */
  virtual usize getNumChildren() const;

  /**
   * @brief Returns a vector with the names of each child object.
   *
   * This will return an empty vector if the GroupIO is invalid.
   * @return std::vector<std::string>
   */
  virtual std::vector<std::string> getChildNames() const;

  /**
   * @brief Returns true if the target child is a group. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  virtual bool isGroup(const std::string& childName) const;

  /**
   * @brief Returns true if the target child is a dataset. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  virtual bool isDataset(const std::string& childName) const;

  /**
   * @brief Returns the number of attributes in the object. Returns 0 if the
   * object is not valid.
   * @return usize
   */
  usize getNumAttributes() const override;

  /**
   * @brief Returns a vector with each attribute name.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getAttributeNames() const override;

  /**
   * @brief Deletes the attribute with the specified name.
   * @param name
   */
  void deleteAttribute(const std::string& name) override;

  /**
   * @brief Creates an attribute with the specified name and value.
   * @param attributeName
   * @param value
   */
  template <typename T>
  void createAttribute(const std::string& attributeName, const T& value)
  {
    auto* parentPtr = parentGroup();
    if(parentPtr == nullptr)
    {
      auto optFile = h5File();
      if(optFile.has_value() == false)
      {
        return;
      }
      if(optFile->hasAttribute(attributeName))
      {
        optFile->deleteAttribute(attributeName);
      }
      optFile->createAttribute(attributeName, value);
    }
    else
    {
      if(m_Group.hasAttribute(attributeName))
      {
        m_Group.deleteAttribute(attributeName);
      }
      m_Group.createAttribute(attributeName, value);
    }
  }

  /**
   * @brief Creates a string attribute with the specified name and value.
   * @param attributeName
   * @param value
   */
  template <>
  void createAttribute<std::string>(const std::string& attributeName, const std::string& value)
  {
    auto* parentPtr = parentGroup();
    if(parentPtr == nullptr)
    {
      auto optFile = h5File();
      if(optFile.has_value() == false)
      {
        return;
      }
      if(optFile->hasAttribute(attributeName))
      {
        optFile->deleteAttribute(attributeName);
      }
      writeStringAttribute(optFile->getId(), attributeName, value);
    }
    else
    {
      if(m_Group.hasAttribute(attributeName))
      {
        m_Group.deleteAttribute(attributeName);
      }
      writeStringAttribute(m_Group.getId(), attributeName, value);
    }
  }

  /**
   * @brief Reads an attribute with the specified name.
   * @param attributeName
   * @param value
   */
  template <typename T>
  void readAttribute(const std::string& attributeName, T& value) const
  {
    auto* parentPtr = parentGroup();
    if(parentPtr == nullptr)
    {
      auto optFile = h5File();
      if(optFile.has_value())
      {
        try
        {
          value = optFile->getAttribute(attributeName).read<T>();
        } catch(const std::exception& e)
        {
          std::string msg = fmt::format("Failed to read attribute '{}' in file at path '{}'. Error: {}", attributeName, getFilePath().string(), e.what());
          throw std::runtime_error(msg);
        }
      }
    }
    else
    {
      std::string path = getObjectPath();
      if(!m_Group.hasAttribute(attributeName))
      {
        std::string msg = fmt::format("Cannot read attribute '{}' at path '{}' because it does not exist", attributeName, path);
        throw std::runtime_error(msg);
      }
      else
      {
        m_Group.getAttribute(attributeName).read(value);
      }
    }
  }

  /**
   * @brief Reads a string attribute with the specified name.
   * @param attributeName
   * @param value
   */
  template <>
  void readAttribute<std::string>(const std::string& attributeName, std::string& value) const
  {
    auto* parentPtr = parentGroup();
    if(parentPtr == nullptr)
    {
      auto optFile = h5File();
      if(optFile.has_value())
      {
        try
        {
          auto attrib = optFile->getAttribute(attributeName);
          value = readStringAttribute(attrib.getId());
        } catch(const std::exception& e)
        {
          std::string msg = fmt::format("Failed to read attribute '{}' in file at path '{}'. Error: {}", attributeName, getFilePath().string(), e.what());
          throw std::runtime_error(msg);
        }
      }
    }
    else
    {
      std::string path = getObjectPath();
      if(!m_Group.hasAttribute(attributeName))
      {
        std::string msg = fmt::format("Cannot read attribute '{}' at path '{}' because it does not exist", attributeName, path);
        throw std::runtime_error(msg);
      }
      else
      {
        auto attrib = m_Group.getAttribute(attributeName);
        value = readStringAttribute(attrib.getId());
      }
    }
  }

  /**
   * @brief Returns a reference to the target HighFive::Group.
   * This should only be called by the HDF5 IO wrappers.
   * @return HighFive::Group&
   */
  HighFive::Group& groupRef();

  /**
   * @brief Returns a const reference to the target HighFive::Group.
   * This should only be called by the HDF5 IO wrappers.
   * @return const HighFive::Group&
   */
  const HighFive::Group& groupRef() const;

  /**
   * @brief Opens and returns the HighFive::DataSet of the target name.
   * This should only be called by the HDF5 IO wrappers.
   * @param name
   * @return HighFive::DataSet
   */
  virtual HighFive::DataSet openH5Dataset(const std::string& name) const;

  /**
   * @brief Creates or opens and returns the HighFive::DataSet of the target name.
   * This should only be called by the HDF5 IO wrappers.
   * @param name
   * @param dims
   * @param dataType
   * @return HighFive::DataSet
   */
  virtual HighFive::DataSet createOrOpenH5Dataset(const std::string& name, const HighFive::DataSpace& dims, HighFive::DataType dataType);

  /**
   * @brief Creates or opens and returns the HighFive::DataSet of the target name.
   * This should only be called by the HDF5 IO wrappers.
   * @param name
   * @param dims
   * @return HighFive::DataSet
   */
  template <typename T>
  HighFive::DataSet createOrOpenH5Dataset(const std::string& name, const HighFive::DataSpace& dims)
  {
    return createOrOpenH5Dataset(name, dims, HighFive::create_datatype<T>());
  }

  /**
   * @brief Creates or opens and returns the boolean HighFive::DataSet of the target name.
   * This should only be called by the HDF5 IO wrappers.
   * @param name
   * @param dims
   * @return HighFive::DataSet
   */
  template <>
  HighFive::DataSet createOrOpenH5Dataset<bool>(const std::string& name, const HighFive::DataSpace& dims)
  {
    return createOrOpenH5Dataset(name, dims, HighFive::create_datatype<H5_BOOL_TYPE>());
  }

  /**
  * @brief Creates or opens an HDF5 dataset using the HDF5 C api.
  * 
  * This should only be called by DatasetIO for writing strings.
  * @param name
  * @param typeId
  * @param dataspaceId
  * @param propertiesId = H5P_DEFAULT
  * @return hid_t
  */
  hid_t createOrOpenHDF5Dataset(const std::string& name, hid_t typeId, hid_t dataspaceId, hid_t propertiesId = H5P_DEFAULT);

  /**
   * @param Returns the HDF5 object id.
   * Should only be called by HDF5 IO wrapper classes.
   * @return hid_t
   */
  hid_t getH5Id() const override;

private:
  HighFive::Group m_Group;
};

// -----------------------------------------------------------------------------
// Declare our extern templates
} // namespace nx::core::HDF5
