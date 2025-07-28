#include "ConvertOrientationsFilter.hpp"

#include "Algorithms/ConvertOrientations.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <EbsdLib/Core/EbsdDataArray.hpp>
#include <EbsdLib/OrientationMath/OrientationConverter.hpp>

#include <fmt/format.h>

#ifndef _MSC_VER
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
#endif
using namespace nx::core;

namespace
{
// Error Code constants
constexpr nx::core::int32 k_InputRepresentationTypeError = -67001;
constexpr nx::core::int32 k_OutputRepresentationTypeError = -67002;
constexpr nx::core::int32 k_InputComponentDimensionError = -67003;
constexpr nx::core::int32 k_InputComponentCountError = -67004;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ConvertOrientationsFilter::name() const
{
  return FilterTraits<ConvertOrientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertOrientationsFilter::className() const
{
  return FilterTraits<ConvertOrientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertOrientationsFilter::uuid() const
{
  return FilterTraits<ConvertOrientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertOrientationsFilter::humanName() const
{
  return "Convert Orientation Representation";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertOrientationsFilter::defaultTags() const
{
  return {className(), "Processing", "Conversion", "Orientation", "Quaternions", "Euler Angles", "Orientation Matrix", "Cubochoric", "Homochoric", "Rodrigues", "AxisAngle"};
}

//------------------------------------------------------------------------------
Parameters ConvertOrientationsFilter::parameters() const
{
  using OrientationConverterType = ebsdlib::OrientationConverter<EbsdDataArray<float>, float>;

  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_InputType_Key, "Input Orientation Type", "Specifies the incoming orientation representation enumeration index", 0,
                                                   OrientationConverterType::GetOrientationTypeStrings<ChoicesParameter::Choices>()));
  params.insert(std::make_unique<ChoicesParameter>(k_OutputType_Key, "Output Orientation Type",
                                                   "Specifies to which orientation representation to convert the incoming data. Specified as an enumeration index", 1,
                                                   OrientationConverterType::GetOrientationTypeStrings<ChoicesParameter::Choices>()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputOrientationArrayPath_Key, "Input Orientations", "The complete path to the incoming orientation representation data array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputOrientationArrayName_Key, "Output Orientations", "The name of the data array with the converted orientation representation", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ConvertOrientationsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertOrientationsFilter::clone() const
{
  return std::make_unique<ConvertOrientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertOrientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputType = static_cast<ebsdlib::orientations::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key));
  auto outputType = static_cast<ebsdlib::orientations::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key));

  if(static_cast<int>(inputType) < 0 || inputType >= ebsdlib::orientations::Type::Unknown)
  {
    return {MakeErrorResult<OutputActions>(convert_orientations_constants::k_InputRepresentationTypeError,
                                           fmt::format("Input Representation Type must be a value from 0 to 6. '{}'", fmt::underlying(inputType)))};
  }

  if(static_cast<int>(outputType) < 0 || outputType >= ebsdlib::orientations::Type::Unknown)
  {
    return {MakeErrorResult<OutputActions>(convert_orientations_constants::k_OutputRepresentationTypeError,
                                           fmt::format("Output Representation Type must be a value from 0 to 6. '{}'", fmt::underlying(outputType)))};
  }

  if(inputType == outputType)
  {
    return {MakeErrorResult<OutputActions>(convert_orientations_constants::k_MatchingTypesError,
                                           fmt::format("The Input Representation Type and the Output Representation Type cannot be the same!", fmt::underlying(outputType)))};
  }

  auto pInputArrayPath = filterArgs.value<DataPath>(k_InputOrientationArrayPath_Key);
  const auto* inputArray = dataStructure.getDataAs<IDataArray>(pInputArrayPath);
  std::vector<size_t> inputCompShape = inputArray->getIDataStore()->getComponentShape();

  if(inputCompShape.size() > 1)
  {
    return {MakeErrorResult<OutputActions>(convert_orientations_constants::k_InputComponentDimensionError,
                                           fmt::format("Input Component Shape has multiple dimensions. It can only have 1 dimension. '{}'", inputCompShape.size()))};
  }
  using OrientationConverterType = ebsdlib::OrientationConverter<EbsdDataArray<float>, float>;
  auto representationNames = OrientationConverterType::GetOrientationTypeStrings<std::vector<std::string>>();
  auto representationElementCount = OrientationConverterType::GetComponentCounts<std::vector<size_t>>();

  if(inputCompShape[0] != representationElementCount[static_cast<size_t>(inputType)])
  {
    std::stringstream message;
    message << "Number of components for input array is not correct for input representation type. " << representationNames[static_cast<size_t>(inputType)] << " should have "
            << representationElementCount[static_cast<size_t>(inputType)] << " components but the selected input array has " << inputCompShape[0];
    return {MakeErrorResult<OutputActions>(convert_orientations_constants::k_InputComponentCountError, message.str())};
  }
  auto pOutputArrayPath = pInputArrayPath.replaceName(filterArgs.value<std::string>(k_OutputOrientationArrayName_Key));

  auto numericType = static_cast<DataType>(inputArray->getDataType());
  std::vector<size_t> componentDims = {representationElementCount[static_cast<size_t>(outputType)]};
  auto action = std::make_unique<CreateArrayAction>(numericType, inputArray->getIDataStore()->getTupleShape(), componentDims, pOutputArrayPath);

  // Create the Output actions
  OutputActions actions;
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ConvertOrientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ConvertOrientationsInputValues inputValues;
  // Replace the keys below with the variables from the header.
  inputValues.InputOrientationArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_InputOrientationArrayPath_Key);
  inputValues.InputType = static_cast<ebsdlib::orientations::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key));
  inputValues.OutputOrientationArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputOrientationArrayName_Key);
  inputValues.OutputType = static_cast<ebsdlib::orientations::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key));
  return ConvertOrientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

#ifndef _MSC_VER
#pragma clang diagnostic pop
#endif // !_MSVC_

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputTypeKey = "InputType";
constexpr StringLiteral k_OutputTypeKey = "OutputType";
constexpr StringLiteral k_InputOrientationArrayPathKey = "InputOrientationArrayPath";
constexpr StringLiteral k_OutputOrientationArrayNameKey = "OutputOrientationArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ConvertOrientationsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ConvertOrientationsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_InputTypeKey, k_InputType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_OutputTypeKey, k_OutputType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_InputOrientationArrayPathKey, k_InputOrientationArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_OutputOrientationArrayNameKey, k_OutputOrientationArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
