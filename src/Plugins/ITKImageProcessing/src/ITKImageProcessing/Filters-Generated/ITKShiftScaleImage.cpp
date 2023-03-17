#include "ITKShiftScaleImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkShiftScaleImageFilter.h>

using namespace complex;

namespace cxITKShiftScaleImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

struct ITKShiftScaleImageFunctor
{
  float64 shift = 0;
  float64 scale = 1.0;
  std::vector<uint32> kernelRadius = {1, 1, 1};

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::ShiftScaleImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetShift(shift);
    filter->SetScale(scale);
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    return filter;
  }
};
} // namespace cxITKShiftScaleImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKShiftScaleImage::name() const
{
  return FilterTraits<ITKShiftScaleImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKShiftScaleImage::className() const
{
  return FilterTraits<ITKShiftScaleImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKShiftScaleImage::uuid() const
{
  return FilterTraits<ITKShiftScaleImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKShiftScaleImage::humanName() const
{
  return "ITK Shift Scale Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKShiftScaleImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKShiftScaleImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKShiftScaleImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Shift_Key, "Shift", "", 0));
  params.insert(std::make_unique<Float64Parameter>(k_Scale_Key, "Scale", "", 1.0));
  params.insert(std::make_unique<VectorParameter<uint32>>(k_OutputPixelType_Key, "KernelRadius", "", itk::simple::sitkUnknown, std::vector<std::string>(3)));

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
IFilter::UniquePointer ITKShiftScaleImage::clone() const
{
  return std::make_unique<ITKShiftScaleImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKShiftScaleImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto shift = filterArgs.value<float64>(k_Shift_Key);
  auto scale = filterArgs.value<float64>(k_Scale_Key);
  auto outputPixelType = filterArgs.value<VectorParameter<uint32>::ValueType>(k_OutputPixelType_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKShiftScaleImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKShiftScaleImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto shift = filterArgs.value<float64>(k_Shift_Key);
  auto scale = filterArgs.value<float64>(k_Scale_Key);
  auto outputPixelType = filterArgs.value<VectorParameter<uint32>::ValueType>(k_OutputPixelType_Key);

  cxITKShiftScaleImage::ITKShiftScaleImageFunctor itkFunctor = {shift, scale, outputPixelType};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKShiftScaleImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
