#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Messaging/AbstractDataStructureMessage.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * class DataReparentedMessage
 *
 */
class COMPLEX_EXPORT DataReparentedMessage : public AbstractDataStructureMessage
{
public:
  static inline constexpr int MsgType = 4;

  /**
   * @brief
   * @param ds
   * @param targetData
   * @param targetParent
   * @param parentAdded
   */
  DataReparentedMessage(const DataStructure* ds, DataObject::IdType targetData, DataObject::IdType targetParent, bool parentAdded = true);

  /**
   * @brief
   * @param other
   */
  DataReparentedMessage(const DataReparentedMessage& other);

  /**
   * @brief
   * @param other
   */
  DataReparentedMessage(DataReparentedMessage&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataReparentedMessage();

  /**
   * @brief
   * @return
   */
  int32_t getMsgType() const override;

  /**
   * @brief
   * @return IdType
   */
  DataObject::IdType getTargetId() const;

  /**
   * @brief
   * @return IdType
   */
  DataObject::IdType getParentId() const;

  /**
   * @brief
   * @return DataObject*
   */
  const DataObject* getTargetData() const;

  /**
   * @brief
   * @return DataObject*
   */
  const DataObject* getParentData() const;

  /**
   * @brief
   * @return bool
   */
  bool wasParentAdded() const;

  /**
   * @brief
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
