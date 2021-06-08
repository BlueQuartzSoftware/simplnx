#pragma once

#include <cstdint>

namespace Complex
{
class DataStructure;

class AbstractDataStructureMessage
{
public:
  /**
   * @brief
   * @param ds
   */
  AbstractDataStructureMessage(const DataStructure* ds);

  /**
   * @brief
   * @param other
   */
  AbstractDataStructureMessage(const AbstractDataStructureMessage& other);

  /**
   * @brief
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
   * @brief
   * @return
   */
  virtual int32_t getMsgType() const = 0;

private:
  const DataStructure* m_DataStructure = nullptr;
};
} // namespace SIMPL
