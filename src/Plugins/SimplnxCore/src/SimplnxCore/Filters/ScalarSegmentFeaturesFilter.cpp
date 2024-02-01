#include "ScalarSegmentFeaturesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ScalarSegmentFeatures.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

#define CX_DEFAULT_CONSTRUCTORS(className)                                                                                                                                                             \
  className(const className&) = delete;                                                                                                                                                                \
  className(className&&) noexcept = delete;                                                                                                                                                            \
  className& operator=(const className&) = delete;                                                                                                                                                     \
  className& operator=(className&&) noexcept = delete;

namespace
{
using FeatureIdsArrayType = Int32Array;
using GoodVoxelsArrayType = BoolArray;

constexpr int64 k_IncorrectInputArray = -600;
constexpr int64 k_MissingInputArray = -601;
constexpr int64 k_MissingOrIncorrectGoodVoxelsArray = -602;
inline constexpr int32 k_MissingGeomError = -440;

} // namespace

namespace nx::core
{

std::string ScalarSegmentFeaturesFilter::name() const
{
  return FilterTraits<ScalarSegmentFeaturesFilter>::name;
}

std::string ScalarSegmentFeaturesFilter::className() const
{
  return FilterTraits<ScalarSegmentFeaturesFilter>::className;
}

Uuid ScalarSegmentFeaturesFilter::uuid() const
{
  return FilterTraits<ScalarSegmentFeaturesFilter>::uuid;
}

std::string ScalarSegmentFeaturesFilter::humanName() const
{
  return "Segment Features (Scalar)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ScalarSegmentFeaturesFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Segmentation"};
}

Parameters ScalarSegmentFeaturesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Segmentation Parameters"});
  params.insert(std::make_unique<NumberParameter<int>>(k_ScalarToleranceKey, "Scalar Tolerance", "Tolerance for segmenting input Cell Data", 1));
  params.insert(std::make_unique<BoolParameter>(k_RandomizeFeatures_Key, "Randomize Feature IDs", "Specifies if feature IDs should be randomized during calculations", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Determines if a mask array is used for segmenting", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Path to the DataArray Mask", DataPath(), ArraySelectionParameter::AllowedTypes{DataType::boolean},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, std::make_any<bool>(true));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GridGeomPath_Key, "Input Image Geometry", "DataPath to input Image Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputArrayPathKey, "Scalar Array to Segment", "Path to the DataArray to segment", DataPath(), nx::core::GetIntegerDataTypes(),
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsPathKey, "Cell Feature IDs", "Path to the created Feature IDs path", "FeatureIds"));

  params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellFeaturePathKey, "Cell Feature Attribute Matrix", "Created Cell Feature Attribute Matrix", "CellFeatureData"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ActiveArrayPathKey, "Active", "Created array", "Active"));

  return params;
}

IFilter::UniquePointer ScalarSegmentFeaturesFilter::clone() const
{
  return std::make_unique<ScalarSegmentFeaturesFilter>();
}

