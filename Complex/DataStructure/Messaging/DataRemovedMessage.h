#pragma once

#include "SIMPL/DataStructure/DataObject.h"
#include "SIMPL/DataStructure/DataPath.h"
#include "SIMPL/DataStructure/Messaging/AbstractDataStructureMessage.h"

namespace SIMPL
{

/**
 * class DataRemovedMessage
 *
 */

class DataRemovedMessage : virtual public AbstractDataStructureMessage
{
public:
  static const int MsgType = 2;

  /**
   * @brief
   * @param ds
   * @param id
   * @param name
   */
  DataRemovedMessage(const DataStructure* ds, DataObject::IdType id, const std::string& name);

  /**
   * @brief
   * @param other
   */
  DataRemovedMessage(const DataRemovedMessage& other);

  /**
   * @brief
   * @param other
   */
  DataRemovedMessage(DataRemovedMessage&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataRemovedMessage();

  /**
   * @brief
   * @return
   */
  int32_t getMsgType() const override;

  /**
   * @brief
   * @return IdType
   */
  DataObject::IdType getId() const;

  /**
   * @brief
   * @return std::string
   */
  std::string getName() const;

protected:
private:
  std::string m_Name;
  DataObject::IdType m_Id;
};
} // namespace SIMPL
