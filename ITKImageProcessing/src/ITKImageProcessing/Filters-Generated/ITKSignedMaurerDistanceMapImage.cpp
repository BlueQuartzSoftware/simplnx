#include "ITKSignedMaurerDistanceMapImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkSignedMaurerDistanceMapImageFilter.h>

using namespace complex;

namespace cxITKSignedMaurerDistanceMapImage
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;
template<class PixelT>
using FilterOutputT = float32;

struct ITKSignedMaurerDistanceMapImageFunctor
{
  bool insideIsPositive = false;
  bool squaredDistance = true;
  bool useImageSpacing = false;
  float64 backgroundValue = 0.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::SignedMaurerDistanceMapImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetInsideIsPositive(insideIsPositive);
    filter->SetSquaredDistance(squaredDistance);
    filter->SetUseImageSpacing(useImageSpacing);
    filter->SetBackgroundValue(backgroundValue);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKSignedMaurerDistanceMapImage::name() const
{
  return FilterTraits<ITKSignedMaurerDistanceMapImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKSignedMaurerDistanceMapImage::className() const
{
  return FilterTraits<ITKSignedMaurerDistanceMapImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSignedMaurerDistanceMapImage::uuid() const
{
  return FilterTraits<ITKSignedMaurerDistanceMapImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSignedMaurerDistanceMapImage::humanName() const
{
  return "ITK Signed Maurer Distance Map Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSignedMaurerDistanceMapImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKSignedMaurerDistanceMapImage", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKSignedMaurerDistanceMapImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_InsideIsPositive_Key, "InsideIsPositive", "", false));
  params.insert(std::make_unique<BoolParameter>(k_SquaredDistance_Key, "SquaredDistance", "", true));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 0.0));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}), GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, complex::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSignedMaurerDistanceMapImage::clone() const
{
  return std::make_unique<ITKSignedMaurerDistanceMapImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSignedMaurerDistanceMapImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto insideIsPositive = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKSignedMaurerDistanceMapImage::ArrayOptionsT, cxITKSignedMaurerDistanceMapImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSignedMaurerDistanceMapImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  
  auto insideIsPositive = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  cxITKSignedMaurerDistanceMapImage::ITKSignedMaurerDistanceMapImageFunctor itkFunctor = {insideIsPositive, squaredDistance, useImageSpacing, backgroundValue};

// LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath); 
// LINK GEOMETRY OUTPUT STOP
  
  return ITK::Execute<cxITKSignedMaurerDistanceMapImage::ArrayOptionsT, cxITKSignedMaurerDistanceMapImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
