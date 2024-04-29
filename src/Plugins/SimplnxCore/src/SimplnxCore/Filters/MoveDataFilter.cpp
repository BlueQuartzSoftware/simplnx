#include "MoveDataFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/Filter/Actions/MoveDataAction.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"

namespace nx::core
{
//------------------------------------------------------------------------------
std::string MoveDataFilter::name() const
{
  return FilterTraits<MoveDataFilter>::name;
}

//------------------------------------------------------------------------------
std::string MoveDataFilter::className() const
{
  return FilterTraits<MoveDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MoveDataFilter::uuid() const
{
  return FilterTraits<MoveDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MoveDataFilter::humanName() const
{
  return "Move Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> MoveDataFilter::defaultTags() const
{
  return {className(), "Move", "Memory Management", "Data Management", "Data Structure"};
}

//------------------------------------------------------------------------------
Parameters MoveDataFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<MultiPathSelectionParameter>(k_SourceDataPaths_Key, "Data to Move", "The complete paths to the data object(s) to be moved", MultiPathSelectionParameter::ValueType{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DestinationParentPath_Key, "New Parent", "The complete path to the parent data object to which the data will be moved", DataPath(),
                                                              BaseGroup::GetAllGroupTypes()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MoveDataFilter::clone() const
{
  return std::make_unique<MoveDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MoveDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto dataPaths = args.value<MultiPathSelectionParameter::ValueType>(k_SourceDataPaths_Key);
  auto newParentPath = args.value<DataPath>(k_DestinationParentPath_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Scope AM check since we fully expect it to be a nullptr
  {
    const auto* possibleAM = dataStructure.getDataAs<AttributeMatrix>(newParentPath);
    if(possibleAM != nullptr)
    {
      for(const auto& path : dataPaths)
      {
        const auto* possibleIArray = dataStructure.getDataAs<IArray>(path);
        if(possibleIArray != nullptr)
        {
          if(possibleAM->getShape() != possibleIArray->getTupleShape())
          {
            resultOutputActions.warnings().push_back(Warning{-69250, fmt::format("The tuple dimensions of {} [{}] do not match the AttributeMatrix {} tuple dimensions [{}]. This could result in a "
                                                                                 "runtime error if the sizing remains the same at time of this filters execution. Proceed with caution.",
                                                                                 possibleIArray->getName(), possibleIArray->getNumberOfTuples(), possibleAM->getName(), possibleAM->getNumTuples())});
          }
        }
      }
    }
  }

  for(const auto& dataPath : dataPaths)
  {
    auto moveDataAction = std::make_unique<MoveDataAction>(dataPath, newParentPath);
    resultOutputActions.value().appendAction(std::move(moveDataAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MoveDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                     const std::atomic_bool& shouldCancel) const
{
  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_WhatToMoveKey = "WhatToMove";
constexpr StringLiteral k_AttributeMatrixSourceKey = "AttributeMatrixSource";
constexpr StringLiteral k_DataContainerDestinationKey = "DataContainerDestination";
constexpr StringLiteral k_DataArraySourceKey = "DataArraySource";
constexpr StringLiteral k_AttributeMatrixDestinationKey = "AttributeMatrixDestination";
} // namespace SIMPL
} // namespace

Result<Arguments> MoveDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MoveDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  if(json[SIMPL::k_WhatToMoveKey].get<int32>() == 0)
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::SingleToMultiDataPathSelectionFilterParameterConverter>(args, json, SIMPL::k_AttributeMatrixSourceKey, k_SourceDataPaths_Key));
    results.push_back(
        SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_DataContainerDestinationKey, k_DestinationParentPath_Key));
  }
  else
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::SingleToMultiDataPathSelectionFilterParameterConverter>(args, json, SIMPL::k_DataArraySourceKey, k_SourceDataPaths_Key));
    results.push_back(
        SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_AttributeMatrixDestinationKey, k_DestinationParentPath_Key));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
