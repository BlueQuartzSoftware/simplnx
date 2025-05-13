#include "DataStoreUtilities.hpp"

#include "simplnx/Core/Application.hpp"

using namespace nx::core;

//-----------------------------------------------------------------------------
void DataStoreUtilities::TryForceLargeDataFormatFromPrefs(std::string& dataFormat)
{
  auto* preferencesPtr = Application::GetOrCreateInstance()->getPreferences();
  if(preferencesPtr->forceOocData())
  {
    dataFormat = preferencesPtr->largeDataFormat();
  }
}

//-----------------------------------------------------------------------------
std::shared_ptr<DataIOCollection> DataStoreUtilities::GetIOCollection()
{
  return Application::GetOrCreateInstance()->getIOCollection();
}
