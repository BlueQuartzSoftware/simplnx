#include "MultiThresholdObjects.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Utilities/ArrayThreshold.hpp"

namespace complex
{
namespace
{
constexpr StringLiteral k_ArrayThresholds_Key = "array_thresholds";
constexpr StringLiteral k_CreatedDataPath_Key = "created_data_path";

constexpr int64 k_PathNotFoundError = -178;
} // namespace

std::string MultiThresholdObjects::name() const
{
  return FilterTraits<MultiThresholdObjects>::name;
}

std::string MultiThresholdObjects::className() const
{
  return FilterTraits<MultiThresholdObjects>::className;
}

Uuid MultiThresholdObjects::uuid() const
{
  return FilterTraits<MultiThresholdObjects>::uuid;
}

std::string MultiThresholdObjects::humanName() const
{
  return "Multi-Threshold Objects";
}

Parameters MultiThresholdObjects::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<ArrayThresholdsParameter>(k_ArrayThresholds_Key, "Data Thresholds", "DataArray thresholds to mask", ArrayThresholdSet{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedDataPath_Key, "Mask Array", "DataPath to the created Mask Array", DataPath{}));
  return params;
}

IFilter::UniquePointer MultiThresholdObjects::clone() const
{
  return std::make_unique<MultiThresholdObjects>();
}

std::vector<usize> getDims(const DataStructure& data, const std::set<DataPath>& dataPaths)
{
  if(dataPaths.size() == 0)
  {
    return {0};
  }
  std::vector<usize> dims;
  auto dataArray = data.getDataAs<IDataArray>(*dataPaths.begin());
  if(dataArray != nullptr)
  {
    return {dataArray->getNumberOfTuples()};
  }
  return {0};
}

IFilter::PreflightResult MultiThresholdObjects::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto thresholdsObject = args.value<ArrayThresholdSet>(k_ArrayThresholds_Key);
  auto maskArrayPath = args.value<DataPath>(k_CreatedDataPath_Key);

  auto thresholdPaths = thresholdsObject.getRequiredPaths();
  for(const auto& path : thresholdPaths)
  {
    if(!data.getData(path))
    {
      auto errorMessage = fmt::format("Could not find DataArray at path {}.", path.toString());
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_PathNotFoundError, errorMessage}})};
    }
  }

  std::vector<usize> dims = getDims(data, thresholdPaths);
  auto action = std::make_unique<CreateArrayAction>(NumericType::uint8, dims, std::vector<usize>{1}, maskArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> MultiThresholdObjects::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto thresholdsObject = args.value<ArrayThresholdSet>(k_ArrayThresholds_Key);
  auto maskArrayPath = args.value<DataPath>(k_CreatedDataPath_Key);

  thresholdsObject.applyMaskValues(data, maskArrayPath);

  return {};
}
} // namespace complex
