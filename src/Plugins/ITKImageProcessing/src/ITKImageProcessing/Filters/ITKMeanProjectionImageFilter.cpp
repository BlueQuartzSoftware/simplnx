#include "ITKMeanProjectionImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include <itkMeanProjectionImageFilter.h>

using namespace nx::core;

namespace cxITKMeanProjectionImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;
template <class PixelT>
using FilterOutputType = double;

struct ITKMeanProjectionImageFilterFunctor
{
  uint32 projectionDimension = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::MeanProjectionImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetProjectionDimension(projectionDimension);
    return filter;
  }
};
} // namespace cxITKMeanProjectionImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMeanProjectionImageFilter::name() const
{
  return FilterTraits<ITKMeanProjectionImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKMeanProjectionImageFilter::className() const
{
  return FilterTraits<ITKMeanProjectionImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMeanProjectionImageFilter::uuid() const
{
  return FilterTraits<ITKMeanProjectionImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMeanProjectionImageFilter::humanName() const
{
  return "ITK Mean Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMeanProjectionImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMeanProjectionImageFilter", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKMeanProjectionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt32Parameter>(k_ProjectionDimension_Key, "Projection Dimension", "The dimension index to project. 0=Slowest moving dimension.", 0u));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In-Place", "Performs the projection in-place for the given Image Geometry", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<StringParameter>(k_OutputImageGeomName_Key, "Created Image Geometry", "The name of the projected geometry", "Projected Image"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  params.linkParameters(k_RemoveOriginalGeometry_Key, k_OutputImageGeomName_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ITKMeanProjectionImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMeanProjectionImageFilter::clone() const
{
  return std::make_unique<ITKMeanProjectionImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMeanProjectionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  auto preformInPlace = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto outputGeomName = filterArgs.value<std::string>(k_OutputImageGeomName_Key);
  DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions;
  // The input geometry must be preserved, so we will just copy the needed array into newly created output geometry
  if(!preformInPlace)
  {
    DataPath outputGeomPath({outputGeomName});

    const auto& originalGeometry = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

    // Make copy of input geometry
    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(
        outputGeomPath, originalGeometry.getDimensions().toContainer<CreateImageGeometryAction::DimensionType>(), originalGeometry.getOrigin().toContainer<CreateImageGeometryAction::OriginType>(),
        originalGeometry.getSpacing().toContainer<CreateImageGeometryAction::SpacingType>(), originalGeometry.getCellDataPath().getTargetName()));

    outputArrayPath = outputGeomPath.createChildPath(originalGeometry.getCellDataPath().getTargetName()).createChildPath(outputArrayName);
  }

  Result<OutputActions> helperOutputActions =
      ITK::DataCheck<cxITKMeanProjectionImageFilter::ArrayOptionsType, cxITKMeanProjectionImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  // Consolidate actions
  resultOutputActions.value().actions.insert(resultOutputActions.value().actions.end(), helperOutputActions.value().actions.begin(), helperOutputActions.value().actions.end());

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMeanProjectionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto preformInPlace = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);

  if(!preformInPlace)
  {
    const auto& originalGeometry = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

    imageGeomPath = DataPath({filterArgs.value<std::string>(k_OutputImageGeomName_Key)});
    outputArrayPath = imageGeomPath.createChildPath(originalGeometry.getCellDataPath().getTargetName()).createChildPath(outputArrayName);
  }

  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);

  const cxITKMeanProjectionImageFilter::ITKMeanProjectionImageFilterFunctor itkFunctor = {projectionDimension};

  auto result = ITK::Execute<cxITKMeanProjectionImageFilter::ArrayOptionsType, cxITKMeanProjectionImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                                 itkFunctor, shouldCancel);
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
  dataStructure.getDataAs<AttributeMatrix>(outputArrayPath.getParent())->resizeTuples(iArrayTupleShape);

  return {};
}
} // namespace nx::core
