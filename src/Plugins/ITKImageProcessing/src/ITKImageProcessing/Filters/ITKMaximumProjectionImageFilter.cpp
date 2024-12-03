#include "ITKMaximumProjectionImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkMaximumProjectionImageFilter.h>

using namespace nx::core;

namespace cxITKMaximumProjectionImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;
template <class PixelT>
using FilterOutputType = double;

struct ITKMaximumProjectionImageFilterFunctor
{
  uint32 projectionDimension = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::MaximumProjectionImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetProjectionDimension(projectionDimension);
    return filter;
  }
};
} // namespace cxITKMaximumProjectionImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMaximumProjectionImageFilter::name() const
{
  return FilterTraits<ITKMaximumProjectionImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKMaximumProjectionImageFilter::className() const
{
  return FilterTraits<ITKMaximumProjectionImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMaximumProjectionImageFilter::uuid() const
{
  return FilterTraits<ITKMaximumProjectionImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMaximumProjectionImageFilter::humanName() const
{
  return "ITK Mean Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMaximumProjectionImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMaximumProjectionImageFilter", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKMaximumProjectionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt32Parameter>(k_ProjectionDimension_Key, "Projection Dimension", "The dimension index to project. 0=Slowest moving dimension.", 0u));

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
IFilter::VersionType ITKMaximumProjectionImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMaximumProjectionImageFilter::clone() const
{
  return std::make_unique<ITKMaximumProjectionImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMaximumProjectionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKMaximumProjectionImageFilter::ArrayOptionsType, cxITKMaximumProjectionImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMaximumProjectionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);

  const cxITKMaximumProjectionImageFilter::ITKMaximumProjectionImageFilterFunctor itkFunctor = {projectionDimension};

  auto result = ITK::Execute<cxITKMaximumProjectionImageFilter::ArrayOptionsType, cxITKMaximumProjectionImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                       outputArrayPath, itkFunctor, shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  auto iArrayTupleShape = dataStructure.getDataAs<IArray>(outputArrayPath)->getTupleShape();

  // Update the Image Geometry with the new dimensions
  imageGeom.setDimensions({iArrayTupleShape[2], iArrayTupleShape[1], iArrayTupleShape[0]});

  // Update the AttributeMatrix with the new tuple shape. THIS WILL ALSO CHANGE ANY OTHER DATA ARRAY THAT IS ALSO
  // STORED IN THAT ATTRIBUTE MATRIX
  auto amPathVector = outputArrayPath.getPathVector();
  amPathVector.pop_back();
  DataPath amPath(amPathVector);
  auto& attributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(amPath);
  attributeMatrix.resizeTuples(iArrayTupleShape);

  return {};
}
} // namespace nx::core
