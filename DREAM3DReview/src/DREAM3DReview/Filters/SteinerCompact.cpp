#include "SteinerCompact.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string SteinerCompact::name() const
{
  return FilterTraits<SteinerCompact>::name.str();
}

Uuid SteinerCompact::uuid() const
{
  return FilterTraits<SteinerCompact>::uuid;
}

std::string SteinerCompact::humanName() const
{
  return "Steiner Compact";
}

Parameters SteinerCompact::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_Plane_Key, "Section Plane", "", 0, ChoicesParameter::Choices{"XY", "XZ", "YZ"}));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_Sites_Key, "Number Of Sites", "", 0, ChoicesParameter::Choices{"8", "12", "16", "24", "36"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_VtkOutput_Key, "Graphical Output As .vtk", "", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_VtkFileName_Key, "Output Vtk File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_TxtOutput_Key, "Text Output As .txt", "", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_TxtFileName_Key, "Output Text File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_VtkOutput_Key, k_VtkFileName_Key, true);
  params.linkParameters(k_TxtOutput_Key, k_TxtFileName_Key, true);

  return params;
}

IFilter::UniquePointer SteinerCompact::clone() const
{
  return std::make_unique<SteinerCompact>();
}

Result<OutputActions> SteinerCompact::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pSitesValue = filterArgs.value<ChoicesParameter::ValueType>(k_Sites_Key);
  auto pVtkOutputValue = filterArgs.value<bool>(k_VtkOutput_Key);
  auto pVtkFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_VtkFileName_Key);
  auto pTxtOutputValue = filterArgs.value<bool>(k_TxtOutput_Key);
  auto pTxtFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_TxtFileName_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<SteinerCompactAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> SteinerCompact::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pSitesValue = filterArgs.value<ChoicesParameter::ValueType>(k_Sites_Key);
  auto pVtkOutputValue = filterArgs.value<bool>(k_VtkOutput_Key);
  auto pVtkFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_VtkFileName_Key);
  auto pTxtOutputValue = filterArgs.value<bool>(k_TxtOutput_Key);
  auto pTxtFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_TxtFileName_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
