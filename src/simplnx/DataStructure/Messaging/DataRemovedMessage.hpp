#pragma once

#include "simplnx/DataStructure/AbstractDataObject.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Messaging/AbstractDataStructureMessage.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{

/**
 * @class DataRemovedMessage
 * @brief The DataRemovedMessage class is a DataStructure message class that
 * notifies observers that a AbstractDataObject has been removed. The message includes
 * AbstractDataObject's ID and name at the moment of deletion. DataPaths are not
 * available because parent information is not available when a AbstractDataObject is
 * being deleted.
 */
class SIMPLNX_EXPORT DataRemovedMessage : public AbstractDataStructureMessage
{
public:
  static const MessageType MsgType = 2;

  /**
   * @brief Constructs a DataRemovedMessage for the target DataStructure marking a
   * AbstractDataObject ID as being removed. The AbstractDataObject's name is also provided to
   * describe the object in a human-readable format.
   * @param dataStructure
   * @param identifier
   * @param name
   */
  DataRemovedMessage(const DataStructure* dataStructure, AbstractDataObject::IdType identifier, const std::string& name);

  /**
   * @brief Creates a copy of the target DataRemovedMessage.
   * @param other
   */
  DataRemovedMessage(const DataRemovedMessage& other);

  /**
   * @brief Creates a new DataRemovedMessage and moves values from the target.
   * @param other
   */
  DataRemovedMessage(DataRemovedMessage&& other) noexcept;

  ~DataRemovedMessage() override;

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return MessageType
   */
  MessageType getMsgType() const override;

  /**
   * @brief Returns the removed AbstractDataObject's ID.
   * @return IdType
   */
  AbstractDataObject::IdType getId() const;

  /**
   * @brief Returns the name of the removed AbstractDataObject.
   * @return std::string
   */
  std::string getName() const;

protected:
private:
  std::string m_Name;
  AbstractDataObject::IdType m_Id;
};
} // namespace nx::core
