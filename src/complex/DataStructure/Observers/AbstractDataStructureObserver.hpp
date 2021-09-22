#pragma once

#include <memory>

#include "nod/nod.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractDataStructureMessage;
class DataStructure;

/**
 * @class AbstractDataStructureObserver
 * @brief The AbstractDataStructureObserver class is a base class for objects
 * that are interested in receiving message notifications from at least one
 * DataStructure. Concrete instances of AbstractDataStructureObserver should
 * provide an implementation of the onNotify method.
 */
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
  AbstractDataStructureObserver(const AbstractDataStructureObserver& other) = delete;

  /**
   * @brief Move constructor
   * @param other
   */
  AbstractDataStructureObserver(AbstractDataStructureObserver&& other) noexcept = delete;

  virtual ~AbstractDataStructureObserver();

  /**
   * @brief Returns a pointer to the observed DataStructure.
   * @return DataStructure*
   */
  DataStructure* getObservedStructure() const;

  /**
   * @brief Returns true if the a DataStructure is being observed. Returns
   * false otherwise.
   * @return bool
   */
  bool isObservingStructure() const;

  /**
   * @brief Starts observing the specified DataStructure.
   * Only one DataStructure can be observed at a time.
   * @param ds
   */
  void startObservingStructure(DataStructure* ds);

  /**
   * @brief Stops observing the current DataStructure.
   */
  void stopObservingStructure();

  /**
   * @brief Called when the target DataStructure emits a message.
   * @param target
   * @param msg
   */
  virtual void onNotify(DataStructure* target, const std::shared_ptr<AbstractDataStructureMessage>& msg) = 0;

private:
  DataStructure* m_ObservedStructure = nullptr;
  nod::connection m_Connection;
};
} // namespace complex
