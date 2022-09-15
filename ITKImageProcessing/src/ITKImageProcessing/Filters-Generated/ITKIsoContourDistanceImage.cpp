#include "ITKIsoContourDistanceImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkIsoContourDistanceImageFilter.h>

using namespace complex;

namespace cxITKIsoContourDistanceImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = float32;

struct ITKIsoContourDistanceImageFunctor
{
  float64 levelSetValue = 0.0;
  float64 farValue = 10;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::IsoContourDistanceImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetLevelSetValue(levelSetValue);
    filter->SetFarValue(farValue);
    return filter;
  }
};
} // namespace cxITKIsoContourDistanceImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKIsoContourDistanceImage::name() const
{
  return FilterTraits<ITKIsoContourDistanceImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKIsoContourDistanceImage::className() const
{
  return FilterTraits<ITKIsoContourDistanceImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKIsoContourDistanceImage::uuid() const
{
  return FilterTraits<ITKIsoContourDistanceImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKIsoContourDistanceImage::humanName() const
{
  return "ITK Iso Contour Distance Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKIsoContourDistanceImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKIsoContourDistanceImage", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKIsoContourDistanceImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_LevelSetValue_Key, "LevelSetValue", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_FarValue_Key, "FarValue", "", 10));

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
IFilter::UniquePointer ITKIsoContourDistanceImage::clone() const
{
  return std::make_unique<ITKIsoContourDistanceImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKIsoContourDistanceImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto levelSetValue = filterArgs.value<float64>(k_LevelSetValue_Key);
  auto farValue = filterArgs.value<float64>(k_FarValue_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKIsoContourDistanceImage::ArrayOptionsT, cxITKIsoContourDistanceImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKIsoContourDistanceImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto levelSetValue = filterArgs.value<float64>(k_LevelSetValue_Key);
  auto farValue = filterArgs.value<float64>(k_FarValue_Key);

  cxITKIsoContourDistanceImage::ITKIsoContourDistanceImageFunctor itkFunctor = {levelSetValue, farValue};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKIsoContourDistanceImage::ArrayOptionsT, cxITKIsoContourDistanceImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
