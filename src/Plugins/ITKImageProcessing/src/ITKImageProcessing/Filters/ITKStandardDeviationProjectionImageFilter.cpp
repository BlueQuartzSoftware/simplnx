#include "ITKStandardDeviationProjectionImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <ITKStandardDeviationProjectionImageFilter.h>

using namespace nx::core;

namespace cxITKStandardDeviationProjectionImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = double;

struct ITKStandardDeviationProjectionImageFilterFunctor
{
  uint32 projectionDimension = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::StandardDeviationProjectionImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetProjectionDimension(projectionDimension);
    return filter;
  }
};
} // namespace cxITKStandardDeviationProjectionImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKStandardDeviationProjectionImageFilter::name() const
{
  return FilterTraits<ITKStandardDeviationProjectionImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKStandardDeviationProjectionImageFilter::className() const
{
  return FilterTraits<ITKStandardDeviationProjectionImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKStandardDeviationProjectionImageFilter::uuid() const
{
  return FilterTraits<ITKStandardDeviationProjectionImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKStandardDeviationProjectionImageFilter::humanName() const
{
  return "ITK Standard Deviation Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKStandardDeviationProjectionImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKStandardDeviationProjectionImageFilter", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKStandardDeviationProjectionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt32Parameter>(k_ProjectionDimension_Key, "Projection Dimension", "The dimension index to project. 0=Slowest moving dimension.", 0u));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKStandardDeviationProjectionImageFilter::clone() const
{
  return std::make_unique<ITKStandardDeviationProjectionImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKStandardDeviationProjectionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKStandardDeviationProjectionImageFilter::ArrayOptionsType, cxITKStandardDeviationProjectionImageFilter::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKStandardDeviationProjectionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);

  const cxITKStandardDeviationProjectionImageFilter::ITKStandardDeviationProjectionImageFilterFunctor itkFunctor = {projectionDimension};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKStandardDeviationProjectionImageFilter::ArrayOptionsType, cxITKStandardDeviationProjectionImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                                    outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
