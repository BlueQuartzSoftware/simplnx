#include "ITKIntensityWindowingImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkIntensityWindowingImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKIntensityWindowingImageFunctor
{
  float64 windowMinimum = 0.0;
  float64 windowMaximum = 255.0;
  float64 outputMinimum = 0.0;
  float64 outputMaximum = 255.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::IntensityWindowingImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetWindowMinimum(windowMinimum);
    filter->SetWindowMaximum(windowMaximum);
    filter->SetOutputMinimum(outputMinimum);
    filter->SetOutputMaximum(outputMaximum);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKIntensityWindowingImage::name() const
{
  return FilterTraits<ITKIntensityWindowingImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKIntensityWindowingImage::className() const
{
  return FilterTraits<ITKIntensityWindowingImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKIntensityWindowingImage::uuid() const
{
  return FilterTraits<ITKIntensityWindowingImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKIntensityWindowingImage::humanName() const
{
  return "ITK::IntensityWindowingImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKIntensityWindowingImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKIntensityWindowingImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKIntensityWindowingImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_WindowMinimum_Key, "WindowMinimum", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_WindowMaximum_Key, "WindowMaximum", "", 255.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMinimum_Key, "OutputMinimum", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMaximum_Key, "OutputMaximum", "", 255.0));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKIntensityWindowingImage::clone() const
{
  return std::make_unique<ITKIntensityWindowingImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKIntensityWindowingImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto windowMinimum = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto windowMaximum = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKIntensityWindowingImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto windowMinimum = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto windowMaximum = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);

  ITKIntensityWindowingImageFunctor itkFunctor = {windowMinimum, windowMaximum, outputMinimum, outputMaximum};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKIntensityWindowingImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