IFilter::PreflightResult ScalarSegmentFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto inputDataPath = args.value<DataPath>(k_InputArrayPathKey);
  int scalarTolerance = args.value<int>(k_ScalarToleranceKey);
  auto featureIdsName = args.value<std::string>(k_FeatureIdsPathKey);
  auto cellFeaturesName = args.value<std::string>(k_CellFeaturePathKey);
  auto activeArrayName = args.value<std::string>(k_ActiveArrayPathKey);
  DataPath featureIdsPath = inputDataPath.getParent().createChildPath(featureIdsName);

  bool useGoodVoxels = args.value<bool>(k_UseMask_Key);
  DataPath goodVoxelsPath;
  if(useGoodVoxels)
  {
    goodVoxelsPath = args.value<DataPath>(k_MaskArrayPath_Key);
  }

  auto gridGeomPath = args.value<DataPath>(k_GridGeomPath_Key);
  DataPath cellFeaturesPath = gridGeomPath.createChildPath(cellFeaturesName);
  DataPath activeArrayPath = cellFeaturesPath.createChildPath(activeArrayName);

  if(dataStructure.getDataAs<IGridGeometry>(gridGeomPath) == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingGeomError, fmt::format("A Grid Geometry is required for {}", humanName())}})};
  }

  const auto* gridGeometryPtr = dataStructure.getDataAs<IGridGeometry>(gridGeomPath);
  auto gridDims = gridGeometryPtr->getDimensions();
  const std::vector<usize> cellTupleDims = {gridDims[2], gridDims[1], gridDims[0]};
  std::vector<DataPath> dataPaths;

  usize numTuples = 0;
  std::string createdArrayFormat = "";
  // Input Array
  if(const auto* inputDataArrayPtr = dataStructure.getDataAs<IDataArray>(inputDataPath))
  {
    createdArrayFormat = inputDataArrayPtr->getDataFormat();
    numTuples = inputDataArrayPtr->getNumberOfTuples();
    if(inputDataArrayPtr->getNumberOfComponents() == 1)
    {
      dataPaths.push_back(inputDataPath);
    }
    else
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Input Array must be a scalar array"}})};
    }
  }
  else
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingInputArray, "Input Array must be specified"}})};
  }

  // Good Voxels
  if(useGoodVoxels)
  {
    const nx::core::IDataArray* goodVoxelsArrayPtr = dataStructure.getDataAs<IDataArray>(goodVoxelsPath);
    if(nullptr == goodVoxelsArrayPtr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array is not located at path: '{}'", goodVoxelsPath.toString())}})};
    }
    const GoodVoxelsArrayType* maskArrayPtr = dataStructure.getDataAs<GoodVoxelsArrayType>(goodVoxelsPath);
    if(maskArrayPtr == nullptr)
    {
      return {nonstd::make_unexpected(
          std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array at path '{}' is not of the correct type. It must be Bool.", goodVoxelsPath.toString())}})};
    }
    dataPaths.push_back(goodVoxelsPath);
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Create the Cell Level FeatureIds array
  auto createFeatureIdsAction = std::make_unique<CreateArrayAction>(DataType::int32, cellTupleDims, std::vector<usize>{1}, featureIdsPath, createdArrayFormat);

  // Create the Feature Attribute Matrix
  auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(cellFeaturesPath, std::vector<usize>{numTuples});
  auto createActiveAction = std::make_unique<CreateArrayAction>(DataType::uint8, std::vector<usize>{numTuples}, std::vector<usize>{1}, activeArrayPath, createdArrayFormat);

  OutputActions actions;
  actions.appendAction(std::move(createAttributeMatrixAction));
  actions.appendAction(std::move(createActiveAction));
  actions.appendAction(std::move(createFeatureIdsAction));

  return {std::move(actions)};
}

// -----------------------------------------------------------------------------
Result<> ScalarSegmentFeaturesFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  ScalarSegmentFeaturesInputValues inputValues;

  inputValues.pInputDataPath = args.value<DataPath>(k_InputArrayPathKey);
  inputValues.pScalarTolerance = args.value<int>(k_ScalarToleranceKey);
  inputValues.pShouldRandomizeFeatureIds = args.value<bool>(k_RandomizeFeatures_Key);
  inputValues.pFeatureIdsPath = inputValues.pInputDataPath.getParent().createChildPath(args.value<std::string>(k_FeatureIdsPathKey));
  inputValues.pUseGoodVoxels = args.value<bool>(k_UseMask_Key);
  inputValues.pGoodVoxelsPath = args.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.pGridGeomPath = args.value<DataPath>(k_GridGeomPath_Key);
  inputValues.pCellFeaturesPath = inputValues.pGridGeomPath.createChildPath(args.value<std::string>(k_CellFeaturePathKey));
  inputValues.pActiveArrayPath = inputValues.pCellFeaturesPath.createChildPath(args.value<std::string>(k_ActiveArrayPathKey));

  nx::core::ScalarSegmentFeatures filterAlgorithm(data, &inputValues, shouldCancel, messageHandler);
  Result<> result = filterAlgorithm();
  return result;
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ScalarToleranceKey = "ScalarTolerance";
constexpr StringLiteral k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteral k_RandomizeFeatureIdsKey = "RandomizeFeatureIds";
constexpr StringLiteral k_ScalarArrayPathKey = "ScalarArrayPath";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteral k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
constexpr StringLiteral k_CellFeatureAttributeMatrixNameKey = "CellFeatureAttributeMatrixName";
constexpr StringLiteral k_ActiveArrayNameKey = "ActiveArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ScalarSegmentFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ScalarSegmentFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_ScalarToleranceKey, k_ScalarToleranceKey));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_RandomizeFeatureIdsKey, k_RandomizeFeatures_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ScalarArrayPathKey, k_InputArrayPathKey));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayNameKey, k_FeatureIdsPathKey));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellFeatureAttributeMatrixNameKey, k_CellFeaturePathKey));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_ScalarArrayPathKey, k_GridGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_ActiveArrayNameKey, k_ActiveArrayPathKey));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
