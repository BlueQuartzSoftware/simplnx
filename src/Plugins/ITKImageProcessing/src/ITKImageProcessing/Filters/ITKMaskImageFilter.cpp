#include "ITKMaskImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkMaskImageFilter.h>

using namespace nx::core;

namespace cxITKMaskImageFilter
{
using ArrayOptionsType = ITK::ScalarVectorPixelIdTypeList;

struct ITKMaskImageFunctor
{
  float64 outsideValue = 0;
  float64 maskingValue = 0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::MaskImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetOutsideValue(outsideValue);
    filter->SetMaskingValue(maskingValue);
    return filter;
  }
};
} // namespace cxITKMaskImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMaskImageFilter::name() const
{
  return FilterTraits<ITKMaskImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKMaskImageFilter::className() const
{
  return FilterTraits<ITKMaskImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMaskImageFilter::uuid() const
{
  return FilterTraits<ITKMaskImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMaskImageFilter::humanName() const
{
  return "ITK Mask Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMaskImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMaskImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKMaskImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue", "Method to explicitly set the outside value of the mask. Defaults to 0", 0));
  params.insert(std::make_unique<Float64Parameter>(k_MaskingValue_Key, "MaskingValue", "Method to explicitly set the masking value of the mask. Defaults to 0", 0));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetNonLabelPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Array Name",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMaskImageFilter::clone() const
{
  return std::make_unique<ITKMaskImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMaskImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto maskingValue = filterArgs.value<float64>(k_MaskingValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMaskImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMaskImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto maskingValue = filterArgs.value<float64>(k_MaskingValue_Key);

  const cxITKMaskImageFilter::ITKMaskImageFunctor itkFunctor = {outsideValue, maskingValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKMaskImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
