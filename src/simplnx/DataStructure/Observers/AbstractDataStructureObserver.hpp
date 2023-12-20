#pragma once

#include "simplnx/simplnx_export.hpp"

#include <nod/nod.hpp>

#include <memory>

namespace nx::core
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
class SIMPLNX_EXPORT AbstractDataStructureObserver
{
public:
  /**
   * @brief Default constructor
   */
  AbstractDataStructureObserver();

  /**
   * @brief Copy constructor not defined.
   * @param other
   */
  AbstractDataStructureObserver(const AbstractDataStructureObserver& other) = delete;

  /**
   * @brief Move constructor not defined.
   * @param other
   */
  AbstractDataStructureObserver(AbstractDataStructureObserver&& other) noexcept = delete;

  /**
   * @brief Stops observing the current DataStructure, if there is one, and
   * closes the connection.
   */
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
   * Only one DataStructure can be observed at a time. This method replaces the
   * existing target, if there is one, with the new target.
   * @param dataStructure
   */
  void startObservingStructure(DataStructure* dataStructure);

  /**
   * @brief Stops observing the current DataStructure and resets the observed
   * DataStructure pointer.
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
} // namespace nx::core
