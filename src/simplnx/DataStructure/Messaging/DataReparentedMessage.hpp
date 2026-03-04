#pragma once

#include "simplnx/DataStructure/AbstractDataObject.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Messaging/AbstractDataStructureMessage.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{

/**
 * @class DataReparentedMessage
 * @brief The DataReparentedMessage class is a type of DataStructure message
 * emitted when a AbstractDataObject gains or loses a parent object. The message
 * includes the target object's ID, the target parent's ID, and whether or not
 * the parent was added or removed.
 */
class SIMPLNX_EXPORT DataReparentedMessage : public AbstractDataStructureMessage
{
public:
  static const MessageType MsgType = 4;

  /**
   * @brief Constructs a DataReparentedMessage, specifying the target AbstractDataObject,
   * parent ID, and whether or not the parent was added or removed.
   * @param dataStructure
   * @param targetData
   * @param targetParent
   * @param parentAdded
   */
  DataReparentedMessage(const DataStructure* dataStructure, AbstractDataObject::IdType targetData, AbstractDataObject::IdType targetParent, bool parentAdded = true);

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

  ~DataReparentedMessage() override;

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return MessageType
   */
  MessageType getMsgType() const override;

  /**
   * @brief Returns the target AbstractDataObject ID.
   * @return IdType
   */
  AbstractDataObject::IdType getTargetId() const;

  /**
   * @brief Returns the parent AbstractDataObject ID.
   * @return IdType
   */
  AbstractDataObject::IdType getParentId() const;

  /**
   * @brief Returns a read-only pointer to the target AbstractDataObject.
   * @return AbstractDataObject*
   */
  const AbstractDataObject* getTargetData() const;

  /**
   * @brief Returns a read-only pointer to the target parent.
   * @return AbstractDataObject*
   */
  const AbstractDataObject* getParentData() const;

  /**
   * @brief Returns true if the target parent was added to the AbstractDataObject.
   * @return bool
   */
  bool wasParentAdded() const;

  /**
   * @brief Returns true if the target parent was removed from the AbstractDataObject.
   * @return bool
   */
  bool wasParentRemoved() const;

protected:
private:
  AbstractDataObject::IdType m_TargetId;
  AbstractDataObject::IdType m_ParentId;
  bool m_ParentAdded = true;
};
} // namespace nx::core
