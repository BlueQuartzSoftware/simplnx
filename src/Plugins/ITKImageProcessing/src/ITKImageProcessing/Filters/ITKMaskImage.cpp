#include "ITKMaskImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkCastImageFilter.h>
#include <itkMaskImageFilter.h>

#include <numeric>
#include <stdexcept>

using namespace complex;

namespace cxITKMaskImage
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

struct ITKMaskImageFunctor
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
} // namespace cxITKMaskImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMaskImage::name() const
{
  return FilterTraits<ITKMaskImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMaskImage::className() const
{
  return FilterTraits<ITKMaskImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMaskImage::uuid() const
{
  return FilterTraits<ITKMaskImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMaskImage::humanName() const
{
  return "ITK Mask Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMaskImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMaskImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKMaskImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue", "Method to explicitly set the outside value of the mask.", 0));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::GetAllDataTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskImageDataPath_Key, "MaskImage", "The path to the image data to be used as the mask (should be the same size as the input image)",
                                                          DataPath{}, complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMaskImage::clone() const
{
  return std::make_unique<ITKMaskImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMaskImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskImageDataPath_Key);

  // Once ArraySelectionParameter allows for restricting the type, this check can be removed
  Result<> result = ITK::CheckImageType({DataType::uint8, DataType::uint16, DataType::uint32}, dataStructure, maskArrayPath);
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(result), {})};
  }

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMaskImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMaskImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskImageDataPath_Key);

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  IDataArray& maskArray = dataStructure.getDataRefAs<IDataArray>(maskArrayPath);
  IDataStore& maskStore = maskArray.getIDataStoreRef();

  const IDataArray* inputArray = dataStructure.getDataAs<IDataArray>(selectedInputArray);
  if(inputArray->getDataFormat() != "")
  {
    return MakeErrorResult(-9999, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", selectedInputArray.toString()));
  }

  cxITKMaskImage::ITKMaskImageFunctor itkFunctor = {outsideValue, imageGeom, maskStore};

  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKMaskImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
