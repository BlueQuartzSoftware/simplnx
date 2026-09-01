#include "RandomizeFeatureIdsFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <limits>
#include <memory>
#include <stdexcept>

using namespace nx::core;

namespace
{
constexpr int32 k_EmptyParameterError = -123;
}

namespace nx::core
{
//------------------------------------------------------------------------------
std::string RandomizeFeatureIdsFilter::name() const
{
  return FilterTraits<RandomizeFeatureIdsFilter>::name;
}

//------------------------------------------------------------------------------
std::string RandomizeFeatureIdsFilter::className() const
{
  return FilterTraits<RandomizeFeatureIdsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RandomizeFeatureIdsFilter::uuid() const
{
  return FilterTraits<RandomizeFeatureIdsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RandomizeFeatureIdsFilter::humanName() const
{
  return "Randomize Feature Ids";
}

//------------------------------------------------------------------------------
std::vector<std::string> RandomizeFeatureIdsFilter::defaultTags() const
{
  return {className(), "Randomize", "Feature IDs"};
}

//------------------------------------------------------------------------------
Parameters RandomizeFeatureIdsFilter::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Feature IDs Array", "Array storing the Feature IDs", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAMPath_Key, "Feature Attribute Matrix", "The path to the Feature Attribute Matrix, so it can be updated", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RandomizeFeatureIdsFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Change 1:
  // Added - k_FeatureAMPath_Key = "feature_am_path";
  // Solution - supply a valid Feature Attribute Matrix;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RandomizeFeatureIdsFilter::clone() const
{
  return std::make_unique<RandomizeFeatureIdsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RandomizeFeatureIdsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  auto featureAMPath = filterArgs.value<DataPath>(k_FeatureAMPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  // Preflight reports that execution remaps Feature IDs in place.
  nx::core::MarkDataPathModified(dataStructure, resultOutputActions, featureIdsPath);
  // Preflight reports that execution reorders Feature AttributeMatrix arrays in place.
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, featureAMPath, {});

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> RandomizeFeatureIdsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto featureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsPath_Key);
  auto featureAMPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAMPath_Key);

  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  auto& featureIdsStore = featureIdsArray.getDataStoreRef();
  // Discover the maximum ID with one-mebibyte bulk reads. Iterating the store
  // element by element would repeatedly enter the OOC cache and dominate this
  // otherwise inexpensive permutation filter.
  constexpr usize k_TargetBufferBytes = 1024 * 1024;
  const usize totalElements = featureIdsStore.getSize();
  const usize bufferElements = std::max<usize>(1, std::min(totalElements, k_TargetBufferBytes / sizeof(int32)));
  auto buffer = std::make_unique<int32[]>(bufferElements);
  int32 maxFeatureId = std::numeric_limits<int32>::lowest();
  for(usize offset = 0; offset < totalElements; offset += bufferElements)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(bufferElements, totalElements - offset);
    Result<> readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(buffer.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    maxFeatureId = std::max(maxFeatureId, *std::max_element(buffer.get(), buffer.get() + count));
  }
  const usize totalFeatures = static_cast<usize>(maxFeatureId) + 1;

  const auto* featureAM = dataStructure.getDataAs<AttributeMatrix>(featureAMPath);
  if(totalFeatures > featureAM->getNumberOfTuples())
  {
    return MakeErrorResult(
        -82640, fmt::format("The number of tuples in the supplied Attribute Matrix ({}) is less than the max feature in the Feature Ids Array ({})", featureAM->getNumberOfTuples(), maxFeatureId));
  }

  std::optional<std::vector<DataPath>> amChildPaths = GetAllChildArrayDataPaths(dataStructure, featureAMPath);

  std::vector<IArray*> featureIArrays = {};
  if(amChildPaths.has_value())
  {
    for(const auto& childPath : amChildPaths.value())
    {
      featureIArrays.push_back(dataStructure.getDataAs<IArray>(childPath));
    }
  }

  ClusterUtilities::RandomizeFeatureIds(featureIdsStore, totalFeatures, featureIArrays);

  return {};
}
} // namespace nx::core
