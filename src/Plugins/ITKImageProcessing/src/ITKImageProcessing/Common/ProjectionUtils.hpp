#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "ITKArrayHelper.hpp"

using namespace nx::core;

namespace ProjectionUtilities
{
// uint8, int16, uint16, float32
using ITKProjectionSupportedOutputTypes = ArrayTypeOptions<false, false, true, true, true, false, false, false, false, true, false>;

template <class T>
struct FixedOutputTypeHelper
{
  template <class PixelT>
  using FilterOutputType = T;
};

template <class ArrayOptionsType>
struct RunITKProjectionDataCheckFunctor
{
  template <class FixedOutputType>
  auto operator()(const DataStructure& dataStructure, const DataPath& selectedInputArray, const DataPath& imageGeomPath, const DataPath& outputArrayPath) const
  {
    return ITK::DataCheck<ArrayOptionsType, FixedOutputTypeHelper<FixedOutputType>::template FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  }
};

template <class ArrayOptionsType, class ITKFunctorType>
struct RunITKProjectionExecuteFunctor
{
  template <class FixedOutputType>
  auto operator()(DataStructure& dataStructure, const DataPath& selectedInputArray, const DataPath& imageGeomPath, const DataPath& outputArrayPath, ITKFunctorType&& itkFunctor,
                  const std::atomic_bool& shouldCancel) const
  {
    return ITK::Execute<ArrayOptionsType, FixedOutputTypeHelper<FixedOutputType>::template FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                             shouldCancel);
  }
};

template <class ArrayTypeOptions, class FuncT, class FallbackFuncT, class... ArgsT>
auto RunTemplateFunctor(FuncT&& func, FallbackFuncT&& fallback, DataType dataType, ArgsT&&... args)
{
  if constexpr(ArrayTypeOptions::UsingBoolean)
  {
    if(dataType == DataType::boolean)
    {
      return func.template operator()<bool>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt8)
  {
    if(dataType == DataType::int8)
    {
      return func.template operator()<int8>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt16)
  {
    if(dataType == DataType::int16)
    {
      return func.template operator()<int16>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt32)
  {
    if(dataType == DataType::int32)
    {
      return func.template operator()<int32>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt64)
  {
    if(dataType == DataType::int64)
    {
      return func.template operator()<int64>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt8)
  {
    if(dataType == DataType::uint8)
    {
      return func.template operator()<uint8>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt16)
  {
    if(dataType == DataType::uint16)
    {
      return func.template operator()<uint16>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt32)
  {
    if(dataType == DataType::uint32)
    {
      return func.template operator()<uint32>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt64)
  {
    if(dataType == DataType::uint64)
    {
      return func.template operator()<uint64>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingFloat32)
  {
    if(dataType == DataType::float32)
    {
      return func.template operator()<float32>(std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingFloat64)
  {
    if(dataType == DataType::float64)
    {
      return func.template operator()<float64>(std::forward<ArgsT>(args)...);
    }
  }
  return fallback(dataType);
}

template <class ArrayOptionsType>
IFilter::PreflightResult RunITKProjectionDataCheck(const DataStructure& dataStructure, const DataPath& selectedInputArray, const DataPath& imageGeomPath, const std::string& outputGeomName,
                                                   bool performInPlace, const std::string& outputArrayName)
{
  DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);
  Result<OutputActions> resultOutputActions;
  // The input geometry must be preserved, so we will just copy the needed array into newly created output geometry
  if(!performInPlace)
  {
    DataPath outputGeomPath({outputGeomName});

    const auto& originalGeometry = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

    // Make copy of input geometry
    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(
        outputGeomPath, originalGeometry.getDimensions().toContainer<CreateImageGeometryAction::DimensionType>(), originalGeometry.getOrigin().toContainer<CreateImageGeometryAction::OriginType>(),
        originalGeometry.getSpacing().toContainer<CreateImageGeometryAction::SpacingType>(), originalGeometry.getCellDataPath().getTargetName()));

    outputArrayPath = outputGeomPath.createChildPath(originalGeometry.getCellDataPath().getTargetName()).createChildPath(outputArrayName);
  }

  auto fallbackFunc = [](DataType dataType) {
    return MakeErrorResult<OutputActions>(-76590, fmt::format("Input {} type is not currently supported. Please reach out to devs if you have a use case.", DataTypeToString(dataType)));
  };

  DataType type = dataStructure.getDataRefAs<IDataArray>(selectedInputArray).getDataType();
  Result<OutputActions> helperOutputActions = RunTemplateFunctor<ITKProjectionSupportedOutputTypes>(RunITKProjectionDataCheckFunctor<ArrayOptionsType>{}, fallbackFunc, type, dataStructure,
                                                                                                    selectedInputArray, imageGeomPath, outputArrayPath);

  if(helperOutputActions.invalid())
  {
    return {std::move(helperOutputActions)};
  }

  // Consolidate actions
  resultOutputActions.value().actions.insert(resultOutputActions.value().actions.end(), helperOutputActions.value().actions.begin(), helperOutputActions.value().actions.end());

  return {std::move(resultOutputActions)};
}

template <class ArrayOptionsType, class ITKFunctorType>
Result<> RunITKProjectionExecute(DataStructure& dataStructure, const DataPath& selectedInputArray, const DataPath& imageGeomPath, const std::atomic_bool& shouldCancel,
                                 const std::string& outputArrayName, bool performInPlace, ITKFunctorType&& itkFunctor, const std::string& outputImageGeomName)
{
  DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  DataPath finalImageGeomPath = imageGeomPath;

  if(!performInPlace)
  {
    const auto& originalGeometry = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

    finalImageGeomPath = DataPath({outputImageGeomName});
    outputArrayPath = finalImageGeomPath.createChildPath(originalGeometry.getCellDataPath().getTargetName()).createChildPath(outputArrayName);
  }

  DataType type = dataStructure.getDataRefAs<IDataArray>(selectedInputArray).getDataType();

  auto fallbackFunc = [](DataType dataType) {
    return MakeErrorResult(-76591, fmt::format("Input {} type is not currently supported. Please reach out to devs if you have a use case.", DataTypeToString(dataType)));
  };

  Result<> result = RunTemplateFunctor<ITKProjectionSupportedOutputTypes>(RunITKProjectionExecuteFunctor<ArrayOptionsType, ITKFunctorType>{}, fallbackFunc, type, dataStructure, selectedInputArray,
                                                                          finalImageGeomPath, outputArrayPath, itkFunctor, shouldCancel);

  if(result.invalid())
  {
    return result;
  }

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(finalImageGeomPath);
  auto iArrayTupleShape = dataStructure.getDataAs<IArray>(outputArrayPath)->getTupleShape();

  // Update the Image Geometry with the new dimensions
  imageGeom.setDimensions({iArrayTupleShape[2], iArrayTupleShape[1], iArrayTupleShape[0]});

  // Update the AttributeMatrix with the new tuple shape. THIS WILL ALSO CHANGE ANY OTHER DATA ARRAY THAT IS ALSO
  // STORED IN THAT ATTRIBUTE MATRIX
  dataStructure.getDataAs<AttributeMatrix>(outputArrayPath.getParent())->resizeTuples(iArrayTupleShape);

  return {};
}
} // namespace ProjectionUtilities