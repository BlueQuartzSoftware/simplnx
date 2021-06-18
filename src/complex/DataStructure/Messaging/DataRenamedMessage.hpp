#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Messaging/AbstractDataStructureMessage.hpp"

#include "complex/complex_export.hpp"

namespace complex
{

/**
 * class DataRenamedMessage
 *
 */

class COMPLEX_EXPORT DataRenamedMessage : public AbstractDataStructureMessage
{
public:
  static const int MsgType = 3;

  /**
   * @brief Constructs a DataRenamedMessage specifying which DataObject was renamed, its old name, and new name.
   * @param ds
   * @param id
   * @param oldName
   * @param newName
   */
  DataRenamedMessage(const DataStructure* ds, DataObject::IdType id, const std::string& oldName, const std::string& newName);

  /**
   * @brief Copy constructor
   * @param other
   */
  DataRenamedMessage(const DataRenamedMessage& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataRenamedMessage(DataRenamedMessage&& other) noexcept;

  virtual ~DataRenamedMessage();

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return
   */
  int32_t getMsgType() const override;

  /**
   * @brief Returns the renamed object's ID
   * @return IdType
   */
  DataObject::IdType getDataId() const;

  /**
   * @brief Returns a read-only pointer to the renamed DataObject.
   * @return DataObject*
   */
  const DataObject* getData() const;

  /**
   * @brief Returns the previous DataObject name.
   * @return std::string
   */
  std::string getOldName() const;

  /**
   * @brief Returns the new DataObject name.
   * @return std::string
   */
  std::string getNewName() const;

protected:
private:
  DataObject::IdType m_Id;
  std::string m_OldName;
  std::string m_NewName;
};
} // namespace complex
