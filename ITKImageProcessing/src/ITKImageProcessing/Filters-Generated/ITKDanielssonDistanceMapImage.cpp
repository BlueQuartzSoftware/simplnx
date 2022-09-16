#include "ITKDanielssonDistanceMapImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkDanielssonDistanceMapImageFilter.h>

using namespace complex;

namespace cxITKDanielssonDistanceMapImage
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = float32;

struct ITKDanielssonDistanceMapImageFunctor
{
  bool inputIsBinary = false;
  bool squaredDistance = false;
  bool useImageSpacing = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::DanielssonDistanceMapImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetInputIsBinary(inputIsBinary);
    filter->SetSquaredDistance(squaredDistance);
    filter->SetUseImageSpacing(useImageSpacing);
    return filter;
  }
};
} // namespace cxITKDanielssonDistanceMapImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKDanielssonDistanceMapImage::name() const
{
  return FilterTraits<ITKDanielssonDistanceMapImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKDanielssonDistanceMapImage::className() const
{
  return FilterTraits<ITKDanielssonDistanceMapImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKDanielssonDistanceMapImage::uuid() const
{
  return FilterTraits<ITKDanielssonDistanceMapImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKDanielssonDistanceMapImage::humanName() const
{
  return "ITK Danielsson Distance Map Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKDanielssonDistanceMapImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKDanielssonDistanceMapImage", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKDanielssonDistanceMapImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_InputIsBinary_Key, "InputIsBinary", "", false));
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
IFilter::UniquePointer ITKDanielssonDistanceMapImage::clone() const
{
  return std::make_unique<ITKDanielssonDistanceMapImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKDanielssonDistanceMapImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto inputIsBinary = filterArgs.value<bool>(k_InputIsBinary_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKDanielssonDistanceMapImage::ArrayOptionsT, cxITKDanielssonDistanceMapImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKDanielssonDistanceMapImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto inputIsBinary = filterArgs.value<bool>(k_InputIsBinary_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  cxITKDanielssonDistanceMapImage::ITKDanielssonDistanceMapImageFunctor itkFunctor = {inputIsBinary, squaredDistance, useImageSpacing};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKDanielssonDistanceMapImage::ArrayOptionsT, cxITKDanielssonDistanceMapImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                                      messageHandler);
}
} // namespace complex
