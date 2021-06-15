#include "AbstractPlugin.h"

//#include "Complex/Filtering/AbstractFilter.h"

using namespace Complex;


AbstractPlugin::~AbstractPlugin() = default;


/**
 * Add a M_FilterHandles object to the m_m_filterhandlesVector List
 */
void AbstractPlugin::addFilterHandle(const FilterHandle& addHandle)
{
  m_FilterHandles.insert(addHandle);
}

/**
 * Get the list of FilterHandles objects held by m_FilterHandles
 * @return std::set<FilterHandle> of FilterHandles objects held by
 * m_FilterHandles.
 */
std::set<FilterHandle> AbstractPlugin::getFilterHandlesList() const
{
  return m_FilterHandles;
}

// Other methods
//
