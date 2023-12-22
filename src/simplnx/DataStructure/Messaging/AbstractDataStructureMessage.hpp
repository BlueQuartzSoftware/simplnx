#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
class DataStructure;

/**
 * @class AbstractDataStructureMessage
 * @brief The AbstractDataStructureMessage class is the base class for all
 * DataStructure messages emitted to observers.
 */
class SIMPLNX_EXPORT AbstractDataStructureMessage
{
public:
  using MessageType = int32;

  /**
   * @brief Copy constructor
   * @param other
   */
  AbstractDataStructureMessage(const AbstractDataStructureMessage& other);

  /**
   * @brief Move constructor
   * @param other
   */
  AbstractDataStructureMessage(AbstractDataStructureMessage&& other) noexcept;

  virtual ~AbstractDataStructureMessage();

  /**
   * @brief Returns a pointer to the DataStructure the message originated from.
   * @return const DataStructure*
   */
  const DataStructure* getDataStructure() const;

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return MessageType
   */
  virtual MessageType getMsgType() const = 0;

protected:
  /**
   * @brief Creates an AbstractDataStructureMessage for the target DataStructure.
   * @param dataStructure
   */
  AbstractDataStructureMessage(const DataStructure* dataStructure);

private:
  const DataStructure* m_DataStructure = nullptr;
};
} // namespace nx::core
