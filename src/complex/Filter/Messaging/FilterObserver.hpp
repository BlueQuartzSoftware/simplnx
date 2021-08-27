#pragma once

#include <memory>
#include <vector>

#include "complex/Filter/Messaging/FilterMessage.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class IFilter;

class COMPLEX_EXPORT FilterObserver
{
  friend class IFilter;

public:
  /**
   * @brief Constructs a new FilterObserver.
   */
  FilterObserver();

  virtual ~FilterObserver();

  /**
   * @brief Returns a vector of all observed filters.
   * @return std::vector<IFilter*>
   */
  std::vector<IFilter*> getObservedFilters() const;

  /**
   * @brief Returns true if the specified filter is being observed. Returns
   * false otherwise.
   * @param filter
   * @return bool
   */
  bool isObservingFilter(IFilter* filter) const;

  /**
   * @brief Starts observing the specified filter. This function does nothing
   * if the target filter is already being observed.
   * @param filter
   */
  void startObservingFilter(IFilter* filter);

  /**
   * @brief Stops observing the specified filter. This function does nothing
   * if the target filter is not being observed.
   * @param filter
   */
  void stopObservingFilter(IFilter* filter);

protected:
  /**
   * @brief 
   * @param filter
   * @param msg
   */
  virtual void onNotify(IFilter* filter, const std::shared_ptr<FilterMessage>& msg) = 0;

private:
  std::vector<IFilter*> m_ObservedFilters;
};
} // namespace complex
