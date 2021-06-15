#pragma once

#include "Complex/DataStructure/DataObject.h"
#include "Complex/DataStructure/DataPath.h"
#include "Complex/DataStructure/Messaging/AbstractDataStructureMessage.h"

namespace Complex
{

/**
 * class DataRemovedMessage
 *
 */

class DataRemovedMessage : virtual public AbstractDataStructureMessage
{
public:
  static const int MsgType = 2;

  /**
   * @brief Constructs a DataRemovedMessage for the target DataStructure marking a
   * DataObject ID as being removed. The DataObject's name is also provided to
   * describe the object in a human-readable format.
   * @param ds
   * @param id
   * @param name
   */
  DataRemovedMessage(const DataStructure* ds, DataObject::IdType id, const std::string& name);

  /**
   * @brief Copy constructor
   * @param other
   */
  DataRemovedMessage(const DataRemovedMessage& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataRemovedMessage(DataRemovedMessage&& other) noexcept;

  virtual ~DataRemovedMessage();

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return int32_t
   */
  int32_t getMsgType() const override;

  /**
   * @brief Returns the removed DataObject's ID
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
} // namespace Complex
