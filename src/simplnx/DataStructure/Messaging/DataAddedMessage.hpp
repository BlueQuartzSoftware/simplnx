#pragma once

#include <vector>

#include "simplnx/DataStructure/AbstractDataObject.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Messaging/AbstractDataStructureMessage.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{

/**
 * @class DataAddedMessage
 * @brief The DataAddedMessage class is a DataStructure message class for
 * notifying observers to the addition of a AbstractDataObject to the DataStructure.
 * The message can be used to retrieve the AbstractDataObject in question and all
 * DataPaths to the created object.
 */
class SIMPLNX_EXPORT DataAddedMessage : public AbstractDataStructureMessage
{
public:
  static const MessageType MsgType = 1;

  /**
   * @brief Creates a DataAddedMessage for the target DataStructure and AbstractDataObject ID.
   * @param dataStructure
   * @param addedId
   */
  DataAddedMessage(const DataStructure* dataStructure, AbstractDataObject::IdType addedId);

  /**
   * @brief Copy constructor
   * @param other
   */
  DataAddedMessage(const DataAddedMessage& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataAddedMessage(DataAddedMessage&& other) noexcept;

  ~DataAddedMessage() override;

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return MessageType
   */
  MessageType getMsgType() const override;

  /**
   * @brief Returns the added AbstractDataObject ID.
   * @return IdType
   */
  AbstractDataObject::IdType getId() const;

  /**
   * @brief Returns a read-only pointer to the added AbstractDataObject.
   * @return AbstractDataObject*
   */
  const AbstractDataObject* getData() const;

  /**
   * @brief Returns all DataPaths to the added AbstractDataObject.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getDataPaths() const;

protected:
private:
  AbstractDataObject::IdType m_Id;
};
} // namespace nx::core
