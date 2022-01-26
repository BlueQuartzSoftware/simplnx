#include "ITKMedianImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkMedianImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKMedianImageFunctor
{
  std::vector<uint64> radius = std::vector<uint64>(3, 1);

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::MedianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    using RadiusType = typename FilterT::RadiusType;
    auto convertedRadius = ITK::CastVec3ToITK<RadiusType, typename RadiusType::SizeValueType>(radius, RadiusType::Dimension);
    filter->SetRadius(convertedRadius);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMedianImage::name() const
{
  return FilterTraits<ITKMedianImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMedianImage::className() const
{
  return FilterTraits<ITKMedianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMedianImage::uuid() const
{
  return FilterTraits<ITKMedianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMedianImage::humanName() const
{
  return "ITK::MedianImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMedianImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKMedianImage", "ITKSmoothing", "Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKMedianImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_Radius_Key, "Radius", "", std::vector<uint64>(3, 1), std::vector<std::string>(3)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMedianImage::clone() const
{
  return std::make_unique<ITKMedianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMedianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto radius = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Radius_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMedianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto radius = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Radius_Key);

  ITKMedianImageFunctor itkFunctor = {radius};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
