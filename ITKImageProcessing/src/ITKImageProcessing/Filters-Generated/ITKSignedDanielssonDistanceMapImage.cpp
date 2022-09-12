#include "ITKSignedDanielssonDistanceMapImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkSignedDanielssonDistanceMapImageFilter.h>

using namespace complex;

namespace cxITKSignedDanielssonDistanceMapImage
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = float32;

struct ITKSignedDanielssonDistanceMapImageFunctor
{
  bool insideIsPositive = false;
  bool squaredDistance = false;
  bool useImageSpacing = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::SignedDanielssonDistanceMapImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetInsideIsPositive(insideIsPositive);
    filter->SetSquaredDistance(squaredDistance);
    filter->SetUseImageSpacing(useImageSpacing);
    return filter;
  }
};
} // namespace cxITKSignedDanielssonDistanceMapImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKSignedDanielssonDistanceMapImage::name() const
{
  return FilterTraits<ITKSignedDanielssonDistanceMapImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKSignedDanielssonDistanceMapImage::className() const
{
  return FilterTraits<ITKSignedDanielssonDistanceMapImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSignedDanielssonDistanceMapImage::uuid() const
{
  return FilterTraits<ITKSignedDanielssonDistanceMapImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSignedDanielssonDistanceMapImage::humanName() const
{
  return "ITK Signed Danielsson Distance Map Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSignedDanielssonDistanceMapImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKSignedDanielssonDistanceMapImage", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKSignedDanielssonDistanceMapImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_InsideIsPositive_Key, "InsideIsPositive", "", false));
  params.insert(std::make_unique<BoolParameter>(k_SquaredDistance_Key, "SquaredDistance", "", false));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing", "", false));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSignedDanielssonDistanceMapImage::clone() const
{
  return std::make_unique<ITKSignedDanielssonDistanceMapImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSignedDanielssonDistanceMapImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto insideIsPositive = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKSignedDanielssonDistanceMapImage::ArrayOptionsT, cxITKSignedDanielssonDistanceMapImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSignedDanielssonDistanceMapImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto insideIsPositive = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  cxITKSignedDanielssonDistanceMapImage::ITKSignedDanielssonDistanceMapImageFunctor itkFunctor = {insideIsPositive, squaredDistance, useImageSpacing};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKSignedDanielssonDistanceMapImage::ArrayOptionsT, cxITKSignedDanielssonDistanceMapImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                                  itkFunctor);
}
} // namespace complex
