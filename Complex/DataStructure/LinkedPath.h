#pragma once

#include "Complex/DataStructure/DataObject.h"
#include "Complex/DataStructure/DataPath.h"

namespace Complex
{

/**
 * class LinkedPath
 *
 */

class LinkedPath
{
public:
  friend class DataStructure;

  /**
   * @brief Constructs an empty LinkedPath.
   */
  LinkedPath();

  /**
   * @brief Copy constructor
   * @param rhs
   */
  LinkedPath(const LinkedPath& rhs);

  /**
   * @brief Move constructor
   * @param rhs
   */
  LinkedPath(LinkedPath&& rhs) noexcept;

  virtual ~LinkedPath();

  /**
   * @brief Checks if the path is valid. Returns false if the path is empty or
   * any DataObjects along the path cannot be found. Returns true otherwise.
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
   * @return size_t
   */
  size_t getLength() const;

  /**
   * @brief Returns the DataObject ID at the specified position in the path.
   * This method does not perform bounds checking.
   * @param index
   * @return
   */
  DataObject::IdType operator[](size_t index) const;

  /**
   * @brief Returns the ID for the target DataObject. Throws an exception if
   * the path is empty.
   * @return DataObject::IdType
   */
  DataObject::IdType getId() const;

  /**
   * @brief Returns the DataObject ID at the specified position in the path.
   * This method does not perform bounds checking.
   * @param index
   * @return DataObject::IdType
   */
  DataObject::IdType getIdAt(size_t index) const;

  /**
   * @brief Returns a pointer to the const DataObject targetted by the path.
   * @return const DataObject*
   */
  const DataObject* getData() const;

  /**
   * @brief Returns a pointer to the const DataObject at the specified path index.
   * @param index
   * @return const DataObject*
   */
  const DataObject* getDataAt(size_t index) const;

  /**
   * @brief Returns the name of the target DataObject. Throws an exception if
   * the DataObject does not exist.
   * @return std::string
   */
  std::string getName() const;

  /**
   * @brief Returns the name of the DataObject pointed to by the target position
   * of the path. Returns "[ missing ]" if the DataObject could not be found.
   * @param index
   * @return std::string
   */
  std::string getNameAt(size_t index) const;

  /**
   * @brief Returns a string representation of the path using the provided divider
   * between DataObject names. If no divider is provided, " / " is used instead.
   *
   * Names are provided using getNameAt(size_t).
   * @param div
   * @return std::string
   */
  std::string toString(const std::string& div = " / ") const;

  /**
   * @brief Checks equality with the specified LinkedPath.
   * @param rhs
   * @return bool
   */
  bool operator==(const LinkedPath& rhs) const;

  /**
   * @brief Checks equality with the specified DataPath.
   * @param rhs
   * @return bool
   */
  bool operator==(const DataPath& rhs) const;

  /**
   * @brief Checks inequality with the specified LinkedPath.
   * @param rhs
   * @return
   */
  bool operator!=(const LinkedPath& rhs) const;

  /**
   * @brief Checks inequality with the specified DataPath.
   * @param rhs
   * @return
   */
  bool operator!=(const DataPath& rhs) const;

protected:
  /**
   * @brief Constructs a LinkedPath for the target DataStructure and vector of DataObject IDs.
   * @param ds
   * @param idPath
   */
  LinkedPath(const DataStructure* ds, const std::vector<DataObject::IdType>& idPath);

private:
  const DataStructure* m_DataStructure = nullptr;
  std::vector<DataObject::IdType> m_IdPath;
};
} // namespace Complex
