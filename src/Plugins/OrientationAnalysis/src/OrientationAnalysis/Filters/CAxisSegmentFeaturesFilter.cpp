#include "CAxisSegmentFeaturesFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/CAxisSegmentFeatures.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CAxisSegmentFeaturesFilter::name() const
{
  return FilterTraits<CAxisSegmentFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CAxisSegmentFeaturesFilter::className() const
{
  return FilterTraits<CAxisSegmentFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CAxisSegmentFeaturesFilter::uuid() const
{
  return FilterTraits<CAxisSegmentFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CAxisSegmentFeaturesFilter::humanName() const
{
  return "Segment Features (C-Axis Misalignment)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CAxisSegmentFeaturesFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Segmentation"};
}

//------------------------------------------------------------------------------
Parameters CAxisSegmentFeaturesFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "C-Axis Misorientation Tolerance (Degrees)",
                                                   "Tolerance (in degrees) used to determine if neighboring Cells belong to the same Feature", 5.0f));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Use Mask Array", "Specifies whether to use a boolean array to exclude some Cells from the Feature identification process", true));
  params.insert(std::make_unique<BoolParameter>(k_RandomizeFeatureIds_Key, "Randomize Feature Ids", "Specifies whether to randomize the feature ids", true));
  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "Specifies if the Cell is to be counted in the algorithm. Only required if Use Mask Array is checked",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "Specifies to which Feature each Cell belongs", "FeatureIds"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "The name of the created feature attribute matrix", "CellFeatureData"));
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_ActiveArrayName_Key, "Active",
      "Specifies if the Feature is still in the sample (true if the Feature is in the sample and false if it is not). At the end of the Filter, all Features will be Active", "Active"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CAxisSegmentFeaturesFilter::clone() const
{
  return std::make_unique<CAxisSegmentFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CAxisSegmentFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellFeatureAttributeMatrixName_Key);
  auto pActiveArrayNameValue = filterArgs.value<std::string>(k_ActiveArrayName_Key);

  const DataPath featureIdsPath = pQuatsArrayPathValue.getParent().createChildPath(pFeatureIdsArrayNameValue);
  const DataPath cellFeatureAMPath = pImageGeometryPath.createChildPath(pCellFeatureAttributeMatrixNameValue);
  const DataPath activePath = cellFeatureAMPath.createChildPath(pActiveArrayNameValue);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataPaths = {pQuatsArrayPathValue, pCellPhasesArrayPathValue};
  if(pUseGoodVoxelsValue)
  {
    dataPaths.push_back(filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key));
  }

  const auto& imageGeo = dataStructure.getDataRefAs<ImageGeom>(pImageGeometryPath);
  const auto* cellDataAM = imageGeo.getCellData();
  if(!dataStructure.validateNumberOfTuples(dataPaths))
  {
    return MakePreflightErrorResult(-8360, fmt::format("One or more of the cell data arrays (quaternions, phases, and mask) have mismatching number of tuples. Make sure these arrays are located in "
                                                       "the cell data attribute matrix '{}' in the selected image geometry '{}'",
                                                       cellDataAM->getName(), imageGeo.getName()));
  }

  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, cellDataAM->getShape(), std::vector<usize>{1}, featureIdsPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(cellFeatureAMPath, std::vector<usize>{1});
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, std::vector<usize>{1}, std::vector<usize>{1}, activePath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-8361, "Segmenting features via c-axis mis orientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these two types."});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CAxisSegmentFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  CAxisSegmentFeaturesInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key) * Constants::k_PiOver180F;
  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.RandomizeFeatureIds = filterArgs.value<bool>(k_RandomizeFeatureIds_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.FeatureIdsArrayName = inputValues.QuatsArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));
  inputValues.CellFeatureAttributeMatrixName = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellFeatureAttributeMatrixName_Key));
  inputValues.ActiveArrayName = inputValues.CellFeatureAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_ActiveArrayName_Key));

  return CAxisSegmentFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
