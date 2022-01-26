#include "ITKHMaximaImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkHMaximaImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKHMaximaImageFunctor
{
  float64 height = 2.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::HMaximaImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetHeight(height);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKHMaximaImage::name() const
{
  return FilterTraits<ITKHMaximaImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKHMaximaImage::className() const
{
  return FilterTraits<ITKHMaximaImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKHMaximaImage::uuid() const
{
  return FilterTraits<ITKHMaximaImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKHMaximaImage::humanName() const
{
  return "ITK::HMaximaImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKHMaximaImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKHMaximaImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKHMaximaImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_Height_Key, "Height", "", 2.0));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKHMaximaImage::clone() const
{
  return std::make_unique<ITKHMaximaImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKHMaximaImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKHMaximaImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);

  ITKHMaximaImageFunctor itkFunctor = {height};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKHMaximaImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
