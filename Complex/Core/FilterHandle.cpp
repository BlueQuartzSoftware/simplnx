#include "Complex/Core/FilterHandle.h"

using namespace Complex;

// Constructors/Destructors
//

FilterHandle::FilterHandle()
{
}

FilterHandle::~FilterHandle() = default;

//
// Methods
//

// Accessor methods
//

// Other methods
//

bool FilterHandle::operator < (const FilterHandle& rhs) const noexcept
{
  if(getPluginId() < rhs.getPluginId())
  {
    return true;
  }
  else if(getPluginId() > rhs.getPluginId())
  {
    return false;
  }
  else
  {
    return getFilterId() < rhs.getFilterId();
  }
}

bool operator==(const Complex::FilterHandle& lhs, const Complex::FilterHandle& rhs) noexcept
{
  if(lhs.getPluginId() != rhs.getPluginId())
  {
    return false;
  }
  else
  {
    return lhs.getFilterId() == rhs.getFilterId();
  }
}
