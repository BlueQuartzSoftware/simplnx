#pragma once

#include "simplnx/Common/SimplnxConstants.hpp"
#include "simplnx/DataStructure/DataObject.hpp"

#include <memory>
#include <vector>


namespace nx::core::TemplateHelpers
{
/**
 * @brief This class (functor) simply returns true or false if the IDataArray can be downcast to a certain DataArray type
 * parameterized by the template parameter T.
 */
template <class T>
struct CanDynamicCast
{
  bool operator()(const DataObject* p) const
  {
    return dynamic_cast<const T*>(p) != nullptr;
  }
};
} // namespace nx::core::TemplateHelpers

