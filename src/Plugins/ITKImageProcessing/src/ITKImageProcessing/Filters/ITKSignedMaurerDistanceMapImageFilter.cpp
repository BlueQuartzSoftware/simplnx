#include "ITKSignedMaurerDistanceMapImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkSignedMaurerDistanceMapImageFilter.h>

using namespace nx::core;

namespace cxITKSignedMaurerDistanceMapImageFilter
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = float32;

struct ITKSignedMaurerDistanceMapImageFunctor
{
  bool insideIsPositive = false;
  bool squaredDistance = true;
  bool useImageSpacing = false;
  float64 backgroundValue = 0.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::SignedMaurerDistanceMapImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetInsideIsPositive(insideIsPositive);
    filter->SetSquaredDistance(squaredDistance);
    filter->SetUseImageSpacing(useImageSpacing);
    filter->SetBackgroundValue(backgroundValue);
    return filter;
  }
};
} // namespace cxITKSignedMaurerDistanceMapImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKSignedMaurerDistanceMapImageFilter::name() const
{
  return FilterTraits<ITKSignedMaurerDistanceMapImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKSignedMaurerDistanceMapImageFilter::className() const
{
  return FilterTraits<ITKSignedMaurerDistanceMapImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSignedMaurerDistanceMapImageFilter::uuid() const
{
  return FilterTraits<ITKSignedMaurerDistanceMapImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSignedMaurerDistanceMapImageFilter::humanName() const
{
  return "ITK Signed Maurer Distance Map Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSignedMaurerDistanceMapImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKSignedMaurerDistanceMapImage", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKSignedMaurerDistanceMapImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_InsideIsPositive_Key, "Inside Is Positive",
                                                "Set if the inside represents positive values in the signed distance map. By convention ON pixels are treated as inside pixels.", false));
  params.insert(std::make_unique<BoolParameter>(k_SquaredDistance_Key, "SquaredDistance", "Set if the distance should be squared.", true));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "Use Image Spacing", "Set if image spacing should be used in computing distances.", false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value", "Set the background value which defines the object. Usually this value is = 0.", 0.0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ITKSignedMaurerDistanceMapImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSignedMaurerDistanceMapImageFilter::clone() const
{
  return std::make_unique<ITKSignedMaurerDistanceMapImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSignedMaurerDistanceMapImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto insideIsPositive = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKSignedMaurerDistanceMapImageFilter::ArrayOptionsType, cxITKSignedMaurerDistanceMapImageFilter::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSignedMaurerDistanceMapImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto insideIsPositive = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  const cxITKSignedMaurerDistanceMapImageFilter::ITKSignedMaurerDistanceMapImageFunctor itkFunctor = {insideIsPositive, squaredDistance, useImageSpacing, backgroundValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKSignedMaurerDistanceMapImageFilter::ArrayOptionsType, cxITKSignedMaurerDistanceMapImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                            outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InsideIsPositiveKey = "InsideIsPositive";
constexpr StringLiteral k_SquaredDistanceKey = "SquaredDistance";
constexpr StringLiteral k_UseImageSpacingKey = "UseImageSpacing";
constexpr StringLiteral k_BackgroundValueKey = "BackgroundValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKSignedMaurerDistanceMapImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKSignedMaurerDistanceMapImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_InsideIsPositiveKey, k_InsideIsPositive_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SquaredDistanceKey, k_SquaredDistance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UseImageSpacingKey, k_UseImageSpacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BackgroundValueKey, k_BackgroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
