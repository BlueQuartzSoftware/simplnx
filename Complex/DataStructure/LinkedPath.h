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
   * @brief
   */
  LinkedPath();

  /**
   * @brief
   * @param rhs
   */
  LinkedPath(const LinkedPath& rhs);

  /**
   * @brief
   * @param rhs
   */
  LinkedPath(LinkedPath&& rhs) noexcept;

  /**
   * @brief
   */
  virtual ~LinkedPath();

  /**
   * @brief
   * @return bool
   */
  bool isValid() const;

  /**
   * @brief
   * @return DataStructure*
   */
  const DataStructure* getDataStructure() const;

  /**
   * @brief
   * @return DataPath
   */
  DataPath toDataPath() const;

  /**
   * @brief
   * @return size_t
   */
  size_t getLength() const;

  /**
   * @brief
   * @param index
   * @return
   */
  DataObject::IdType operator[](size_t index) const;

  /**
   * @brief
   * @return DataObject::IdType
   */
  DataObject::IdType getId() const;

  /**
   * @brief
   * @param index
   * @return IdType
   * @param  index
   */
  DataObject::IdType getIdAt(size_t index) const;

  /**
   * @brief
   * @return const DataObject*
   */
  const DataObject* getData() const;

  /**
   * @brief
   * @param index
   * @return const DataObject*
   */
  const DataObject* getDataAt(size_t index) const;

  /**
   * @brief
   * @return std::string
   */
  std::string getName() const;

  /**
   * @brief
   * @param index
   * @return std::string
   * @param  index
   */
  std::string getNameAt(size_t index) const;

  /**
   * @brief
   * @param div
   * @return std::string
   */
  std::string toString(const std::string& div = " / ") const;

  /**
   * @brief
   * @param rhs
   * @return
   */
  bool operator==(const LinkedPath& rhs) const;

  /**
   * @brief
   * @param rhs
   * @return
   */
  bool operator==(const DataPath& rhs) const;

  /**
   * @brief
   * @param rhs
   * @return
   */
  bool operator!=(const LinkedPath& rhs) const;

  /**
   * @brief
   * @param rhs
   * @return
   */
  bool operator!=(const DataPath& rhs) const;

protected:
  /**
   * @brief
   * @param ds
   * @param idPath
   */
  LinkedPath(const DataStructure* ds, const std::vector<DataObject::IdType>& idPath);

private:
  const DataStructure* m_DataStructure = nullptr;
  std::vector<DataObject::IdType> m_IdPath;
};
} // namespace SIMPL
