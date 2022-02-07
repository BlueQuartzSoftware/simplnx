#include "ScalarSegmentFeaturesFilter.hpp"

#include <chrono>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ComplexCore/Filters/Algorithms/ScalarSegmentFeatures.hpp"

using namespace complex;

#define CX_DEFAULT_CONSTRUCTORS(className)                                                                                                                                                             \
  className(const className&) = delete;                                                                                                                                                                \
  className(className&&) noexcept = delete;                                                                                                                                                            \
  className& operator=(const className&) = delete;                                                                                                                                                     \
  className& operator=(className&&) noexcept = delete;

namespace
{
using FeatureIdsArrayType = Int32Array;
using GoodVoxelsArrayType = UInt8Array;

constexpr StringLiteral k_CompareFunctKey = "Compare Function";

constexpr int64 k_IncorrectInputArray = -600;
constexpr int64 k_MissingInputArray = -601;
constexpr int64 k_MissingOrIncorrectGoodVoxelsArray = -602;
inline constexpr int32 k_MissingGeomError = -440;

} // namespace

namespace complex
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
  return {"#Reconstruction", "#Segmentation"};
}

Parameters ScalarSegmentFeaturesFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataPathSelectionParameter>(k_GridGeomPath_Key, "Grid Geometry", "DataPath to target Grid Geometry", DataPath{}));

  params.insert(std::make_unique<NumberParameter<int>>(k_ScalarToleranceKey, "Scalar Tolerance", "Tolerance for segmenting input Cell Data", 1));
  params.insert(std::make_unique<BoolParameter>(k_UseGoodVoxelsKey, "Use Mask Array", "Determines if a mask array is used for segmenting", false));

  params.insert(std::make_unique<ArraySelectionParameter>(k_InputArrayPathKey, "Scalar Array to Segment", "Path to the DataArray to segment", DataPath()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsPathKey, "Mask", "Path to the DataArray Mask", DataPath(), true));

  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsPathKey, "Feature IDs", "Path to the created Feature IDs path", DataPath()));

  // params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeaturePathKey, "Cell Feature Group", "Path to the parent Feature group", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayPathKey, "Active", "Created array", DataPath()));

  params.insert(std::make_unique<BoolParameter>(k_RandomizeFeaturesKey, "Randomize Feature IDs", "Specifies if feature IDs should be randomized during calculations", false));
  return params;
}

IFilter::UniquePointer ScalarSegmentFeaturesFilter::clone() const
{
  return std::make_unique<ScalarSegmentFeaturesFilter>();
}

IFilter::PreflightResult ScalarSegmentFeaturesFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto inputDataPath = args.value<DataPath>(k_InputArrayPathKey);
  int scalarTolerance = args.value<int>(k_ScalarToleranceKey);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPathKey);
  // auto cellFeaturesPath = args.value<DataPath>(k_CellFeaturePathKey);
  auto activeArrayPath = args.value<DataPath>(k_ActiveArrayPathKey);

  bool useGoodVoxels = args.value<bool>(k_UseGoodVoxelsKey);
  DataPath goodVoxelsPath;
  if(useGoodVoxels)
  {
    goodVoxelsPath = args.value<DataPath>(k_GoodVoxelsPathKey);
  }

  auto gridGeomPath = args.value<DataPath>(k_GridGeomPath_Key);

  if(data.getDataAs<AbstractGeometryGrid>(gridGeomPath) == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingGeomError, "A Grid Geometry is required for SegmentFeaturesFilter"}})};
  }

  std::vector<DataPath> dataPaths;

  usize numTuples = 0;
  // Input Array
  if(auto inputDataArray = data.getDataAs<IDataArray>(inputDataPath))
  {
    numTuples = inputDataArray->getNumberOfTuples();
    if(inputDataArray->getNumberOfComponents() == 1)
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
    const complex::IDataArray* goodVoxelsArray = data.getDataAs<IDataArray>(goodVoxelsPath);
    if(nullptr == goodVoxelsArray)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array is not located at path: '{}'", goodVoxelsPath.toString())}})};
    }
    const GoodVoxelsArrayType* maskArray = data.getDataAs<GoodVoxelsArrayType>(goodVoxelsPath);
    if(maskArray == nullptr)
    {
      return {nonstd::make_unexpected(
          std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array at path '{}' is not of the correct type. It must be UInt8.", goodVoxelsPath.toString())}})};
    }
    dataPaths.push_back(goodVoxelsPath);
  }

  data.validateNumberOfTuples(dataPaths);

  // Create Active array
  auto createActiveAction = std::make_unique<CreateArrayAction>(NumericType::uint8, std::vector<usize>{numTuples}, std::vector<usize>{1}, activeArrayPath);
  auto createFeatureIdsAction = std::make_unique<CreateArrayAction>(NumericType::int32, std::vector<usize>{numTuples}, std::vector<usize>{1}, featureIdsPath);

  OutputActions actions;
  actions.actions.push_back(std::move(createActiveAction));
  actions.actions.push_back(std::move(createFeatureIdsAction));

  return {std::move(actions)};
}

// -----------------------------------------------------------------------------
Result<> ScalarSegmentFeaturesFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{

  ScalarSegmentFeaturesInputValues inputValues;

  inputValues.pInputDataPath = args.value<DataPath>(k_InputArrayPathKey);
  inputValues.pScalarTolerance = args.value<int>(k_ScalarToleranceKey);
  inputValues.pShouldRandomizeFeatureIds = args.value<bool>(k_RandomizeFeaturesKey);
  inputValues.pActiveArrayPath = args.value<DataPath>(k_ActiveArrayPathKey);
  inputValues.pFeatureIdsPath = args.value<DataPath>(k_FeatureIdsPathKey);
  inputValues.pUseGoodVoxels = args.value<bool>(k_UseGoodVoxelsKey);
  inputValues.pGoodVoxelsPath = args.value<DataPath>(k_GoodVoxelsPathKey);
  inputValues.pGridGeomPath = args.value<DataPath>(k_GridGeomPath_Key);

  complex::ScalarSegmentFeatures scalarSegmentFeatures(data, &inputValues, this, messageHandler);
  Result<> result = scalarSegmentFeatures();
  return result;
}

} // namespace complex
