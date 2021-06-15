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
   * @brief Constructs a DataReparentedMessage, specifying the target DataObject,
   * parent ID, and whether or not the parent was added or removed.
   * @param ds
   * @param targetData
   * @param targetParent
   * @param parentAdded
   */
  DataReparentedMessage(const DataStructure* ds, DataObject::IdType targetData, DataObject::IdType targetParent, bool parentAdded = true);

  /**
   * @brief Copy constructor
   * @param other
   */
  DataReparentedMessage(const DataReparentedMessage& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataReparentedMessage(DataReparentedMessage&& other) noexcept;

  virtual ~DataReparentedMessage();

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return
   */
  int32_t getMsgType() const override;

  /**
   * @brief Returns the target DataObject ID.
   * @return IdType
   */
  DataObject::IdType getTargetId() const;

  /**
   * @brief Returns the parent DataObject ID.
   * @return IdType
   */
  DataObject::IdType getParentId() const;

  /**
   * @brief Returns a read-only pointer to the target DataObject.
   * @return DataObject*
   */
  const DataObject* getTargetData() const;

  /**
   * @brief Returns a read-only pointer to the target parent.
   * @return DataObject*
   */
  const DataObject* getParentData() const;

  /**
   * @brief Returns true if the target parent was added to the DataObject.
   * @return bool
   */
  bool wasParentAdded() const;

  /**
   * @brief Returns true if the target parent was removed from the DataObject.
   * @return bool
   */
  bool wasParentRemoved() const;

protected:
private:
  DataObject::IdType m_TargetId;
  DataObject::IdType m_ParentId;
  bool m_ParentAdded = true;
};
} // namespace Complex
