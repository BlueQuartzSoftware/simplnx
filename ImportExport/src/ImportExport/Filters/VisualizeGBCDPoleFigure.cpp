#include "VisualizeGBCDPoleFigure.hpp"

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
std::string VisualizeGBCDPoleFigure::name() const
{
  return FilterTraits<VisualizeGBCDPoleFigure>::name.str();
}

//------------------------------------------------------------------------------
std::string VisualizeGBCDPoleFigure::className() const
{
  return FilterTraits<VisualizeGBCDPoleFigure>::className;
}

//------------------------------------------------------------------------------
Uuid VisualizeGBCDPoleFigure::uuid() const
{
  return FilterTraits<VisualizeGBCDPoleFigure>::uuid;
}

//------------------------------------------------------------------------------
std::string VisualizeGBCDPoleFigure::humanName() const
{
  return "Export GBCD Pole Figure (VTK)";
}

//------------------------------------------------------------------------------
std::vector<std::string> VisualizeGBCDPoleFigure::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters VisualizeGBCDPoleFigure::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_PhaseOfInterest_Key, "Phase of Interest", "", 1234356));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MisorientationRotation_Key, "Misorientation Axis-Angle", "", std::vector<float32>(4), std::vector<std::string>(4)));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output Regular Grid VTK File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insertSeparator(Parameters::Separator{"Face Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_GBCDArrayPath_Key, "GBCD", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer VisualizeGBCDPoleFigure::clone() const
{
  return std::make_unique<VisualizeGBCDPoleFigure>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult VisualizeGBCDPoleFigure::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPhaseOfInterestValue = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  auto pMisorientationRotationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pGBCDArrayPathValue = filterArgs.value<DataPath>(k_GBCDArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<VisualizeGBCDPoleFigureAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> VisualizeGBCDPoleFigure::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
