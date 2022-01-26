#include "ITKHConvexImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkHConvexImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKHConvexImageFunctor
{
  float64 height = 2.0;
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::HConvexImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetHeight(height);
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKHConvexImage::name() const
{
  return FilterTraits<ITKHConvexImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKHConvexImage::className() const
{
  return FilterTraits<ITKHConvexImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKHConvexImage::uuid() const
{
  return FilterTraits<ITKHConvexImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKHConvexImage::humanName() const
{
  return "ITK::HConvexImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKHConvexImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKHConvexImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKHConvexImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_Height_Key, "Height", "", 2.0));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKHConvexImage::clone() const
{
  return std::make_unique<ITKHConvexImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKHConvexImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKHConvexImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  ITKHConvexImageFunctor itkFunctor = {height, fullyConnected};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKHConvexImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
