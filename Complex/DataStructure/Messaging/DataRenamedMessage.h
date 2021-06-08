#pragma once

#include "SIMPL/DataStructure/DataObject.h"
#include "SIMPL/DataStructure/DataPath.h"
#include "SIMPL/DataStructure/Messaging/AbstractDataStructureMessage.h"

namespace SIMPL
{

/**
 * class DataRenamedMessage
 *
 */

class DataRenamedMessage : virtual public AbstractDataStructureMessage
{
public:
  static const int MsgType = 3;

  /**
   * @brief
   * @param ds
   * @param id
   * @param oldName
   * @param newName
   */
  DataRenamedMessage(const DataStructure* ds, DataObject::IdType id, const std::string& oldName, const std::string& newName);

  /**
   * @brief
   * @param other
   */
  DataRenamedMessage(const DataRenamedMessage& other);

  /**
   * @brief
   * @param other
   */
  DataRenamedMessage(DataRenamedMessage&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataRenamedMessage();

  /**
   * @brief
   * @return
   */
  int32_t getMsgType() const override;

  /**
   * @brief
   * @return IdType
   */
  DataObject::IdType getDataId() const;

  /**
   * @brief
   * @return DataObject*
   */
  const DataObject* getData() const;

  /**
   * @brief
   * @return std::string
   */
  std::string getOldName() const;

  /**
   * @brief
   * @return std::string
   */
  std::string getNewName() const;

protected:
private:
  DataObject::IdType m_Id;
  std::string m_OldName;
  std::string m_NewName;
};
} // namespace SIMPL
