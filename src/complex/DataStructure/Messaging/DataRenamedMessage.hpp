#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Messaging/AbstractDataStructureMessage.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class DataRenamedMessage
 * @brief The DataRenamedMessage class is a DataStructure message class used
 * to signal that a DataObject's name was changed. The message includes the
 * target DataObject's ID, the previous name, and the name it was changed to.
 */
class COMPLEX_EXPORT DataRenamedMessage : public AbstractDataStructureMessage
{
public:
  static const MessageType MsgType = 3;

  /**
   * @brief Constructs a DataRenamedMessage specifying which DataObject was
   * renamed, its old name, and new name.
   * @param ds
   * @param dataId
   * @param prevName
   * @param newName
   */
  DataRenamedMessage(const DataStructure* ds, DataObject::IdType dataId, const std::string& prevName, const std::string& newName);

  /**
   * @brief Creates a copy of the target DataRenamedMessage.
   * @param other
   */
  DataRenamedMessage(const DataRenamedMessage& other);

  /**
   * @brief Creates a new DataRenamedMessage and moves values from the target.
   * @param other
   */
  DataRenamedMessage(DataRenamedMessage&& other) noexcept;

  ~DataRenamedMessage() override;

  /**
   * @brief Returns the AbstractDataStructureMessage type.
   * @return MessageType
   */
  MessageType getMsgType() const override;

  /**
   * @brief Returns the renamed object's ID.
   * @return IdType
   */
  DataObject::IdType getDataId() const;

  /**
   * @brief Returns a const pointer to the renamed DataObject.
   * @return DataObject*
   */
  const DataObject* getData() const;

  /**
   * @brief Returns the DataObject's previous name.
   * @return std::string
   */
  std::string getPreviousName() const;

  /**
   * @brief Returns the DataObject's new name.
   * @return std::string
   */
  std::string getNewName() const;

private:
  DataObject::IdType m_Id;
  std::string m_OldName;
  std::string m_NewName;
};
} // namespace complex
