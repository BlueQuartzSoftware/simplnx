#pragma once

#include <vector>

#include "Complex/DataStructure/DataObject.h"
#include "Complex/DataStructure/DataPath.h"
#include "Complex/DataStructure/Messaging/AbstractDataStructureMessage.h"

namespace Complex
{

/**
 * class DataAddedMessage
 *
 */

class DataAddedMessage : virtual public AbstractDataStructureMessage
{
public:
  static const int MsgType = 1;

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
  DataAddedMessage::DataAddedMessage(const DataAddedMessage& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataAddedMessage::DataAddedMessage(DataAddedMessage&& other) noexcept;

  virtual ~DataAddedMessage();

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return int32_t
   */
  int32_t getMsgType() const override;

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
} // namespace Complex
