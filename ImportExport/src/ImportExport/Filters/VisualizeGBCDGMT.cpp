#include "VisualizeGBCDGMT.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string VisualizeGBCDGMT::name() const
{
  return FilterTraits<VisualizeGBCDGMT>::name.str();
}

//------------------------------------------------------------------------------
std::string VisualizeGBCDGMT::className() const
{
  return FilterTraits<VisualizeGBCDGMT>::className;
}

//------------------------------------------------------------------------------
Uuid VisualizeGBCDGMT::uuid() const
{
  return FilterTraits<VisualizeGBCDGMT>::uuid;
}

//------------------------------------------------------------------------------
std::string VisualizeGBCDGMT::humanName() const
{
  return "Export GBCD Pole Figure (GMT 5)";
}

//------------------------------------------------------------------------------
std::vector<std::string> VisualizeGBCDGMT::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters VisualizeGBCDGMT::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_PhaseOfInterest_Key, "Phase of Interest", "", 1234356));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MisorientationRotation_Key, "Misorientation Axis-Angle", "", std::vector<float32>(4), std::vector<std::string>(4)));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output GMT File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insertSeparator(Parameters::Separator{"Face Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_GBCDArrayPath_Key, "GBCD", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer VisualizeGBCDGMT::clone() const
{
  return std::make_unique<VisualizeGBCDGMT>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult VisualizeGBCDGMT::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pPhaseOfInterestValue = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  auto pMisorientationRotationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pGBCDArrayPathValue = filterArgs.value<DataPath>(k_GBCDArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<VisualizeGBCDGMTAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> VisualizeGBCDGMT::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPhaseOfInterestValue = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  auto pMisorientationRotationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pGBCDArrayPathValue = filterArgs.value<DataPath>(k_GBCDArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
