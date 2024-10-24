#include "ITKIntensityWindowingImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkIntensityWindowingImageFilter.h>

using namespace nx::core;

namespace cxITKIntensityWindowingImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKIntensityWindowingImageFunctor
{
  float64 windowMinimum = 0.0;
  float64 windowMaximum = 255.0;
  float64 outputMinimum = 0.0;
  float64 outputMaximum = 255.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::IntensityWindowingImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetWindowMinimum(windowMinimum);
    filter->SetWindowMaximum(windowMaximum);
    filter->SetOutputMinimum(outputMinimum);
    filter->SetOutputMaximum(outputMaximum);
    return filter;
  }
};
} // namespace cxITKIntensityWindowingImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKIntensityWindowingImageFilter::name() const
{
  return FilterTraits<ITKIntensityWindowingImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKIntensityWindowingImageFilter::className() const
{
  return FilterTraits<ITKIntensityWindowingImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKIntensityWindowingImageFilter::uuid() const
{
  return FilterTraits<ITKIntensityWindowingImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKIntensityWindowingImageFilter::humanName() const
{
  return "ITK Intensity Windowing Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKIntensityWindowingImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKIntensityWindowingImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKIntensityWindowingImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_WindowMinimum_Key, "Window Minimum", "Set/Get the values of the maximum and minimum intensities of the input intensity window.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_WindowMaximum_Key, "Window Maximum", "Set/Get the values of the maximum and minimum intensities of the input intensity window.", 255.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMinimum_Key, "Output Minimum", "Set/Get the values of the maximum and minimum intensities of the output image.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMaximum_Key, "Output Maximum", "Set/Get the values of the maximum and minimum intensities of the output image.", 255.0));

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
IFilter::VersionType ITKIntensityWindowingImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKIntensityWindowingImageFilter::clone() const
{
  return std::make_unique<ITKIntensityWindowingImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKIntensityWindowingImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto windowMinimum = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto windowMaximum = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKIntensityWindowingImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKIntensityWindowingImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto windowMinimum = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto windowMaximum = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);

  const cxITKIntensityWindowingImageFilter::ITKIntensityWindowingImageFunctor itkFunctor = {windowMinimum, windowMaximum, outputMinimum, outputMaximum};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKIntensityWindowingImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_WindowMinimumKey = "WindowMinimum";
constexpr StringLiteral k_WindowMaximumKey = "WindowMaximum";
constexpr StringLiteral k_OutputMinimumKey = "OutputMinimum";
constexpr StringLiteral k_OutputMaximumKey = "OutputMaximum";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKIntensityWindowingImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKIntensityWindowingImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_WindowMinimumKey, k_WindowMinimum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_WindowMaximumKey, k_WindowMaximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutputMinimumKey, k_OutputMinimum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutputMaximumKey, k_OutputMaximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
