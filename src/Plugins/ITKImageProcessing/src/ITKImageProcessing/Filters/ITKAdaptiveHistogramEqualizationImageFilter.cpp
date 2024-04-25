#include "ITKAdaptiveHistogramEqualizationImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkAdaptiveHistogramEqualizationImageFilter.h>

using namespace nx::core;

namespace cxITKAdaptiveHistogramEqualizationImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKAdaptiveHistogramEqualizationImageFunctor
{
  using RadiusInputRadiusType = std::vector<uint32>;
  RadiusInputRadiusType radius = std::vector<unsigned int>(3, 5);
  float32 alpha = 0.3f;
  float32 beta = 0.3f;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::AdaptiveHistogramEqualizationImageFilter<InputImageT>;
    auto filter = FilterType::New();
    // Set the Radius Filter Property
    {
      using RadiusType = typename FilterType::RadiusType;
      auto convertedRadius = ITK::CastVec3ToITK<RadiusType, typename RadiusType::SizeValueType>(radius, RadiusType::Dimension);
      filter->SetRadius(convertedRadius);
    }

    filter->SetAlpha(alpha);
    filter->SetBeta(beta);
    return filter;
  }
};
} // namespace cxITKAdaptiveHistogramEqualizationImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKAdaptiveHistogramEqualizationImageFilter::name() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKAdaptiveHistogramEqualizationImageFilter::className() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKAdaptiveHistogramEqualizationImageFilter::uuid() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKAdaptiveHistogramEqualizationImageFilter::humanName() const
{
  return "ITK Adaptive Histogram Equalization Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKAdaptiveHistogramEqualizationImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKAdaptiveHistogramEqualizationImage", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKAdaptiveHistogramEqualizationImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorUInt32Parameter>(k_Radius_Key, "Radius", "Radius Dimensions XYZ", std::vector<unsigned int>(3, 5), std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(std::make_unique<Float32Parameter>(
      k_Alpha_Key, "Alpha", "Set/Get the value of alpha. Alpha = 0 produces the adaptive histogram equalization (provided beta=0). Alpha = 1 produces an unsharp mask. Default is 0.3.", 0.3f));
  params.insert(std::make_unique<Float32Parameter>(
      k_Beta_Key, "Beta",
      "Set/Get the value of beta. If beta = 1 (and alpha = 1), then the output image matches the input image. As beta approaches 0, the filter behaves as an unsharp mask. Default is 0.3.", 0.3f));

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
IFilter::UniquePointer ITKAdaptiveHistogramEqualizationImageFilter::clone() const
{
  return std::make_unique<ITKAdaptiveHistogramEqualizationImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKAdaptiveHistogramEqualizationImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto radius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);

  auto alpha = filterArgs.value<float32>(k_Alpha_Key);
  auto beta = filterArgs.value<float32>(k_Beta_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKAdaptiveHistogramEqualizationImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKAdaptiveHistogramEqualizationImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto radius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);

  auto alpha = filterArgs.value<float32>(k_Alpha_Key);
  auto beta = filterArgs.value<float32>(k_Beta_Key);

  const cxITKAdaptiveHistogramEqualizationImageFilter::ITKAdaptiveHistogramEqualizationImageFunctor itkFunctor = {radius, alpha, beta};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKAdaptiveHistogramEqualizationImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
