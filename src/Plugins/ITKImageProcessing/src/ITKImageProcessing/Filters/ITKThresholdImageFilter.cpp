#include "ITKThresholdImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkThresholdImageFilter.h>

using namespace nx::core;

namespace cxITKThresholdImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKThresholdImageFunctor
{
  float64 lower = 0.0;
  float64 upper = 1.0;
  float64 outsideValue = 0.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::ThresholdImageFilter<InputImageT>;
    auto filter = FilterType::New();
    filter->SetLower(lower);
    filter->SetUpper(upper);
    filter->SetOutsideValue(outsideValue);
    return filter;
  }
};
} // namespace cxITKThresholdImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKThresholdImageFilter::name() const
{
  return FilterTraits<ITKThresholdImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKThresholdImageFilter::className() const
{
  return FilterTraits<ITKThresholdImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKThresholdImageFilter::uuid() const
{
  return FilterTraits<ITKThresholdImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKThresholdImageFilter::humanName() const
{
  return "ITK Threshold Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKThresholdImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKThresholdImage", "ITKThresholding", "Thresholding"};
}

//------------------------------------------------------------------------------
Parameters ITKThresholdImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Lower_Key, "Lower", "Set/Get methods to set the lower threshold.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_Upper_Key, "Upper", "Set/Get methods to set the upper threshold.", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue",
                                                   "The pixel type must support comparison operators. Set the 'outside' pixel value. The default value NumericTraits<PixelType>::ZeroValue() .", 0.0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKThresholdImageFilter::clone() const
{
  return std::make_unique<ITKThresholdImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKThresholdImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto lower = filterArgs.value<float64>(k_Lower_Key);
  auto upper = filterArgs.value<float64>(k_Upper_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKThresholdImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKThresholdImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto lower = filterArgs.value<float64>(k_Lower_Key);
  auto upper = filterArgs.value<float64>(k_Upper_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  const cxITKThresholdImageFilter::ITKThresholdImageFunctor itkFunctor = {lower, upper, outsideValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKThresholdImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_LowerKey = "Lower";
constexpr StringLiteral k_UpperKey = "Upper";
constexpr StringLiteral k_OutsideValueKey = "OutsideValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKThresholdImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKThresholdImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_LowerKey, k_Lower_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_UpperKey, k_Upper_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutsideValueKey, k_OutsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
