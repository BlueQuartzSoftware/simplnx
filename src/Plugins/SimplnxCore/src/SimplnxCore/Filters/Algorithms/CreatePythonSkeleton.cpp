#include "CreatePythonSkeleton.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

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

// -----------------------------------------------------------------------------
const std::atomic_bool& CreatePythonSkeleton::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CreatePythonSkeleton::operator()()
{
  if(m_InputValues->use_existing_plugin)
  {
    return nx::core::WritePythonFiltersToPlugin(m_InputValues->plugin_input_dir, m_InputValues->filter_names);
  }

  return nx::core::WritePythonPluginFiles(m_InputValues->plugin_output_dir, m_InputValues->plugin_name, m_InputValues->plugin_human_name, "Description", m_InputValues->filter_names);
}
