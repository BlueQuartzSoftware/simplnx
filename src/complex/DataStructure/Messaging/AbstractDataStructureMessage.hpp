#pragma once

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class DataStructure;

/**
 * @class AbstractDataStructureMessage
 * @brief The AbstractDataStructureMessage class is the base class for all
 * DataStructure messages emitted to observers.
 */
class COMPLEX_EXPORT AbstractDataStructureMessage
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
} // namespace complex
