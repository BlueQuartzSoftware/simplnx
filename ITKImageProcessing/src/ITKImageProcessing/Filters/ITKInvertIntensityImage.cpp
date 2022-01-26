#include "ITKInvertIntensityImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkInvertIntensityImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKInvertIntensityImageFunctor
{
  float64 maximum = 255;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::InvertIntensityImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetMaximum(maximum);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKInvertIntensityImage::name() const
{
  return FilterTraits<ITKInvertIntensityImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKInvertIntensityImage::className() const
{
  return FilterTraits<ITKInvertIntensityImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKInvertIntensityImage::uuid() const
{
  return FilterTraits<ITKInvertIntensityImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKInvertIntensityImage::humanName() const
{
  return "ITK::InvertIntensityImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKInvertIntensityImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKInvertIntensityImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKInvertIntensityImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_Maximum_Key, "Maximum", "", 255));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKInvertIntensityImage::clone() const
{
  return std::make_unique<ITKInvertIntensityImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKInvertIntensityImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto maximum = filterArgs.value<float64>(k_Maximum_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKInvertIntensityImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto maximum = filterArgs.value<float64>(k_Maximum_Key);

  ITKInvertIntensityImageFunctor itkFunctor = {maximum};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKInvertIntensityImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
