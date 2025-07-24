#include "RandomizeFeatureIdsFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

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
  return {};
}

//------------------------------------------------------------------------------
Result<> RandomizeFeatureIdsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto featureIdsPath = args.value<ArraySelectionParameter::ValueType>(k_FeatureIdsPath_Key);
  auto featureAMPath = args.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAMPath_Key);

  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  auto& featureIdsStore = featureIdsArray.getDataStoreRef();
  usize totalFeatures = *std::max_element(featureIdsStore.begin(), featureIdsStore.end());

  std::optional<std::vector<DataPath>> amChildPaths = GetAllChildArrayDataPaths(dataStructure, featureAMPath);

  std::vector<IArray*> featureIArrays = {};
  if(amChildPaths.has_value())
  {
    for(const auto& childPath : amChildPaths.value())
    {
      featureIArrays.push_back(dataStructure.getDataAs<IArray>(childPath));
    }
  }

  ClusterUtilities::RandomizeFeatureIds(featureIdsStore, (totalFeatures + 1), featureIArrays);

  return {};
}
} // namespace nx::core
