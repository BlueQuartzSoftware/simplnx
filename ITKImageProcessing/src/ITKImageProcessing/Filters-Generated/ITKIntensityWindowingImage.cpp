#include "ITKIntensityWindowingImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkIntensityWindowingImageFilter.h>

using namespace complex;

namespace cxITKIntensityWindowingImage
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
} // namespace cxITKIntensityWindowingImage

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
  return "ITK Intensity Windowing Image Filter";
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
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_WindowMinimum_Key, "WindowMinimum", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_WindowMaximum_Key, "WindowMaximum", "", 255.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMinimum_Key, "OutputMinimum", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMaximum_Key, "OutputMaximum", "", 255.0));

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
IFilter::UniquePointer ITKIntensityWindowingImage::clone() const
{
  return std::make_unique<ITKIntensityWindowingImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKIntensityWindowingImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto windowMinimum = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto windowMaximum = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKIntensityWindowingImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKIntensityWindowingImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto windowMinimum = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto windowMaximum = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);

  cxITKIntensityWindowingImage::ITKIntensityWindowingImageFunctor itkFunctor = {windowMinimum, windowMaximum, outputMinimum, outputMaximum};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKIntensityWindowingImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
