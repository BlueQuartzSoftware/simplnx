#include "RandomizeFeatureIdsFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

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
  return "Randomize Feature IDs";
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

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIds_Key, "Feature IDs Array", "Array storing the Feature IDs", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RandomizeFeatureIdsFilter::parametersVersion() const
{
  return 1;
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
  return {};
}

//------------------------------------------------------------------------------
Result<> RandomizeFeatureIdsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIds_Key);

  Int32Array& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  auto& featureIdsStore = featureIdsArray.getDataStoreRef();
  usize totalFeatures = *std::max_element(featureIdsStore.begin(), featureIdsStore.end());

  ClusterUtilities::RandomizeFeatureIds(featureIdsStore, totalFeatures);

  return {};
}
} // namespace nx::core
