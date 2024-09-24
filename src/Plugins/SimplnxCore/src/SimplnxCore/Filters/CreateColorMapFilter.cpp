#include "CreateColorMapFilter.hpp"

#include "Algorithms/CreateColorMap.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/CreateColorMapParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

using namespace nx::core;
namespace
{
constexpr int32 k_MissingGeomError = -72440;
constexpr int32 k_IncorrectInputArray = -72441;
constexpr int32 k_MissingInputArray = -72442;
constexpr int32 k_MissingOrIncorrectGoodVoxelsArray = -72443;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CreateColorMapFilter::name() const
{
  return FilterTraits<CreateColorMapFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateColorMapFilter::className() const
{
  return FilterTraits<CreateColorMapFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateColorMapFilter::uuid() const
{
  return FilterTraits<CreateColorMapFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateColorMapFilter::humanName() const
{
  return "Create Color Map";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateColorMapFilter::defaultTags() const
{
  return {className(), "Core", "Image", "Color Table", "Generate"};
}

//------------------------------------------------------------------------------
Parameters CreateColorMapFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<CreateColorMapParameter>(k_SelectedPreset_Key, "Select Preset...", "Select a preset color scheme to apply to the created array",
                                                          ColorTableUtilities::GetDefaultRGBPresetName()));
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedDataArrayPath_Key, "Data Array",
                                                          "The complete path to the data array from which to create the rgb array by applying the selected preset color scheme", DataPath{},
                                                          nx::core::GetAllDataTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Whether to assign a black color to 'bad' Elements", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Path to the data array used to define Elements as good or bad.", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<VectorUInt8Parameter>(k_InvalidColorValue_Key, "Masked Color (RGB)", "The color to assign to voxels that have a mask value of FALSE",
                                                       VectorUInt8Parameter::ValueType{0, 0, 0}, std::vector<std::string>{"Red", "Green", "Blue"}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_UseMask_Key, k_InvalidColorValue_Key, true);

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_RgbArrayPath_Key, "Output RGB Array", "The rgb array created by normalizing each element of the input array and converting to a color based on the selected preset color scheme", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateColorMapFilter::clone() const
{
  return std::make_unique<CreateColorMapFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateColorMapFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pSelectedDataArrayPathValue = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  auto pRgbArrayPathValue = pSelectedDataArrayPathValue.replaceName(filterArgs.value<std::string>(k_RgbArrayPath_Key));

  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(pSelectedDataArrayPathValue);
  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, dataArray.getTupleShape(), std::vector<usize>{3}, pRgbArrayPathValue);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  std::vector<DataPath> dataPaths;
  dataPaths.push_back(pSelectedDataArrayPathValue);

  // Validate the GoodVoxels/Mask Array combination
  DataPath goodVoxelsPath;
  if(pUseGoodVoxelsValue)
  {
    goodVoxelsPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

    const auto* goodVoxelsArray = dataStructure.getDataAs<IDataArray>(goodVoxelsPath);
    if(nullptr == goodVoxelsArray)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array is not located at path: '{}'", goodVoxelsPath.toString())}})};
    }

    if(goodVoxelsArray->getDataType() != DataType::boolean && goodVoxelsArray->getDataType() != DataType::uint8)
    {
      return {nonstd::make_unexpected(
          std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array at path '{}' is not of the correct type. It must be Bool or UInt8", goodVoxelsPath.toString())}})};
    }
    dataPaths.push_back(goodVoxelsPath);
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateColorMapFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  CreateColorMapInputValues inputValues;

  inputValues.PresetName = filterArgs.value<CreateColorMapParameter::ValueType>(k_SelectedPreset_Key);
  inputValues.SelectedDataArrayPath = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  inputValues.RgbArrayPath = inputValues.SelectedDataArrayPath.replaceName(filterArgs.value<std::string>(k_RgbArrayPath_Key));
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.InvalidColor = filterArgs.value<std::vector<uint8>>(k_InvalidColorValue_Key);

  return CreateColorMap(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedPresetNameKey = "SelectedPresetName";
constexpr StringLiteral k_SelectedDataArrayPathKey = "SelectedDataArrayPath";
constexpr StringLiteral k_RgbArrayNameKey = "RgbArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> CreateColorMapFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateColorMapFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_SelectedPresetNameKey, k_SelectedPreset_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedDataArrayPathKey, k_SelectedDataArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_RgbArrayNameKey, k_RgbArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
