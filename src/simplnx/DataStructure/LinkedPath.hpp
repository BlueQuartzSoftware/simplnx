#pragma once

#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{

/**
 * @class LinkedPath
 * @brief The LinkedPath class is an alternate way to store a path through the
 * DataStructure. Instead of storing DataObject names like DataPath does, the
 * LinkedPath class stores DataObject IDs and the DataStructure it belongs to.
 * LinkedPath objects can be used to directly create a corresponding DataPath
 * or find the DataObject at any point along the path.
 */
class SIMPLNX_EXPORT LinkedPath
{
public:
  friend class DataStructure;

  /**
   * @brief Constructs an empty LinkedPath. Empty paths are not valid.
   */
  LinkedPath();

  /**
   * @brief Creates a copy of the target LinkedPath.
   * @param rhs The LinkedPath to copy from
   */
  LinkedPath(const LinkedPath& rhs);

  /**
   * @brief Creates a new LinkedPath and moves values from the target path.
   * @param rhs The LinkedPath to move from
   */
  LinkedPath(LinkedPath&& rhs) noexcept;

  /**
   * @brief Copy assignment operator.
   * @param rhs The LinkedPath to copy from
   * @return LinkedPath& Reference to this LinkedPath
   */
  LinkedPath& operator=(const LinkedPath& rhs);

  /**
   * @brief Move assignment operator.
   * @param rhs The LinkedPath to move from
   * @return LinkedPath& Reference to this LinkedPath
   */
  LinkedPath& operator=(LinkedPath&& rhs) noexcept;

  /**
   * @brief Destroys the LinkedPath.
   */
  ~LinkedPath();

  /**
   * @brief Checks if the path is valid.
   *
   * Returns false if the path is empty or any DataObjects along the path
   * cannot be found. Returns true otherwise.
   * @return bool
   */
  bool isValid() const;

  /**
   * @brief Returns a read-only pointer to the DataStructure.
   * @return const DataStructure*
   */
  const DataStructure* getDataStructure() const;

  /**
   * @brief Constructs a DataPath using the current names of the DataObjects
   * along the LinkedPath.
   * @return DataPath
   */
  DataPath toDataPath() const;

  /**
   * @brief Returns the number of items in the path.
   * @return usize
   */
  usize getLength() const;

  /**
   * @brief Returns the DataObject ID at the specified position in the path.
   * This method does not perform bounds checking.
   * @param index The position in the path
   * @return DataObject::IdType The DataObject ID at the specified position
   */
  DataObject::IdType operator[](usize index) const;

  /**
   * @brief Returns the ID for the target DataObject.
   *
   * Throws an exception if the path is empty. Otherwise, this will operate
   * even if the path is otherwise invalid.
   * @return DataObject::IdType
   */
  DataObject::IdType getId() const;

  /**
   * @brief Returns the DataObject ID at the specified position in the path.
   * This method does not perform bounds checking.
   * @param index The position in the path
   * @return DataObject::IdType The DataObject ID at the specified position
   */
  DataObject::IdType getIdAt(usize index) const;

  /**
   * @brief Returns a pointer to the const DataObject targetted by the path.
   * @return const DataObject*
   */
  const DataObject* getData() const;

  /**
   * @brief Returns a pointer to the const DataObject at the specified path index.
   * @param index The position in the path
   * @return const DataObject* Pointer to the DataObject at the specified position, or nullptr if not found
   */
  const DataObject* getDataAt(usize index) const;

  /**
   * @brief Returns the name of the target DataObject. Throws an exception if
   * the DataObject does not exist.
   * @return std::string
   */
  std::string getName() const;

  /**
   * @brief Returns the name of the DataObject pointed to by the target position
   * of the path. Returns "[ missing ]" if the DataObject could not be found.
   * @param index The position in the path
   * @return std::string The name of the DataObject at the specified position
   */
  std::string getNameAt(usize index) const;

  /**
   * @brief Returns a string representation of the path using the provided divider
   * between DataObject names. If no divider is provided, " / " is used instead.
   *
   * Names are provided using getNameAt(usize).
   * @param div The divider string to use between names (default: " / ")
   * @return std::string String representation of the LinkedPath
   */
  std::string toString(const std::string& div = " / ") const;

  /**
   * @brief Checks equality with the specified LinkedPath.
   * @param rhs The LinkedPath to compare with
   * @return bool True if the paths are equal, false otherwise
   */
  bool operator==(const LinkedPath& rhs) const;

  /**
   * @brief Checks equality with the specified DataPath.
   * @param rhs The DataPath to compare with
   * @return bool True if the paths are equal, false otherwise
   */
  bool operator==(const DataPath& rhs) const;

  /**
   * @brief Checks inequality with the specified LinkedPath.
   * @param rhs The LinkedPath to compare with
   * @return bool True if the paths are not equal, false otherwise
   */
  bool operator!=(const LinkedPath& rhs) const;

  /**
   * @brief Checks inequality with the specified DataPath.
   * @param rhs The DataPath to compare with
   * @return bool True if the paths are not equal, false otherwise
   */
  bool operator!=(const DataPath& rhs) const;

protected:
  /**
   * @brief Constructs a LinkedPath for the target DataStructure and vector of DataObject IDs.
   * @param dataStructure Pointer to the DataStructure this path belongs to
   * @param idPath Vector of DataObject IDs defining the path
   */
  LinkedPath(const DataStructure* dataStructure, const std::vector<DataObject::IdType>& idPath);

private:
  const DataStructure* m_DataStructure = nullptr;
  std::vector<DataObject::IdType> m_IdPath;
};
} // namespace nx::core
