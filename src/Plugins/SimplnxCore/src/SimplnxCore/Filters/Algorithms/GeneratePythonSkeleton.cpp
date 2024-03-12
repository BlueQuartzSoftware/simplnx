#include "GeneratePythonSkeleton.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
GeneratePythonSkeleton::GeneratePythonSkeleton(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               GeneratePythonSkeletonInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GeneratePythonSkeleton::~GeneratePythonSkeleton() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GeneratePythonSkeleton::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GeneratePythonSkeleton::operator()()
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
