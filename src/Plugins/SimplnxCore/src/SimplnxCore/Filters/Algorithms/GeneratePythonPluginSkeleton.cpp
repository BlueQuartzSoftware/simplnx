#include "GeneratePythonPluginSkeleton.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <Eigen/Dense>

using namespace nx::core;

// -----------------------------------------------------------------------------
GeneratePythonPluginSkeleton::GeneratePythonPluginSkeleton(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           GeneratePythonPluginSkeletonInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GeneratePythonPluginSkeleton::~GeneratePythonPluginSkeleton() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GeneratePythonPluginSkeleton::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GeneratePythonPluginSkeleton::operator()()
{

  auto result = nx::core::WritePythonPluginFiles(m_InputValues->pluginOutputDir, m_InputValues->pluginName, m_InputValues->pluginName, "Description", m_InputValues->filterNames);

  return {};
}
