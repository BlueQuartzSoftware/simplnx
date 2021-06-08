#pragma once

#include <memory>

namespace Complex
{
class AbstractDataStructureMessage;
class DataStructure;

class AbstractDataStructureObserver
{
public:
  /**
   * @brief
   */
  AbstractDataStructureObserver();

  /**
   * @brief
   * @param other
   */
  AbstractDataStructureObserver(const AbstractDataStructureObserver& other);

  /**
   * @brief
   * @param other
   */
  AbstractDataStructureObserver(AbstractDataStructureObserver&& other) noexcept;

  ~AbstractDataStructureObserver();

  /**
   * @brief
   * @param target
   * @param msg
   */
  virtual void onNotify(DataStructure* target, const std::shared_ptr<AbstractDataStructureMessage>& msg) = 0;
};
} // namespace SIMPL
