#include "DataStoreUtilities.hpp"

#include "simplnx/Core/Application.hpp"

using namespace nx::core;

//-----------------------------------------------------------------------------
DataIOCollection& DataStoreUtilities::GetIOCollection()
{
  return Application::GetOrCreateInstance()->getIOCollection();
}
