#include "ComputeGBCDPoleFigureFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/ComputeGBCDPoleFigure.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeGBCDPoleFigureFilter::name() const
{
  return FilterTraits<ComputeGBCDPoleFigureFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeGBCDPoleFigureFilter::className() const
{
  return FilterTraits<ComputeGBCDPoleFigureFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeGBCDPoleFigureFilter::uuid() const
{
  return FilterTraits<ComputeGBCDPoleFigureFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeGBCDPoleFigureFilter::humanName() const
{
  return "Compute GBCD Pole Figure";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeGBCDPoleFigureFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Pole Figure", "GBCD", "Generate"};
}

//------------------------------------------------------------------------------
Parameters ComputeGBCDPoleFigureFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Int32Parameter>(k_PhaseOfInterest_Key, "Phase of Interest", "Index of the Ensemble for which to plot the pole figure", 1));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MisorientationRotation_Key, "Misorientation Angle-Axis", "Angle-Axis values for drawing GBCD", std::vector<float32>{60.0F, 1.0F, 1.0F, 1.0F},
                                                         std::vector<std::string>{"Angle (Deg)", "h", "k", "l"}));
  params.insert(std::make_unique<Int32Parameter>(k_OutputImageDimension_Key, "Output Image Dimension", "The value to use for the dimensions for the image geometry", 100));
  params.insertSeparator(Parameters::Separator{"Input Triangle Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_GBCDArrayPath_Key, "Input GBCD",
                                                          "5 parameter GBCD data. The 6th component is used internally to track the northern vs. southern hemisphere of the Lambert sphere",
                                                          DataPath({"Face Ensemble Data", "GBCD"}), ArraySelectionParameter::AllowedTypes{DataType::float64}));
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
IFilter::UniquePointer ComputeGBCDPoleFigureFilter::clone() const
{
  return std::make_unique<ComputeGBCDPoleFigureFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeGBCDPoleFigureFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  nx::core::Result<OutputActions> resultOutputActions;
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
Result<> ComputeGBCDPoleFigureFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  ComputeGBCDPoleFigureInputValues inputValues;
  inputValues.PhaseOfInterest = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  inputValues.MisorientationRotation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  inputValues.GBCDArrayPath = filterArgs.value<DataPath>(k_GBCDArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.OutputImageDimension = filterArgs.value<int32>(k_OutputImageDimension_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.CellIntensityArrayName = filterArgs.value<std::string>(k_CellIntensityArrayName_Key);

  return ComputeGBCDPoleFigure(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_PhaseOfInterestKey = "PhaseOfInterest";
constexpr StringLiteral k_MisorientationRotationKey = "MisorientationRotation";
constexpr StringLiteral k_OutputFileKey = "OutputFile";
constexpr StringLiteral k_OutputDimensionKey = "OutputDimension";
constexpr StringLiteral k_GBCDArrayPathKey = "GBCDArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_IntensityArrayNameKey = "IntensityArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeGBCDPoleFigureFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeGBCDPoleFigureFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_PhaseOfInterestKey, k_PhaseOfInterest_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AxisAngleFilterParameterConverter<float32>>(args, json, SIMPL::k_MisorientationRotationKey, k_MisorientationRotation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_OutputDimensionKey, k_OutputImageDimension_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GBCDArrayPathKey, k_GBCDArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_ImageGeometryName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_IntensityArrayNameKey, k_CellIntensityArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
