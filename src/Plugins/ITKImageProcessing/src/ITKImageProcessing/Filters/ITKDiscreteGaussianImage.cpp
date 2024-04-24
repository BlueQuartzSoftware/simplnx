#include "ITKDiscreteGaussianImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkDiscreteGaussianImageFilter.h>

using namespace nx::core;

namespace cxITKDiscreteGaussianImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKDiscreteGaussianImageFunctor
{
  using VarianceInputArrayType = std::vector<float64>;
  VarianceInputArrayType variance = std::vector<double>(3, 1.0);
  uint32 maximumKernelWidth = 32u;
  using MaximumErrorInputArrayType = std::vector<float64>;
  MaximumErrorInputArrayType maximumError = std::vector<double>(3, 0.01);
  bool useImageSpacing = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::DiscreteGaussianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetVariance(variance.data()); // Set the Variance Filter Property
    filter->SetMaximumKernelWidth(maximumKernelWidth);
    filter->SetMaximumError(maximumError.data()); // Set the MaximumError Filter Property
    filter->SetUseImageSpacing(useImageSpacing);
    return filter;
  }
};
} // namespace cxITKDiscreteGaussianImage

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKDiscreteGaussianImage::name() const
{
  return FilterTraits<ITKDiscreteGaussianImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKDiscreteGaussianImage::className() const
{
  return FilterTraits<ITKDiscreteGaussianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKDiscreteGaussianImage::uuid() const
{
  return FilterTraits<ITKDiscreteGaussianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKDiscreteGaussianImage::humanName() const
{
  return "ITK Discrete Gaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKDiscreteGaussianImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKDiscreteGaussianImage", "ITKSmoothing", "Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKDiscreteGaussianImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(
      std::make_unique<VectorFloat64Parameter>(k_Variance_Key, "Variance", "The value of the input variance for each axis", std::vector<double>(3, 1.0), std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(std::make_unique<UInt32Parameter>(k_MaximumKernelWidth_Key, "MaximumKernelWidth",
                                                  "Set the kernel to be no wider than MaximumKernelWidth pixels, even if MaximumError demands it. The default is 32 pixels.", 32u));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_MaximumError_Key, "MaximumError", "The maximum error for each axis", std::vector<double>(3, 0.01), std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(
      std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing",
                                      "Set/Get whether or not the filter will use the spacing of the input image in its calculations. Use On to take the image spacing information into account and to "
                                      "specify the Gaussian variance in real world units; use Off to ignore the image spacing and to specify the Gaussian variance in voxel units. Default is On.",
                                      true));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Array Name",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKDiscreteGaussianImage::clone() const
{
  return std::make_unique<ITKDiscreteGaussianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKDiscreteGaussianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto variance = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Variance_Key);

  auto maximumKernelWidth = filterArgs.value<uint32>(k_MaximumKernelWidth_Key);
  auto maximumError = filterArgs.value<VectorFloat64Parameter::ValueType>(k_MaximumError_Key);

  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKDiscreteGaussianImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKDiscreteGaussianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto variance = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Variance_Key);

  auto maximumKernelWidth = filterArgs.value<uint32>(k_MaximumKernelWidth_Key);
  auto maximumError = filterArgs.value<VectorFloat64Parameter::ValueType>(k_MaximumError_Key);

  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  const cxITKDiscreteGaussianImage::ITKDiscreteGaussianImageFunctor itkFunctor = {variance, maximumKernelWidth, maximumError, useImageSpacing};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKDiscreteGaussianImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_VarianceKey = "Variance";
constexpr StringLiteral k_MaximumKernelWidthKey = "MaximumKernelWidth";
constexpr StringLiteral k_MaximumErrorKey = "MaximumError";
constexpr StringLiteral k_UseImageSpacingKey = "UseImageSpacing";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKDiscreteGaussianImage::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKDiscreteGaussianImage().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleVec3FilterParameterConverter>(args, json, SIMPL::k_VarianceKey, k_Variance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint32>>(args, json, SIMPL::k_MaximumKernelWidthKey, k_MaximumKernelWidth_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleVec3FilterParameterConverter>(args, json, SIMPL::k_MaximumErrorKey, k_MaximumError_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UseImageSpacingKey, k_UseImageSpacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
