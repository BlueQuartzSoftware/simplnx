#pragma once

#include <vector>

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Messaging/AbstractDataStructureMessage.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * class DataAddedMessage
 *
 */
class COMPLEX_EXPORT DataAddedMessage : virtual public AbstractDataStructureMessage
{
public:
  static const int MsgType = 1;

  /**
   * @brief
   * @param ds
   * @param addedId
   */
  DataAddedMessage(const DataStructure* ds, DataObject::IdType addedId);

  /**
   * @brief
   * @param other
   */
  DataAddedMessage(const DataAddedMessage& other);

  /**
   * @brief
   * @param other
   */
  DataAddedMessage(DataAddedMessage&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataAddedMessage();

  /**
   * @brief
   * @return
   */
  int32_t getMsgType() const override;

  /**
   * @brief
   * @return IdType
   */
  DataObject::IdType getId() const;

  /**
   * @brief
   * @return DataObject*
   */
  const DataObject* getData() const;

  /**
   * @brief
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getDataPaths() const;

protected:
private:
  DataObject::IdType m_Id;
};
} // namespace complex
