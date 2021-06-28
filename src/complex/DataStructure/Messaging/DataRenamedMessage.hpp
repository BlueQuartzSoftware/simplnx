#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Messaging/AbstractDataStructureMessage.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * class DataRenamedMessage
 *
 */
class COMPLEX_EXPORT DataRenamedMessage : virtual public AbstractDataStructureMessage
{
public:
  static const int MsgType = 3;

  /**
   * @brief
   * @param ds
   * @param id
   * @param oldName
   * @param newName
   */
  DataRenamedMessage(const DataStructure* ds, DataObject::IdType id, const std::string& oldName, const std::string& newName);

  /**
   * @brief
   * @param other
   */
  DataRenamedMessage(const DataRenamedMessage& other);

  /**
   * @brief
   * @param other
   */
  DataRenamedMessage(DataRenamedMessage&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataRenamedMessage();

  /**
   * @brief
   * @return
   */
  int32_t getMsgType() const override;

  /**
   * @brief
   * @return IdType
   */
  DataObject::IdType getDataId() const;

  /**
   * @brief
   * @return DataObject*
   */
  const DataObject* getData() const;

  /**
   * @brief
   * @return std::string
   */
  std::string getOldName() const;

  /**
   * @brief
   * @return std::string
   */
  std::string getNewName() const;

protected:
private:
  DataObject::IdType m_Id;
  std::string m_OldName;
  std::string m_NewName;
};
} // namespace complex
