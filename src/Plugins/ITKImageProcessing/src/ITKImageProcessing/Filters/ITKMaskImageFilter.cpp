#include "ITKMaskImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include <itkCastImageFilter.h>
#include <itkMaskImageFilter.h>

#include <numeric>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <stdexcept>

using namespace nx::core;

namespace cxITKMaskImageFilter
{
template <uint32 Dimension>
using MaskImageT = itk::Image<uint32, Dimension>;

template <uint32 Dimension, class T>
typename MaskImageT<Dimension>::Pointer CastDataStoreToUInt32Image(DataStore<T>& dataStore, const ImageGeom& imageGeom)
{
  static_assert(std::is_same_v<T, uint8> || std::is_same_v<T, uint16> || std::is_same_v<T, uint32>);

  using InputImageT = itk::Image<T, Dimension>;
  using OutputImageT = MaskImageT<Dimension>;
  using CastFilterT = itk::CastImageFilter<InputImageT, OutputImageT>;

  auto inputImage = ITK::WrapDataStoreInImage<T, Dimension>(dataStore, imageGeom);

  auto castFilter = CastFilterT::New();
  castFilter->SetInput(inputImage);
  castFilter->Update();

  typename OutputImageT::Pointer outputImage = castFilter->GetOutput();
  return outputImage;
}

template <uint32 Dimension>
typename MaskImageT<Dimension>::Pointer CastIDataStoreToUInt32Image(IDataStore& dataStore, const ImageGeom& imageGeom)
{
  DataType dataType = dataStore.getDataType();
  switch(dataType)
  {
  case DataType::uint8: {
    auto& typedDataStore = dynamic_cast<DataStore<uint8>&>(dataStore);
    return CastDataStoreToUInt32Image<Dimension>(typedDataStore, imageGeom);
  }
  case DataType::uint16: {
    auto& typedDataStore = dynamic_cast<DataStore<uint16>&>(dataStore);
    return CastDataStoreToUInt32Image<Dimension>(typedDataStore, imageGeom);
  }
  case DataType::uint32: {
    auto& typedDataStore = dynamic_cast<DataStore<uint32>&>(dataStore);
    return CastDataStoreToUInt32Image<Dimension>(typedDataStore, imageGeom);
  }
  default: {
    throw std::runtime_error("Unsupported mask image type");
  }
  }
}

template <class InputPixelT, class OutputPixelT>
OutputPixelT MakeOutsideValue(float64 value)
{
  std::vector<usize> cDims = ITK::GetComponentDimensions<InputPixelT>();
  usize numComponents = std::accumulate(cDims.cbegin(), cDims.cend(), static_cast<usize>(1), std::multiplies<>());
  OutputPixelT outsideValue{};
  itk::NumericTraits<OutputPixelT>::SetLength(outsideValue, numComponents);
  outsideValue = static_cast<OutputPixelT>(value);
  return outsideValue;
}

using ArrayOptionsT = ITK::ScalarVectorPixelIdTypeList;

struct ITKMaskImageFilterFunctor
{
  float64 outsideValue = 0;
  const ImageGeom& imageGeom;
  IDataStore& maskDataStore;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using MaskImageType = MaskImageT<Dimension>;
    using FilterT = itk::MaskImageFilter<InputImageT, MaskImageType, OutputImageT>;
    using InputPixelT = typename InputImageT::ValueType;
    using OutsideValueT = typename OutputImageT::PixelType;

    typename MaskImageType::Pointer maskImage = CastIDataStoreToUInt32Image<Dimension>(maskDataStore, imageGeom);

    OutsideValueT trueOutsideValue = MakeOutsideValue<InputPixelT, OutsideValueT>(outsideValue);

    auto filter = FilterT::New();
    filter->SetOutsideValue(trueOutsideValue);
    filter->SetMaskImage(maskImage);

    return filter;
  }
};
} // namespace cxITKMaskImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMaskImageFilter::name() const
{
  return FilterTraits<ITKMaskImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKMaskImageFilter::className() const
{
  return FilterTraits<ITKMaskImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMaskImageFilter::uuid() const
{
  return FilterTraits<ITKMaskImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMaskImageFilter::humanName() const
{
  return "ITK Mask Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMaskImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMaskImageFilter", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKMaskImageFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "Outside Value", "Method to explicitly set the outside value of the mask.", 0));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{}, nx::core::GetAllDataTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskImageDataPath_Key, "MaskImage", "The path to the image data to be used as the mask (should be the same size as the input image)",
                                                          DataPath{}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ITKMaskImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMaskImageFilter::clone() const
{
  return std::make_unique<ITKMaskImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMaskImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskImageDataPath_Key);

  // Once ArraySelectionParameter allows for restricting the type, this check can be removed
  Result<> result = ITK::CheckImageType({DataType::uint8, DataType::uint16, DataType::uint32}, dataStructure, maskArrayPath);
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(result), {})};
  }

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMaskImageFilter::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMaskImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskImageDataPath_Key);

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  IDataArray& maskArray = dataStructure.getDataRefAs<IDataArray>(maskArrayPath);
  IDataStore& maskStore = maskArray.getIDataStoreRef();

  cxITKMaskImageFilter::ITKMaskImageFilterFunctor itkFunctor = {outsideValue, imageGeom, maskStore};

  return ITK::Execute<cxITKMaskImageFilter::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutsideValueKey = "OutsideValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_MaskCellArrayPathKey = "MaskCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKMaskImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKMaskImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutsideValueKey, k_OutsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskCellArrayPathKey, k_MaskImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
