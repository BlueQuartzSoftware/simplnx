#include "ITKMedianImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkMedianImageFilter.h>

using namespace complex;

namespace cxITKMedianImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKMedianImageFunctor
{
  std::vector<uint64> radius = std::vector<uint64>(3, 1);

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::MedianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    using RadiusType = typename FilterT::RadiusType;
    auto convertedRadius = ITK::CastVec3ToITK<RadiusType, typename RadiusType::SizeValueType>(radius, RadiusType::Dimension);
    filter->SetRadius(convertedRadius);
    return filter;
  }
};
} // namespace cxITKMedianImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMedianImage::name() const
{
  return FilterTraits<ITKMedianImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMedianImage::className() const
{
  return FilterTraits<ITKMedianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMedianImage::uuid() const
{
  return FilterTraits<ITKMedianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMedianImage::humanName() const
{
  return "ITK Median Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMedianImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMedianImage", "ITKSmoothing", "Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKMedianImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Parameters"});
  params.insert(std::make_unique<VectorUInt64Parameter>(k_Radius_Key, "Radius", "", std::vector<uint64>(3, 1), std::vector<std::string>(3)));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMedianImage::clone() const
{
  return std::make_unique<ITKMedianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMedianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto radius = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Radius_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMedianImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMedianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                     const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto radius = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Radius_Key);

  cxITKMedianImage::ITKMedianImageFunctor itkFunctor = {radius};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  itk::ProgressObserver::Pointer progressObs = itk::ProgressObserver::New(messageHandler);
  progressObs->setMessagePrefix("Processing Median Image");
  return ITK::Execute<cxITKMedianImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel, progressObs);
}
} // namespace complex
