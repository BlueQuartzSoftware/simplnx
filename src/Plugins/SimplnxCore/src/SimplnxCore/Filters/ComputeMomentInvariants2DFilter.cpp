#include "ComputeMomentInvariants2DFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeMomentInvariants2D.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeMomentInvariants2DFilter::name() const
{
  return FilterTraits<ComputeMomentInvariants2DFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeMomentInvariants2DFilter::className() const
{
  return FilterTraits<ComputeMomentInvariants2DFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeMomentInvariants2DFilter::uuid() const
{
  return FilterTraits<ComputeMomentInvariants2DFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeMomentInvariants2DFilter::humanName() const
{
  return "Compute Moment Invariants (2D)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeMomentInvariants2DFilter::defaultTags() const
{
  return {className(), "Unsupported", "Statistics"};
}

//------------------------------------------------------------------------------
Parameters ComputeMomentInvariants2DFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "2D Image Geometry", "The path to the 2D image geometry to be used as input", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureRectArrayPath_Key, "Feature Rect", "Array holding the min xy and max xy pixel coordinates of each feature id", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{6}}));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeMomentInvariants_Key, "Normalize Moment Invariants", "Should the algorithm normalize the results to unit circle.", true));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix",
                                                                    "The path to the cell feature attribute matrix where the created data arrays will be stored", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_Omega1ArrayName_Key, "Omega 1", "Omega1 value", "Omega1"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_Omega2ArrayName_Key, "Omega 2", "Omega2 value", "Omega2"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveCentralMoments_Key, "Save Central Moments", "Write the Central Moments to a new Data Array", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CentralMomentsArrayName_Key, "Central Moments", "Central Moments value", "CentralMoments"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SaveCentralMoments_Key, k_CentralMomentsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeMomentInvariants2DFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeMomentInvariants2DFilter::clone() const
{
  return std::make_unique<ComputeMomentInvariants2DFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeMomentInvariants2DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto pGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureRectArrayPathValue = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);
  auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pOmega1ArrayNameValue = filterArgs.value<std::string>(k_Omega1ArrayName_Key);
  auto pOmega2ArrayNameValue = filterArgs.value<std::string>(k_Omega2ArrayName_Key);
  auto pSaveCentralMomentsValue = filterArgs.value<bool>(k_SaveCentralMoments_Key);

  const DataPath omega1Path = pFeatureAttributeMatrixPathValue.createChildPath(pOmega1ArrayNameValue);
  const DataPath omega2Path = pFeatureAttributeMatrixPathValue.createChildPath(pOmega2ArrayNameValue);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& imageGeometry = dataStructure.getDataRefAs<ImageGeom>(pGeometryPathValue);
  const SizeVec3 imageDimensions = imageGeometry.getDimensions();
  if(imageDimensions[2] != 1)
  {
    return MakePreflightErrorResult(-9510, fmt::format("This filter currently only works on XY Planes in 2D data and the input geometry at '{}' has more than 1 dimension in the Z direction. "
                                                       "Either crop the 3D data down to 2D in the Z Direction or use other data.",
                                                       pGeometryPathValue.toString()));
  }

  std::vector<usize> componentDims(1, 1);
  const auto& cellFeatureAttributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(pFeatureAttributeMatrixPathValue);
  const std::vector<usize>& tupleDims = cellFeatureAttributeMatrix.getShape();

  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, componentDims, omega1Path);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, componentDims, omega2Path);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  if(pSaveCentralMomentsValue)
  {
    auto pCentralMomentsArrayNameValue = filterArgs.value<std::string>(k_CentralMomentsArrayName_Key);
    const DataPath centralMomentsPath = pFeatureAttributeMatrixPathValue.createChildPath(pCentralMomentsArrayNameValue);
    componentDims = {3, 3};
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, componentDims, centralMomentsPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeMomentInvariants2DFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  ComputeMomentInvariants2DInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeatureRectArrayPath = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);
  inputValues.NormalizeMomentInvariants = filterArgs.value<bool>(k_NormalizeMomentInvariants_Key);
  const auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  inputValues.Omega1ArrayPath = pFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_Omega1ArrayName_Key));
  inputValues.Omega2ArrayPath = pFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_Omega2ArrayName_Key));
  inputValues.SaveCentralMoments = filterArgs.value<bool>(k_SaveCentralMoments_Key);
  if(inputValues.SaveCentralMoments)
  {
    inputValues.CentralMomentsArrayPath = pFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_CentralMomentsArrayName_Key));
  }

  return ComputeMomentInvariants2D(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_FeatureRectArrayPathKey = "FeatureRectArrayPath";
constexpr StringLiteral k_NormalizeMomentInvariantsKey = "NormalizeMomentInvariants";
constexpr StringLiteral k_Omega1ArrayPathKey = "Omega1ArrayPath";
constexpr StringLiteral k_Omega2ArrayPathKey = "Omega2ArrayPath";
constexpr StringLiteral k_SaveCentralMomentsKey = "SaveCentralMoments";
constexpr StringLiteral k_CentralMomentsArrayPathKey = "CentralMomentsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeMomentInvariants2DFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeMomentInvariants2DFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureRectArrayPathKey, k_FeatureRectArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_NormalizeMomentInvariantsKey, k_NormalizeMomentInvariants_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_Omega1ArrayPathKey, k_FeatureAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_Omega1ArrayPathKey, k_Omega1ArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_Omega2ArrayPathKey, k_Omega2ArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_SaveCentralMomentsKey, k_SaveCentralMoments_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_CentralMomentsArrayPathKey, k_CentralMomentsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
