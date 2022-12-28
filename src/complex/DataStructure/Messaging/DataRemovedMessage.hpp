#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Messaging/AbstractDataStructureMessage.hpp"

#include "complex/complex_export.hpp"

namespace complex
{

/**
 * @class DataRemovedMessage
 * @brief The DataRemovedMessage class is a DataStructure message class that
 * notifies observers that a DataObject has been removed. The message includes
 * DataObject's ID and name at the moment of deletion. DataPaths are not
 * available because parent information is not available when a DataObject is
 * being deleted.
 */
class COMPLEX_EXPORT DataRemovedMessage : public AbstractDataStructureMessage
{
public:
  static const MessageType MsgType = 2;

  /**
   * @brief Constructs a DataRemovedMessage for the target DataStructure marking a
   * DataObject ID as being removed. The DataObject's name is also provided to
   * describe the object in a human-readable format.
   * @param dataGraph
   * @param id
   * @param name
   */
  DataRemovedMessage(const DataStructure* dataGraph, DataObject::IdType id, const std::string& name);

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

  virtual ~DataRemovedMessage();

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return MessageType
   */
  MessageType getMsgType() const override;

  /**
   * @brief Returns the removed DataObject's ID.
   * @return IdType
   */
  DataObject::IdType getId() const;

  /**
   * @brief Returns the name of the removed DataObject.
   * @return std::string
   */
  std::string getName() const;

protected:
private:
  std::string m_Name;
  DataObject::IdType m_Id;
};
} // namespace complex
