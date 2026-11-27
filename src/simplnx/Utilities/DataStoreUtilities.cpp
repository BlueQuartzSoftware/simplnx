#include "DataStoreUtilities.hpp"

#include "simplnx/Core/Application.hpp"

using namespace nx::core;

DataIOCollection& data_store_utilities::GetIOCollection()
{
  return Application::GetOrCreateInstance()->getIOCollection();
}
