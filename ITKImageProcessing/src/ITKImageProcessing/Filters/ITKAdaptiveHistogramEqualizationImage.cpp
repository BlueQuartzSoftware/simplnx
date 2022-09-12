#include "ITKAdaptiveHistogramEqualizationImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkAdaptiveHistogramEqualizationImageFilter.h>

using namespace complex;

namespace cxITKAdaptiveHistogramEqualizationImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKAdaptiveHistogramEqualizationImageFunctor
{
  using InputRadiusType = std::vector<float>;
  InputRadiusType radius = std::vector<float>(3, 5.0F);
  float32 alpha = 0.3f;
  float32 beta = 0.3f;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::AdaptiveHistogramEqualizationImageFilter<InputImageT>;
    auto filter = FilterT::New();
    // Set the Radius Filter Property
    using RadiusType = typename FilterT::RadiusType;
    auto convertedRadius = ITK::CastVec3ToITK<RadiusType, typename RadiusType::SizeValueType>(radius, RadiusType::Dimension);
    filter->SetRadius(convertedRadius);
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
  return {"ITKImageProcessing", "ITKAdaptiveHistogramEqualizationImage", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKAdaptiveHistogramEqualizationImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Radius_Key, "Radius", "Radius Dimensions XYZ", std::vector<float>(3, 5.0F), std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(std::make_unique<Float32Parameter>(k_Alpha_Key, "Alpha", "", 0.3f));
  params.insert(std::make_unique<Float32Parameter>(k_Beta_Key, "Beta", "", 0.3f));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

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
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto radius = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Radius_Key);

  auto alpha = filterArgs.value<float32>(k_Alpha_Key);
  auto beta = filterArgs.value<float32>(k_Beta_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKAdaptiveHistogramEqualizationImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKAdaptiveHistogramEqualizationImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto radius = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Radius_Key);

  auto alpha = filterArgs.value<float32>(k_Alpha_Key);
  auto beta = filterArgs.value<float32>(k_Beta_Key);

  cxITKAdaptiveHistogramEqualizationImage::ITKAdaptiveHistogramEqualizationImageFunctor itkFunctor = {radius, alpha, beta};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKAdaptiveHistogramEqualizationImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
