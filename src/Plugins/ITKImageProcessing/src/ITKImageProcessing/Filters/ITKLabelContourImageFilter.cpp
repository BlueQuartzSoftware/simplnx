#include "ITKLabelContourImageFilter.hpp"

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

#include <itkLabelContourImageFilter.h>

using namespace nx::core;

namespace cxITKLabelContourImageFilter
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;

struct ITKLabelContourImageFunctor
{
  bool fullyConnected = false;
  float64 backgroundValue = 0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::LabelContourImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetFullyConnected(fullyConnected);
    filter->SetBackgroundValue(backgroundValue);
    return filter;
  }
};
} // namespace cxITKLabelContourImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKLabelContourImageFilter::name() const
{
  return FilterTraits<ITKLabelContourImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKLabelContourImageFilter::className() const
{
  return FilterTraits<ITKLabelContourImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKLabelContourImageFilter::uuid() const
{
  return FilterTraits<ITKLabelContourImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKLabelContourImageFilter::humanName() const
{
  return "ITK Label Contour Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKLabelContourImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKLabelContourImage", "ITKImageLabel", "ImageLabel"};
}

//------------------------------------------------------------------------------
Parameters ITKLabelContourImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "**NOTE** For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "Set/Get the background value used to identify the objects and mark the pixels not on the border of the objects.", 0));

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
IFilter::UniquePointer ITKLabelContourImageFilter::clone() const
{
  return std::make_unique<ITKLabelContourImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKLabelContourImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKLabelContourImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKLabelContourImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  const cxITKLabelContourImageFilter::ITKLabelContourImageFunctor itkFunctor = {fullyConnected, backgroundValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKLabelContourImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKLabelContourImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKLabelContourImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
