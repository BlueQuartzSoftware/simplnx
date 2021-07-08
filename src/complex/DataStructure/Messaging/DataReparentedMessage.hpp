#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Messaging/AbstractDataStructureMessage.hpp"

#include "complex/complex_export.hpp"

namespace complex
{

/**
 * @class DataReparentedMessage
 * @brief The DataReparentedMessage class is a type of DataStructure message
 * emitted when a DataObject gains or loses a parent object. The message
 * includes the target object's ID, the target parent's ID, and whether or not
 * the parent was added or removed.
 */
class COMPLEX_EXPORT DataReparentedMessage : public AbstractDataStructureMessage
{
public:
  static const MessageType MsgType = 4;

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
   * @return MessageType
   */
  MessageType getMsgType() const override;

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
} // namespace complex
