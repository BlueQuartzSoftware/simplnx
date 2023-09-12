#include "ITKAdaptiveHistogramEqualizationImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkAdaptiveHistogramEqualizationImageFilter.h>

using namespace complex;

namespace cxITKAdaptiveHistogramEqualizationImage
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
} // namespace cxITKAdaptiveHistogramEqualizationImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKAdaptiveHistogramEqualizationImage::name() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKAdaptiveHistogramEqualizationImage::className() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKAdaptiveHistogramEqualizationImage::uuid() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKAdaptiveHistogramEqualizationImage::humanName() const
{
  return "ITK Adaptive Histogram Equalization Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKAdaptiveHistogramEqualizationImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKAdaptiveHistogramEqualizationImage", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKAdaptiveHistogramEqualizationImage::parameters() const
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
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKAdaptiveHistogramEqualizationImage::clone() const
{
  return std::make_unique<ITKAdaptiveHistogramEqualizationImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKAdaptiveHistogramEqualizationImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto radius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);

  auto alpha = filterArgs.value<float32>(k_Alpha_Key);
  auto beta = filterArgs.value<float32>(k_Beta_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKAdaptiveHistogramEqualizationImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKAdaptiveHistogramEqualizationImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto radius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);

  auto alpha = filterArgs.value<float32>(k_Alpha_Key);
  auto beta = filterArgs.value<float32>(k_Beta_Key);

  const cxITKAdaptiveHistogramEqualizationImage::ITKAdaptiveHistogramEqualizationImageFunctor itkFunctor = {radius, alpha, beta};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKAdaptiveHistogramEqualizationImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
