#include "ITKLabelContourImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkLabelContourImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;

struct ITKLabelContourImageFunctor
{
  bool fullyConnected = false;
  float64 backgroundValue = 0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::LabelContourImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetFullyConnected(fullyConnected);
    filter->SetBackgroundValue(backgroundValue);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKLabelContourImage::name() const
{
  return FilterTraits<ITKLabelContourImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKLabelContourImage::className() const
{
  return FilterTraits<ITKLabelContourImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKLabelContourImage::uuid() const
{
  return FilterTraits<ITKLabelContourImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKLabelContourImage::humanName() const
{
  return "ITK::LabelContourImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKLabelContourImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKLabelContourImage", "ITKImageLabel", "ImageLabel"};
}

//------------------------------------------------------------------------------
Parameters ITKLabelContourImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 0));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKLabelContourImage::clone() const
{
  return std::make_unique<ITKLabelContourImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKLabelContourImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKLabelContourImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  ITKLabelContourImageFunctor itkFunctor = {fullyConnected, backgroundValue};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKLabelContourImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
