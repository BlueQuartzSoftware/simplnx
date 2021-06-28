#pragma once

#include <memory>

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractDataStructureMessage;
class DataStructure;

class COMPLEX_EXPORT AbstractDataStructureObserver
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
} // namespace complex
