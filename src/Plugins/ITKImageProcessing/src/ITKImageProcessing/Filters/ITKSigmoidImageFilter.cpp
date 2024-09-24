#include "ITKSigmoidImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkSigmoidImageFilter.h>

using namespace nx::core;

namespace cxITKSigmoidImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKSigmoidImageFunctor
{
  float64 alpha = 1;
  float64 beta = 0;
  float64 outputMaximum = 255;
  float64 outputMinimum = 0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::SigmoidImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetAlpha(alpha);
    filter->SetBeta(beta);
    filter->SetOutputMaximum(outputMaximum);
    filter->SetOutputMinimum(outputMinimum);
    return filter;
  }
};
} // namespace cxITKSigmoidImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKSigmoidImageFilter::name() const
{
  return FilterTraits<ITKSigmoidImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKSigmoidImageFilter::className() const
{
  return FilterTraits<ITKSigmoidImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSigmoidImageFilter::uuid() const
{
  return FilterTraits<ITKSigmoidImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSigmoidImageFilter::humanName() const
{
  return "ITK Sigmoid Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSigmoidImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKSigmoidImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKSigmoidImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Alpha_Key, "Alpha", "The Alpha value from the Sigmoid equation. ", 1));
  params.insert(std::make_unique<Float64Parameter>(k_Beta_Key, "Beta", "The Beta value from teh sigmoid equation", 0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMaximum_Key, "Output Maximum", "The maximum output value", 255));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMinimum_Key, "Output Minimum", "The minimum output value", 0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSigmoidImageFilter::clone() const
{
  return std::make_unique<ITKSigmoidImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSigmoidImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto alpha = filterArgs.value<float64>(k_Alpha_Key);
  auto beta = filterArgs.value<float64>(k_Beta_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKSigmoidImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSigmoidImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto alpha = filterArgs.value<float64>(k_Alpha_Key);
  auto beta = filterArgs.value<float64>(k_Beta_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);

  const cxITKSigmoidImageFilter::ITKSigmoidImageFunctor itkFunctor = {alpha, beta, outputMaximum, outputMinimum};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKSigmoidImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_AlphaKey = "Alpha";
constexpr StringLiteral k_BetaKey = "Beta";
constexpr StringLiteral k_OutputMaximumKey = "OutputMaximum";
constexpr StringLiteral k_OutputMinimumKey = "OutputMinimum";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKSigmoidImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKSigmoidImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_AlphaKey, k_Alpha_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BetaKey, k_Beta_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutputMaximumKey, k_OutputMaximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutputMinimumKey, k_OutputMinimum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
