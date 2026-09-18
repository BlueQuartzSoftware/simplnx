#include "ConvertColorToGrayScaleFilter.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ColorToGrayScale.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ConvertColorToGrayScaleFilter::name() const
{
  return FilterTraits<ConvertColorToGrayScaleFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertColorToGrayScaleFilter::className() const
{
  return FilterTraits<ConvertColorToGrayScaleFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertColorToGrayScaleFilter::uuid() const
{
  return FilterTraits<ConvertColorToGrayScaleFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertColorToGrayScaleFilter::humanName() const
{
  return "Color to GrayScale";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertColorToGrayScaleFilter::defaultTags() const
{
  return {className(), "Core", "Image"};
}

//------------------------------------------------------------------------------
Parameters ConvertColorToGrayScaleFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_ConversionAlgorithm_Key, "Conversion Algorithm", "Which method to use when flattening the RGB array", 0,
                                                                    ChoicesParameter::Choices{"Luminosity", "Average", "Lightness", "SingleChannel"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ColorWeights_Key, "Color Weighting", "The weightings for each R|G|B component when using the luminosity conversion algorithm",
                                                         std::vector<float32>{0.2125F, 0.7154F, 0.0721F}, std::vector<std::string>{"Red", "Green", "Blue"}));
  params.insert(std::make_unique<Int32Parameter>(k_ColorChannel_Key, "Color Channel", "The specific R|G|B channel to use as the GrayScale values", 0));
  params.linkParameters(k_ConversionAlgorithm_Key, k_ColorWeights_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_ConversionAlgorithm_Key, k_ColorChannel_Key, std::make_any<ChoicesParameter::ValueType>(3));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InputDataArrayPath_Key, "Input Data Arrays", "Select all DataArrays that need to be converted to GrayScale",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{DataType::uint8}, MultiArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<StringParameter>(k_OutputArrayPrefix_Key, "Output Data Array Prefix",
                                                  "This prefix will be added to each array name that is selected for conversion to form the new array name", "Grayscale_"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ConvertColorToGrayScaleFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertColorToGrayScaleFilter::clone() const
{
  return std::make_unique<ConvertColorToGrayScaleFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertColorToGrayScaleFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ConvertColorToGrayScaleInputValues inputValues;
  inputValues.ConversionAlgorithm = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionAlgorithm_Key);
  inputValues.ColorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.ColorChannel = filterArgs.value<int32>(k_ColorChannel_Key);
  inputValues.InputDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputDataArrayPath_Key);
  inputValues.OutputArrayPrefix = filterArgs.value<StringParameter::ValueType>(k_OutputArrayPrefix_Key);
  return PreflightColorToGrayScale(dataStructure, inputValues);
}

//------------------------------------------------------------------------------
Result<> ConvertColorToGrayScaleFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ConvertColorToGrayScaleInputValues inputValues;

  inputValues.ConversionAlgorithm = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionAlgorithm_Key);
  inputValues.ColorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.ColorChannel = filterArgs.value<int32>(k_ColorChannel_Key);
  inputValues.InputDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputDataArrayPath_Key);
  inputValues.OutputArrayPrefix = filterArgs.value<StringParameter::ValueType>(k_OutputArrayPrefix_Key);

  for(const auto& inputDataArrayPath : inputValues.InputDataArrayPaths)
  {
    DataPath outputDataArrayPath;
    std::vector<std::string> inputPathVector = inputDataArrayPath.getPathVector();
    std::string inputArrayName = inputDataArrayPath.getTargetName();
    std::string outputArrayName = fmt::format("{}{}", inputValues.OutputArrayPrefix, inputArrayName);
    inputPathVector.back() = outputArrayName;
    outputDataArrayPath = DataPath(inputPathVector);
    inputValues.OutputDataArrayPaths.push_back(outputDataArrayPath);
  }

  return ConvertColorToGrayScaleArrays(dataStructure, inputValues, messageHandler, shouldCancel);
}
} // namespace nx::core

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ConversionAlgorithmKey = "ConversionAlgorithm";
constexpr StringLiteral k_ColorWeightsKey = "ColorWeights";
constexpr StringLiteral k_ColorChannelKey = "ColorChannel";
constexpr StringLiteral k_InputDataArrayVectorKey = "InputDataArrayVector";
constexpr StringLiteral k_OutputArrayPrefixKey = "OutputArrayPrefix";
} // namespace SIMPL
} // namespace

Result<Arguments> ConvertColorToGrayScaleFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ConvertColorToGrayScaleFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_ConversionAlgorithmKey, k_ConversionAlgorithm_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_ColorWeightsKey, k_ColorWeights_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_ColorChannelKey, k_ColorChannel_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_InputDataArrayVectorKey, k_InputDataArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_OutputArrayPrefixKey, k_OutputArrayPrefix_Key));

  // Create new attribute matrix and output attribute matrix name parameters are not applicable to NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
