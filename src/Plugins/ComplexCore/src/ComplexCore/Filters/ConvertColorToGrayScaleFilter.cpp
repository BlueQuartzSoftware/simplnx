#include "ConvertColorToGrayScaleFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ConvertColorToGrayScale.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
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

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_ConversionAlgorithm_Key, "Conversion Algorithm", "Which method to use when flattening the RGB array", 0,
                                                                    ChoicesParameter::Choices{"Luminosity", "Average", "Lightness", "SingleChannel"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ColorWeights_Key, "Color Weighting", "The weightings for each R|G|B component when using the luminosity conversion algorithm",
                                                         std::vector<float32>{0.2125F, 0.7154F, 0.0721F}, std::vector<std::string>{"Red", "Green", "Blue"}));
  params.insert(std::make_unique<Int32Parameter>(k_ColorChannel_Key, "Color Channel", "The specific R|G|B channel to use as the GrayScale values", 0));
  params.linkParameters(k_ConversionAlgorithm_Key, k_ColorWeights_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_ConversionAlgorithm_Key, k_ColorChannel_Key, std::make_any<ChoicesParameter::ValueType>(3));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InputDataArrayVector_Key, "Input Data Arrays", "Select all DataArrays that need to be converted to GrayScale",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{DataType::uint8}));
  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<StringParameter>(k_OutputArrayPrefix_Key, "Output Data Array Prefix",
                                                  "This prefix will be added to each array name that is selected for conversion to form the new array name", "Grayscale_"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertColorToGrayScaleFilter::clone() const
{
  return std::make_unique<ConvertColorToGrayScaleFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertColorToGrayScaleFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto pConversionAlgorithmValue = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionAlgorithm_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pColorChannelValue = filterArgs.value<int32>(k_ColorChannel_Key);
  auto inputDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputDataArrayVector_Key);
  auto outputArrayPrefix = filterArgs.value<StringParameter::ValueType>(k_OutputArrayPrefix_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(pConversionAlgorithmValue > 3)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{
        -10701, fmt::format("Conversion Algorithm choice is invalid. Valid values are 0=Luminosity, 1=Average, 2=Lightness, 3=SingleChannel. Value supplied is {}", pConversionAlgorithmValue)}})};
  }

  if(pConversionAlgorithmValue == 3 && pColorChannelValue > 3)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-10701, fmt::format("Color channel selection is invalid. Valid values are 0, 1, 2. Value supplied is {}", pColorChannelValue)}})};
  }

  if(pConversionAlgorithmValue == 0)
  {
    if(pColorWeightsValue[0] < 0.0F || pColorWeightsValue[1] < 0.0F || pColorWeightsValue[2] < 0.0F)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-10704, "1 of more of the Color Weight values have a negative value. This is not allowed."}})};
    }

    float colorWeightSum = pColorWeightsValue[0] + pColorWeightsValue[1] + pColorWeightsValue[2];
    if(colorWeightSum < .9800 || colorWeightSum > 1.02)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-10704, fmt::format("Color Weight values should sum up to 1.0. Current sum is {}", colorWeightSum)}})};
    }
  }

  if(inputDataArrayPaths.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-10705, fmt::format("No input arrays selected for conversion.")}})};
  }

  DataPath outputDataArrayPath;
  for(const auto& inputDataArrayPath : inputDataArrayPaths)
  {
    const auto* inputArray = dataStructure.getDataAs<IDataArray>(inputDataArrayPath);
    if(inputArray == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-10700, fmt::format("Input Data Array does not exist at DataPath {}", inputDataArrayPath.toString())}})};
    }
    std::vector<std::string> inputPathVector = inputDataArrayPath.getPathVector();
    std::string inputArrayName = inputDataArrayPath.getTargetName();
    std::string outputArrayName = fmt::format("{}{}", outputArrayPrefix, inputArrayName);
    inputPathVector.back() = outputArrayName;
    outputDataArrayPath = DataPath(inputPathVector);
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(complex::DataType::uint8, inputArray->getIDataStoreRef().getTupleShape(), std::vector<usize>(1, 1), outputDataArrayPath));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ConvertColorToGrayScaleFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  ConvertColorToGrayScaleInputValues inputValues;

  inputValues.ConversionAlgorithm = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionAlgorithm_Key);
  inputValues.ColorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.ColorChannel = filterArgs.value<int32>(k_ColorChannel_Key);
  inputValues.InputDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputDataArrayVector_Key);
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

  return ConvertColorToGrayScale(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
