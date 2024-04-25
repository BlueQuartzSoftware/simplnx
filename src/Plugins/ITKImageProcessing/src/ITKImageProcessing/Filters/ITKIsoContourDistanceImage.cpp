#include "ITKIsoContourDistanceImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkIsoContourDistanceImageFilter.h>

using namespace nx::core;

namespace cxITKIsoContourDistanceImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = float32;

struct ITKIsoContourDistanceImageFunctor
{
  float64 levelSetValue = 0.0;
  float64 farValue = 10;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::IsoContourDistanceImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetLevelSetValue(levelSetValue);
    filter->SetFarValue(farValue);
    return filter;
  }
};
} // namespace cxITKIsoContourDistanceImage

namespace nx::core
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
  return {className(), "ITKImageProcessing", "ITKIsoContourDistanceImage", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKIsoContourDistanceImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_LevelSetValue_Key, "Level Set Value", "Set/Get the value of the level set to be located. The default value is 0.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_FarValue_Key, "Far Value", "Set/Get the value of the level set to be located. The default value is 0.", 10));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

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
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto levelSetValue = filterArgs.value<float64>(k_LevelSetValue_Key);
  auto farValue = filterArgs.value<float64>(k_FarValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKIsoContourDistanceImage::ArrayOptionsType, cxITKIsoContourDistanceImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKIsoContourDistanceImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto levelSetValue = filterArgs.value<float64>(k_LevelSetValue_Key);
  auto farValue = filterArgs.value<float64>(k_FarValue_Key);

  const cxITKIsoContourDistanceImage::ITKIsoContourDistanceImageFunctor itkFunctor = {levelSetValue, farValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKIsoContourDistanceImage::ArrayOptionsType, cxITKIsoContourDistanceImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                                      shouldCancel);
}
} // namespace nx::core
