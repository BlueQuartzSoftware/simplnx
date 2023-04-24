#include "GenerateGBCDPoleFigureFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/GenerateGBCDPoleFigure.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateGBCDPoleFigureFilter::name() const
{
  return FilterTraits<GenerateGBCDPoleFigureFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateGBCDPoleFigureFilter::className() const
{
  return FilterTraits<GenerateGBCDPoleFigureFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateGBCDPoleFigureFilter::uuid() const
{
  return FilterTraits<GenerateGBCDPoleFigureFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateGBCDPoleFigureFilter::humanName() const
{
  return "Generate GBCD Pole Figure";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateGBCDPoleFigureFilter::defaultTags() const
{
  return {"IO", "Output", "Write", "Export", "Pole Figure", "GBCD"};
}

//------------------------------------------------------------------------------
Parameters GenerateGBCDPoleFigureFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Int32Parameter>(k_PhaseOfInterest_Key, "Phase of Interest", "Index of the Ensemble for which to plot the pole figure", 1));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MisorientationRotation_Key, "Misorientation Angle-Axis", "Angle-Axis values for drawing GBCD", std::vector<float32>{60.0F, 1.0F, 1.0F, 1.0F},
                                                         std::vector<std::string>{"Angle (Deg)", "h", "k", "l"}));
  params.insert(std::make_unique<Int32Parameter>(k_OutputImageDimension_Key, "Output Image Dimension", "The value to use for the dimensions for the image geometry", 100));
  params.insertSeparator(Parameters::Separator{"Required Face Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_GBCDArrayPath_Key, "Input GBCD",
                                                          "5 parameter GBCD data. The 6th component is used internally to track the northern vs. southern hemisphere of the Lambert sphere",
                                                          DataPath({"FaceEnsembleData", "GBCD"}), ArraySelectionParameter::AllowedTypes{DataType::float64}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Pole Figure"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryName_Key, "Image Geometry", "The path to the created image geometry", DataPath({"GBCD Pole Figure"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "The name of the cell attribute matrix for the created image geometry",
                                                          ImageGeom::k_CellDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellIntensityArrayName_Key, "Cell MRD Array Name", "The name of the created cell intensity data array", "MRD"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateGBCDPoleFigureFilter::clone() const
{
  return std::make_unique<GenerateGBCDPoleFigureFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateGBCDPoleFigureFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto pPhaseOfInterestValue = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  auto pMisorientationRotationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  auto pOutputImageDimension = static_cast<usize>(filterArgs.value<int32>(k_OutputImageDimension_Key));
  auto pGBCDArrayPathValue = filterArgs.value<DataPath>(k_GBCDArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryName_Key);
  auto pCellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellIntensityArrayName = filterArgs.value<std::string>(k_CellIntensityArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(dataStructure.getDataAs<UInt32Array>(pCrystalStructuresArrayPathValue) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-34640, fmt::format("Could not find crystal structures array at path '{}'", pCrystalStructuresArrayPathValue.toString()))};
  }

  const auto gbcd = dataStructure.getDataAs<Float64Array>(pGBCDArrayPathValue);
  if(gbcd == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-34641, fmt::format("Could not find GBCD array at path '{}'", pGBCDArrayPathValue.toString()))};
  }

  auto numEnsembles = gbcd->getNumberOfTuples();
  if(pPhaseOfInterestValue >= numEnsembles)
  {
    return {MakeErrorResult<OutputActions>(-34642, fmt::format("The phase index {} is larger than the number of Ensembles {}", pPhaseOfInterestValue, numEnsembles))};
  }

  std::vector<usize> imageDims = {pOutputImageDimension, pOutputImageDimension, 1ULL}; // Image Geometry wants the tuple dims as XYZ
  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(
      pImageGeometryPath, imageDims, CreateImageGeometryAction::OriginType{-1.0f, -1.0f, 0.0f},
      CreateImageGeometryAction::SpacingType{2.0f / pOutputImageDimension, 2.0f / pOutputImageDimension, 2.0f / pOutputImageDimension}, pCellAttributeMatrixName);
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  std::vector<usize> cDims = {1};
  std::vector<usize> tDims = {1ULL, pOutputImageDimension, pOutputImageDimension}; // The DataArray Tuple Dims should be slowest to fastest, "C" Order

  DataPath cellIntensityArrayPath = pImageGeometryPath.createChildPath(pCellAttributeMatrixName).createChildPath(pCellIntensityArrayName);
  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float64, tDims, cDims, cellIntensityArrayPath);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateGBCDPoleFigureFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  GenerateGBCDPoleFigureInputValues inputValues;
  inputValues.PhaseOfInterest = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  inputValues.MisorientationRotation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  inputValues.GBCDArrayPath = filterArgs.value<DataPath>(k_GBCDArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.OutputImageDimension = filterArgs.value<int32>(k_OutputImageDimension_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.CellIntensityArrayName = filterArgs.value<std::string>(k_CellIntensityArrayName_Key);

  return GenerateGBCDPoleFigure(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
