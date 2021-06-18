#pragma once

#include <cstdint>

#include "complex/complex_export.hpp"

namespace complex
{
class DataStructure;

class COMPLEX_EXPORT AbstractDataStructureMessage
{
public:
  /**
   * @brief Copy constructor
   * @param other
   */
  AbstractDataStructureMessage(const AbstractDataStructureMessage& other);

  /**
   * @brief Move constructor
   * @param other
   * @return
   */
  AbstractDataStructureMessage(AbstractDataStructureMessage&& other) noexcept;

  /**
   * @brief
   */
  virtual ~AbstractDataStructureMessage();

  /**
   * @brief
   * @return
   */
  const DataStructure* getDataStructure() const;

  /**
   * @brief Returns the AbsractDataStructureMessage type.
   * @return
   */
  virtual int32_t getMsgType() const = 0;

protected:
  /**
   * @brief Creates an AbstractDataStructureMessage for the target DataStructure.
   * @param ds
   */
  AbstractDataStructureMessage(const DataStructure* ds);

private:
  const DataStructure* m_DataStructure = nullptr;
};
} // namespace complex
