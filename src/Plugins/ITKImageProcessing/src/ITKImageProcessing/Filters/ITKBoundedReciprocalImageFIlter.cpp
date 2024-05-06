#include "ITKBoundedReciprocalImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include <itkBoundedReciprocalImageFilter.h>

using namespace nx::core;

namespace cxITKBoundedReciprocalImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;
template <class PixelT>
using FilterOutputType = double;

struct ITKBoundedReciprocalImageFilterFunctor
{
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::BoundedReciprocalImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    return filter;
  }
};
} // namespace cxITKBoundedReciprocalImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKBoundedReciprocalImageFilter::name() const
{
  return FilterTraits<ITKBoundedReciprocalImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKBoundedReciprocalImageFilter::className() const
{
  return FilterTraits<ITKBoundedReciprocalImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBoundedReciprocalImageFilter::uuid() const
{
  return FilterTraits<ITKBoundedReciprocalImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBoundedReciprocalImageFilter::humanName() const
{
  return "ITK Bounded Reciprocal Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBoundedReciprocalImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKBoundedReciprocalImageFilter", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKBoundedReciprocalImageFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBoundedReciprocalImageFilter::clone() const
{
  return std::make_unique<ITKBoundedReciprocalImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBoundedReciprocalImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKBoundedReciprocalImageFilter::ArrayOptionsType, cxITKBoundedReciprocalImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBoundedReciprocalImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const cxITKBoundedReciprocalImageFilter::ITKBoundedReciprocalImageFilterFunctor itkFunctor = {};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKBoundedReciprocalImageFilter::ArrayOptionsType, cxITKBoundedReciprocalImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                                itkFunctor, shouldCancel);
}
} // namespace nx::core
