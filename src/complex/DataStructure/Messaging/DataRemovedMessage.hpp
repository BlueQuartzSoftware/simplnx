#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Messaging/AbstractDataStructureMessage.hpp"

#include "complex/complex_export.hpp"

namespace complex
{

/**
 * class DataRemovedMessage
 *
 */

class COMPLEX_EXPORT DataRemovedMessage : public AbstractDataStructureMessage
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
} // namespace complex
