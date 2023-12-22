#include "MoveData.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/Filter/Actions/MoveDataAction.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"

namespace
{
constexpr int64 k_TupleCountInvalidError = -250;
constexpr int64 k_MissingFeaturePhasesError = -251;

} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string MoveData::name() const
{
  return FilterTraits<MoveData>::name;
}

//------------------------------------------------------------------------------
std::string MoveData::className() const
{
  return FilterTraits<MoveData>::className;
}

//------------------------------------------------------------------------------
Uuid MoveData::uuid() const
{
  return FilterTraits<MoveData>::uuid;
}

//------------------------------------------------------------------------------
std::string MoveData::humanName() const
{
  return "Move Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> MoveData::defaultTags() const
{
  return {className(), "Move", "Memory Management", "Data Management", "Data Structure"};
}

//------------------------------------------------------------------------------
Parameters MoveData::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<MultiPathSelectionParameter>(k_Data_Key, "Data to Move", "The complete paths to the data object(s) to be moved", MultiPathSelectionParameter::ValueType{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_NewParent_Key, "New Parent", "The complete path to the parent data object to which the data will be moved", DataPath(),
                                                              BaseGroup::GetAllGroupTypes()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MoveData::clone() const
{
  return std::make_unique<MoveData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MoveData::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto dataPaths = args.value<MultiPathSelectionParameter::ValueType>(k_Data_Key);
  auto newParentPath = args.value<DataPath>(k_NewParent_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Scope AM check since we fully expect it to be a nullptr
  {
    const auto* possibleAM = data.getDataAs<AttributeMatrix>(newParentPath);
    if(possibleAM != nullptr)
    {
      for(const auto&  path : dataPaths)
      {
        const auto* possibleIArray = data.getDataAs<IArray>(path);
        if(possibleIArray != nullptr)
        {
          if(possibleAM.getShape() != possibleIArray.getTupleShape())
          {
            resultOutputActions.warnings().push_back(Warning{-69250, fmt::format("The tuple dimensions of {} [{}] do not match the AttributeMatrix {} tuple dimensions [{}]. This could result in a runtime error if the sizing remains the same at time of this filters execution. Proceed with caution.", possibleIArray.getName(), possibleIArray.getNumberofTuples(), possibleAM.getName(), possibleAM.getNumberofTuples())});
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
Result<> MoveData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
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

Result<Arguments> MoveData::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MoveData().getDefaultArguments();

  std::vector<Result<>> results;

  if(json[SIMPL::k_WhatToMoveKey].get<int32>() == 0)
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::SingleToMultiDataPathSelectionFilterParameterConverter>(args, json, SIMPL::k_AttributeMatrixSourceKey, k_Data_Key));
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_DataContainerDestinationKey, k_NewParent_Key));
  }
  else
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::SingleToMultiDataPathSelectionFilterParameterConverter>(args, json, SIMPL::k_DataArraySourceKey, k_Data_Key));
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_AttributeMatrixDestinationKey, k_NewParent_Key));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
