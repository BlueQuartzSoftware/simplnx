#include "ITKStandardDeviationProjectionImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkStandardDeviationProjectionImageFilter.h>

using namespace nx::core;

namespace cxITKStandardDeviationProjectionImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = double;

struct ITKStandardDeviationProjectionImageFunctor
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
} // namespace cxITKStandardDeviationProjectionImage

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKStandardDeviationProjectionImage::name() const
{
  return FilterTraits<ITKStandardDeviationProjectionImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKStandardDeviationProjectionImage::className() const
{
  return FilterTraits<ITKStandardDeviationProjectionImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKStandardDeviationProjectionImage::uuid() const
{
  return FilterTraits<ITKStandardDeviationProjectionImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKStandardDeviationProjectionImage::humanName() const
{
  return "ITK Standard Deviation Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKStandardDeviationProjectionImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKStandardDeviationProjectionImage", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKStandardDeviationProjectionImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt32Parameter>(k_ProjectionDimension_Key, "Projection Dimension", "The dimension index to project. 0=Slowest moving dimension.", 0u));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKStandardDeviationProjectionImage::clone() const
{
  return std::make_unique<ITKStandardDeviationProjectionImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKStandardDeviationProjectionImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKStandardDeviationProjectionImage::ArrayOptionsType, cxITKStandardDeviationProjectionImage::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKStandardDeviationProjectionImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);

  const cxITKStandardDeviationProjectionImage::ITKStandardDeviationProjectionImageFunctor itkFunctor = {projectionDimension};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKStandardDeviationProjectionImage::ArrayOptionsType, cxITKStandardDeviationProjectionImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                        outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
