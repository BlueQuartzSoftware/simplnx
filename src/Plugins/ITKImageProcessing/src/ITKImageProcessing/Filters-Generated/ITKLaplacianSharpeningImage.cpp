#include "ITKLaplacianSharpeningImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkLaplacianSharpeningImageFilter.h>

using namespace complex;

namespace cxITKLaplacianSharpeningImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

struct ITKLaplacianSharpeningImageFunctor
{
  bool useImageSpacing = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::LaplacianSharpeningImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetUseImageSpacing(useImageSpacing);
    return filter;
  }
};
} // namespace cxITKLaplacianSharpeningImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKLaplacianSharpeningImage::name() const
{
  return FilterTraits<ITKLaplacianSharpeningImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKLaplacianSharpeningImage::className() const
{
  return FilterTraits<ITKLaplacianSharpeningImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKLaplacianSharpeningImage::uuid() const
{
  return FilterTraits<ITKLaplacianSharpeningImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKLaplacianSharpeningImage::humanName() const
{
  return "ITK Laplacian Sharpening Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKLaplacianSharpeningImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKLaplacianSharpeningImage", "ITKImageFeature", "ImageFeature"};
}

//------------------------------------------------------------------------------
Parameters ITKLaplacianSharpeningImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing", "", true));

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
IFilter::UniquePointer ITKLaplacianSharpeningImage::clone() const
{
  return std::make_unique<ITKLaplacianSharpeningImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKLaplacianSharpeningImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKLaplacianSharpeningImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKLaplacianSharpeningImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  cxITKLaplacianSharpeningImage::ITKLaplacianSharpeningImageFunctor itkFunctor = {useImageSpacing};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKLaplacianSharpeningImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
