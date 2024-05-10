#include "CreatePythonSkeleton.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
CreatePythonSkeleton::CreatePythonSkeleton(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreatePythonSkeletonInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreatePythonSkeleton::~CreatePythonSkeleton() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CreatePythonSkeleton::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CreatePythonSkeleton::operator()()
{
  if(m_InputValues->useExistingPlugin)
  {
    return nx::core::WritePythonFiltersToPlugin(m_InputValues->pluginInputDir, m_InputValues->filterNames);
  }
  else
  {
    return nx::core::WritePythonPluginFiles(m_InputValues->pluginOutputDir, m_InputValues->pluginName, m_InputValues->pluginName, "Description", m_InputValues->filterNames);
  }
}
