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
   * @brief Default constructor
   */
  AbstractDataStructureObserver();

  /**
   * @brief Copy constructor
   * @param other
   */
  AbstractDataStructureObserver(const AbstractDataStructureObserver& other);

  /**
   * @brief Move constructor
   * @param other
   */
  AbstractDataStructureObserver(AbstractDataStructureObserver&& other) noexcept;

  virtual ~AbstractDataStructureObserver();

  /**
   * @brief Called when the target DataStructure emits a message.
   * @param target
   * @param msg
   */
  virtual void onNotify(DataStructure* target, const std::shared_ptr<AbstractDataStructureMessage>& msg) = 0;
};
} // namespace complex
