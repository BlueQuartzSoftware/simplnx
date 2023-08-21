#include "ITKInvertIntensityImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkInvertIntensityImageFilter.h>

using namespace complex;

namespace cxITKInvertIntensityImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

struct ITKInvertIntensityImageFunctor
{
  float64 maximum = 255;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::InvertIntensityImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetMaximum(maximum);
    return filter;
  }
};
} // namespace cxITKInvertIntensityImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKInvertIntensityImage::name() const
{
  return FilterTraits<ITKInvertIntensityImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKInvertIntensityImage::className() const
{
  return FilterTraits<ITKInvertIntensityImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKInvertIntensityImage::uuid() const
{
  return FilterTraits<ITKInvertIntensityImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKInvertIntensityImage::humanName() const
{
  return "ITK Invert Intensity Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKInvertIntensityImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKInvertIntensityImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKInvertIntensityImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Maximum_Key, "Maximum", "The maximum intensity value for the inversion.", 255));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKInvertIntensityImage::clone() const
{
  return std::make_unique<ITKInvertIntensityImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKInvertIntensityImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto maximum = filterArgs.value<float64>(k_Maximum_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKInvertIntensityImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKInvertIntensityImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto maximum = filterArgs.value<float64>(k_Maximum_Key);

  cxITKInvertIntensityImage::ITKInvertIntensityImageFunctor itkFunctor = {maximum};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKInvertIntensityImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
