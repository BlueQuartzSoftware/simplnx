#pragma once

#include <vector>

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Messaging/AbstractDataStructureMessage.hpp"

#include "complex/complex_export.hpp"

namespace complex
{

/**
 * @class DataAddedMessage
 * @brief The DataAddedMessage class is a DataStructure message class for
 * notifying observers to the addition of a DataObject to the DataStructure.
 * The message can be used to retrieve the DataObject in question and all
 * DataPaths to the created object.
 */
class COMPLEX_EXPORT DataAddedMessage : public AbstractDataStructureMessage
{
public:
  static const MessageType MsgType = 1;

  /**
   * @brief Creates a DataAddedMessage for the target DataStructure and DataObject ID.
   * @param ds
   * @param addedId
   */
  DataAddedMessage(const DataStructure* ds, DataObject::IdType addedId);

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

  virtual ~DataAddedMessage();

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return MessageType
   */
  MessageType getMsgType() const override;

  /**
   * @brief Returns the added DataObject ID.
   * @return IdType
   */
  DataObject::IdType getId() const;

  /**
   * @brief Returns a read-only pointer to the added DataObject.
   * @return DataObject*
   */
  const DataObject* getData() const;

  /**
   * @brief Returns all DataPaths to the added DataObject.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getDataPaths() const;

protected:
private:
  DataObject::IdType m_Id;
};
} // namespace complex
