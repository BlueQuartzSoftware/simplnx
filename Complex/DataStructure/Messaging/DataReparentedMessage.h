#pragma once

#include "Complex/DataStructure/DataObject.h"
#include "Complex/DataStructure/DataPath.h"
#include "Complex/DataStructure/Messaging/AbstractDataStructureMessage.h"

namespace Complex
{

/**
 * class DataReparentedMessage
 *
 */

class DataReparentedMessage : public AbstractDataStructureMessage
{
public:
  static const int MsgType = 4;

  /**
   * @brief
   * @param ds
   * @param targetData
   * @param targetParent
   * @param parentAdded
   */
  DataReparentedMessage(const DataStructure* ds, DataObject::IdType targetData, DataObject::IdType targetParent, bool parentAdded = true);

  /**
   * @brief
   * @param other
   */
  DataReparentedMessage(const DataReparentedMessage& other);

  /**
   * @brief
   * @param other
   */
  DataReparentedMessage(DataReparentedMessage&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataReparentedMessage();

  /**
   * @brief
   * @return
   */
  int32_t getMsgType() const override;

  /**
   * @brief
   * @return IdType
   */
  DataObject::IdType getTargetId() const;

  /**
   * @brief
   * @return IdType
   */
  DataObject::IdType getParentId() const;

  /**
   * @brief
   * @return DataObject*
   */
  const DataObject* getTargetData() const;

  /**
   * @brief
   * @return DataObject*
   */
  const DataObject* getParentData() const;

  /**
   * @brief
   * @return bool
   */
  bool wasParentAdded() const;

  /**
   * @brief
   * @return bool
   */
  bool wasParentRemoved() const;

protected:
private:
  DataObject::IdType m_TargetId;
  DataObject::IdType m_ParentId;
  bool m_ParentAdded = true;
};
} // namespace SIMPL
