#include "ITKMinimumProjectionImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include <itkMinimumProjectionImageFilter.h>

using namespace nx::core;

namespace cxITKMinimumProjectionImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

template <class PixelT>
using FilterOutputTypeUI8 = uint8;

template <class PixelT>
using FilterOutputTypeUI16 = uint16;

template <class PixelT>
using FilterOutputTypeF32 = float32;

struct ITKMinimumProjectionImageFilterFunctor
{
  uint32 projectionDimension = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::MinimumProjectionImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetProjectionDimension(projectionDimension);
    return filter;
  }
};
} // namespace cxITKMinimumProjectionImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMinimumProjectionImageFilter::name() const
{
  return FilterTraits<ITKMinimumProjectionImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKMinimumProjectionImageFilter::className() const
{
  return FilterTraits<ITKMinimumProjectionImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMinimumProjectionImageFilter::uuid() const
{
  return FilterTraits<ITKMinimumProjectionImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMinimumProjectionImageFilter::humanName() const
{
  return "ITK Minimum Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMinimumProjectionImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMinimumProjectionImageFilter", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKMinimumProjectionImageFilter::parameters() const
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
IFilter::VersionType ITKMinimumProjectionImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMinimumProjectionImageFilter::clone() const
{
  return std::make_unique<ITKMinimumProjectionImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMinimumProjectionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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

  Result<OutputActions> helperOutputActions = {};
  DataType type = dataStructure.getDataAs<IDataArray>(selectedInputArray)->getDataType();
  switch(type)
  {
  case DataType::uint8: {
    helperOutputActions =
        ITK::DataCheck<cxITKMinimumProjectionImageFilter::ArrayOptionsType, cxITKMinimumProjectionImageFilter::FilterOutputTypeUI8>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
    break;
  }
  case DataType::uint16: {
    helperOutputActions =
        ITK::DataCheck<cxITKMinimumProjectionImageFilter::ArrayOptionsType, cxITKMinimumProjectionImageFilter::FilterOutputTypeUI16>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
    break;
  }
  case DataType::float32: {
    helperOutputActions =
        ITK::DataCheck<cxITKMinimumProjectionImageFilter::ArrayOptionsType, cxITKMinimumProjectionImageFilter::FilterOutputTypeF32>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
    break;
  }
  case DataType::int8:
    [[fallthrough]];
  case DataType::int16:
    [[fallthrough]];
  case DataType::int32:
    [[fallthrough]];
  case DataType::uint32:
    [[fallthrough]];
  case DataType::int64:
    [[fallthrough]];
  case DataType::uint64: {
    [[fallthrough]];
  }
  case DataType::float64: {
    [[fallthrough]];
  }
  case DataType::boolean: {
    return MakePreflightErrorResult(-76610, fmt::format("Input {} type is not currently supported. Please reach out to devs if you have a use case.", DataTypeToString(type)));
  }
  }
  if(helperOutputActions.invalid())
  {
    return {std::move(helperOutputActions)};
  }

  // Consolidate actions
  resultOutputActions.value().actions.insert(resultOutputActions.value().actions.end(), helperOutputActions.value().actions.begin(), helperOutputActions.value().actions.end());

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMinimumProjectionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
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

  const cxITKMinimumProjectionImageFilter::ITKMinimumProjectionImageFilterFunctor itkFunctor = {projectionDimension};

  Result<> result = {};
  DataType type = dataStructure.getDataAs<IDataArray>(selectedInputArray)->getDataType();
  switch(type)
  {
  case DataType::uint8: {
    result = ITK::Execute<cxITKMinimumProjectionImageFilter::ArrayOptionsType, cxITKMinimumProjectionImageFilter::FilterOutputTypeUI8>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                       outputArrayPath, itkFunctor, shouldCancel);
    break;
  }
  case DataType::uint16: {
    result = ITK::Execute<cxITKMinimumProjectionImageFilter::ArrayOptionsType, cxITKMinimumProjectionImageFilter::FilterOutputTypeUI16>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                        outputArrayPath, itkFunctor, shouldCancel);
    break;
  }
  case DataType::float32: {
    result = ITK::Execute<cxITKMinimumProjectionImageFilter::ArrayOptionsType, cxITKMinimumProjectionImageFilter::FilterOutputTypeF32>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                       outputArrayPath, itkFunctor, shouldCancel);
    break;
  }
  case DataType::int8:
    [[fallthrough]];
  case DataType::int16:
    [[fallthrough]];
  case DataType::int32:
    [[fallthrough]];
  case DataType::uint32:
    [[fallthrough]];
  case DataType::int64:
    [[fallthrough]];
  case DataType::uint64: {
    [[fallthrough]];
  }
  case DataType::float64: {
    [[fallthrough]];
  }
  case DataType::boolean: {
    return MakeErrorResult(-76601, fmt::format("Input {} type is not currently supported. Please reach out to devs if you have a use case.", DataTypeToString(type)));
  }
  }
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
