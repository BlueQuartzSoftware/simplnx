#include "BenchmarkFilterRegistry.hpp"

#include "BenchmarkTiling.hpp"

#include "ItkGoldenTestUtils.hpp"

#include "SimplnxCore/Filters/ConvertColorToGrayScaleFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

// New ITK-free filter headers -- included for their FilterTraits<...>::uuid and k_*_Key parameter constants.
// The full plugin FilterList (see ImageProcessing/CMakeLists.txt) is wired below, so every header is included.
#include "ImageProcessing/Filters/AbsImageFilter.hpp"
#include "ImageProcessing/Filters/AcosImageFilter.hpp"
#include "ImageProcessing/Filters/AdaptiveHistogramEqualizationImageFilter.hpp"
#include "ImageProcessing/Filters/ApproximateSignedDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/AsinImageFilter.hpp"
#include "ImageProcessing/Filters/AtanImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryContourImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryDilateImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryErodeImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryMorphologicalClosingImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryMorphologicalOpeningImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryOpeningByReconstructionImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryThinningImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryThresholdImageFilter.hpp"
#include "ImageProcessing/Filters/BlackTopHatImageFilter.hpp"
#include "ImageProcessing/Filters/BoundedReciprocalImageFilter.hpp"
#include "ImageProcessing/Filters/ClosingByReconstructionImageFilter.hpp"
#include "ImageProcessing/Filters/ConnectedComponentImageFilter.hpp"
#include "ImageProcessing/Filters/CosImageFilter.hpp"
#include "ImageProcessing/Filters/CurvatureAnisotropicDiffusionImageFilter.hpp"
#include "ImageProcessing/Filters/CurvatureFlowImageFilter.hpp"
#include "ImageProcessing/Filters/DanielssonDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/DilateObjectMorphologyImageFilter.hpp"
#include "ImageProcessing/Filters/DiscreteGaussianImageFilter.hpp"
#include "ImageProcessing/Filters/DoubleThresholdImageFilter.hpp"
#include "ImageProcessing/Filters/ErodeObjectMorphologyImageFilter.hpp"
#include "ImageProcessing/Filters/ExpImageFilter.hpp"
#include "ImageProcessing/Filters/ExpNegativeImageFilter.hpp"
#include "ImageProcessing/Filters/GradientAnisotropicDiffusionImageFilter.hpp"
#include "ImageProcessing/Filters/GradientMagnitudeImageFilter.hpp"
#include "ImageProcessing/Filters/GradientMagnitudeRecursiveGaussianImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleDilateImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleErodeImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleFillholeImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleGrindPeakImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleMorphologicalClosingImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleMorphologicalOpeningImageFilter.hpp"
#include "ImageProcessing/Filters/HConvexImageFilter.hpp"
#include "ImageProcessing/Filters/HMaximaImageFilter.hpp"
#include "ImageProcessing/Filters/HMinimaImageFilter.hpp"
#include "ImageProcessing/Filters/IntensityWindowingImageFilter.hpp"
#include "ImageProcessing/Filters/InvertIntensityImageFilter.hpp"
#include "ImageProcessing/Filters/IsoContourDistanceImageFilter.hpp"
#include "ImageProcessing/Filters/LabelContourImageFilter.hpp"
#include "ImageProcessing/Filters/LaplacianRecursiveGaussianImageFilter.hpp"
#include "ImageProcessing/Filters/Log10ImageFilter.hpp"
#include "ImageProcessing/Filters/LogImageFilter.hpp"
#include "ImageProcessing/Filters/MaskImageFilter.hpp"
#include "ImageProcessing/Filters/MaximumProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/MeanProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/MedianImageFilter.hpp"
#include "ImageProcessing/Filters/MedianProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/MinMaxCurvatureFlowImageFilter.hpp"
#include "ImageProcessing/Filters/MinimumProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/MorphologicalGradientImageFilter.hpp"
#include "ImageProcessing/Filters/MorphologicalWatershedFromMarkersImageFilter.hpp"
#include "ImageProcessing/Filters/MorphologicalWatershedImageFilter.hpp"
#include "ImageProcessing/Filters/NormalizeImageFilter.hpp"
#include "ImageProcessing/Filters/NormalizeToConstantImageFilter.hpp"
#include "ImageProcessing/Filters/NotImageFilter.hpp"
#include "ImageProcessing/Filters/OpeningByReconstructionImageFilter.hpp"
#include "ImageProcessing/Filters/OtsuMultipleThresholdsImageFilter.hpp"
#include "ImageProcessing/Filters/RegionalMaximaImageFilter.hpp"
#include "ImageProcessing/Filters/RegionalMinimaImageFilter.hpp"
#include "ImageProcessing/Filters/RelabelComponentImageFilter.hpp"
#include "ImageProcessing/Filters/RescaleIntensityImageFilter.hpp"
#include "ImageProcessing/Filters/SigmoidImageFilter.hpp"
#include "ImageProcessing/Filters/SignedDanielssonDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/SignedMaurerDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/SinImageFilter.hpp"
#include "ImageProcessing/Filters/SmoothingRecursiveGaussianImageFilter.hpp"
#include "ImageProcessing/Filters/SqrtImageFilter.hpp"
#include "ImageProcessing/Filters/SquareImageFilter.hpp"
#include "ImageProcessing/Filters/StandardDeviationProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/SumProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/TanImageFilter.hpp"
#include "ImageProcessing/Filters/ThresholdImageFilter.hpp"
#include "ImageProcessing/Filters/ThresholdMaximumConnectedComponentsImageFilter.hpp"
#include "ImageProcessing/Filters/ValuedRegionalMaximaImageFilter.hpp"
#include "ImageProcessing/Filters/ValuedRegionalMinimaImageFilter.hpp"
#include "ImageProcessing/Filters/WhiteTopHatImageFilter.hpp"
#include "ImageProcessing/Filters/ZeroCrossingImageFilter.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <any>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
using namespace nx::core;
using ip_bench::SourceArrayValidator;

constexpr int32 k_InvalidSourceArray = -9130;
constexpr int32 k_SourceReadFailed = -9131;
constexpr int32 k_MissingSourceClasses = -9132;
constexpr int32 k_InputPreparationFailed = -9133;

std::string FirstErrorMessage(const std::vector<Error>& errors)
{
  return errors.empty() ? "(no error message was provided)" : errors.front().message;
}

Result<> ValidateUint8Shape(const IDataArray& sourceArray, const std::filesystem::path& sourcePath, usize expectedComponents, std::string_view purpose)
{
  if(sourceArray.getDataType() != DataType::uint8 || sourceArray.getNumberOfComponents() != expectedComponents)
  {
    return MakeErrorResult(k_InvalidSourceArray,
                           fmt::format("Benchmark input '{}' for {} must be a uint8 array with {} component(s), but its type is '{}' with {} component(s) and {} tuple(s).", sourcePath.string(),
                                       purpose, expectedComponents, DataTypeToString(sourceArray.getDataType()), sourceArray.getNumberOfComponents(), sourceArray.getNumberOfTuples()));
  }
  return {};
}

SourceArrayValidator RequireUint8Values(std::vector<uint8> requiredValues, std::string purpose)
{
  return [requiredValues = std::move(requiredValues), purpose = std::move(purpose)](const IDataArray& sourceArray, const std::filesystem::path& sourcePath) -> Result<> {
    if(Result<> shapeResult = ValidateUint8Shape(sourceArray, sourcePath, 1, purpose); shapeResult.invalid())
    {
      return shapeResult;
    }

    const auto& store = sourceArray.getIDataStoreRefAs<AbstractDataStore<uint8>>();
    std::vector<uint8> sourceValues(store.getSize());
    if(Result<> readResult = store.copyIntoBuffer(0, nonstd::span<uint8>(sourceValues.data(), sourceValues.size())); readResult.invalid())
    {
      if(readResult.errors().empty())
      {
        return MakeErrorResult(k_SourceReadFailed, fmt::format("Could not read the {} source array from '{}' while validating its {} element(s); the data store did not provide an error message.",
                                                               purpose, sourcePath.string(), sourceValues.size()));
      }
      for(Error& error : readResult.errors())
      {
        error.message = fmt::format("Could not read the {} source array from '{}' while validating its {} element(s): {}", purpose, sourcePath.string(), sourceValues.size(), error.message);
      }
      return readResult;
    }

    std::vector<uint8> missingValues;
    for(const uint8 requiredValue : requiredValues)
    {
      if(std::find(sourceValues.cbegin(), sourceValues.cend(), requiredValue) == sourceValues.cend())
      {
        missingValues.push_back(requiredValue);
      }
    }
    if(!missingValues.empty())
    {
      std::string missing;
      for(usize index = 0; index < missingValues.size(); ++index)
      {
        missing += fmt::format("{}{}", index == 0 ? "" : ", ", missingValues[index]);
      }
      return MakeErrorResult(k_MissingSourceClasses,
                             fmt::format("Benchmark input '{}' for {} is missing required value(s) [{}]. The source has type '{}', {} component(s), and {} tuple(s).", sourcePath.string(), purpose,
                                         missing, DataTypeToString(sourceArray.getDataType()), sourceArray.getNumberOfComponents(), sourceArray.getNumberOfTuples()));
    }
    return {};
  };
}

SourceArrayValidator RequireUint8ZeroAndNonzero(std::string purpose)
{
  return [purpose = std::move(purpose)](const IDataArray& sourceArray, const std::filesystem::path& sourcePath) -> Result<> {
    if(Result<> shapeResult = ValidateUint8Shape(sourceArray, sourcePath, 1, purpose); shapeResult.invalid())
    {
      return shapeResult;
    }

    const auto& store = sourceArray.getIDataStoreRefAs<AbstractDataStore<uint8>>();
    std::vector<uint8> sourceValues(store.getSize());
    if(Result<> readResult = store.copyIntoBuffer(0, nonstd::span<uint8>(sourceValues.data(), sourceValues.size())); readResult.invalid())
    {
      if(readResult.errors().empty())
      {
        return MakeErrorResult(k_SourceReadFailed, fmt::format("Could not read the {} source array from '{}' while validating its {} element(s); the data store did not provide an error message.",
                                                               purpose, sourcePath.string(), sourceValues.size()));
      }
      for(Error& error : readResult.errors())
      {
        error.message = fmt::format("Could not read the {} source array from '{}' while validating its {} element(s): {}", purpose, sourcePath.string(), sourceValues.size(), error.message);
      }
      return readResult;
    }

    const bool containsZero = std::find(sourceValues.cbegin(), sourceValues.cend(), uint8{0}) != sourceValues.cend();
    const bool containsNonzero = std::any_of(sourceValues.cbegin(), sourceValues.cend(), [](uint8 value) { return value != 0; });
    if(!containsZero || !containsNonzero)
    {
      return MakeErrorResult(k_MissingSourceClasses,
                             fmt::format("Benchmark input '{}' for {} must contain both zero and nonzero values, but containsZero={} and containsNonzero={}. The source has {} tuple(s).",
                                         sourcePath.string(), purpose, containsZero, containsNonzero, sourceArray.getNumberOfTuples()));
    }
    return {};
  };
}

Result<DataPath> BuildPrimaryInput(DataStructure& dataStructure, const ip_bench::BenchmarkInputContext& context, const SourceArrayValidator& sourceValidator = {})
{
  DataPath inputPath;
  Result<> buildResult = ip_bench::BuildTiledInput(dataStructure, context.realInputPath, context.geometryPath, context.cellAttributeMatrixName, context.inputArrayName, context.targetVoxels,
                                                   context.minZSlices, context.storeMode, inputPath, context.force2D, sourceValidator);
  return ConvertResultTo<DataPath>(std::move(buildResult), std::move(inputPath));
}

Result<DataPath> PrepareMaskInput(DataStructure& dataStructure, const ip_bench::BenchmarkInputContext& context)
{
  Result<DataPath> primaryResult = BuildPrimaryInput(dataStructure, context);
  if(primaryResult.invalid())
  {
    return primaryResult;
  }

  const DataPath primaryPath = primaryResult.value();
  DataPath maskPath;
  Result<> maskResult = ip_bench::AddTiledArray(dataStructure, ip_golden::InputPath("STAPLE2.png"), context.geometryPath, context.cellAttributeMatrixName, "Mask", context.storeMode, maskPath,
                                                RequireUint8ZeroAndNonzero("Mask benchmark mask"));
  if(maskResult.invalid())
  {
    return ConvertResultTo<DataPath>(std::move(maskResult), {});
  }
  if(primaryPath == maskPath)
  {
    return MakeErrorResult<DataPath>(
        k_InputPreparationFailed,
        fmt::format("Mask benchmark primary input path '{}' and mask path '{}' must be distinct. Configure the mask as a separately tiled source array.", primaryPath.toString(), maskPath.toString()));
  }
  return {primaryPath};
}

Result<DataPath> PrepareAdaptiveHistogramInput(DataStructure& dataStructure, const ip_bench::BenchmarkInputContext& context)
{
  Result<DataPath> rgbResult = BuildPrimaryInput(dataStructure, context, [](const IDataArray& sourceArray, const std::filesystem::path& sourcePath) {
    return ValidateUint8Shape(sourceArray, sourcePath, 3, "AdaptiveHistogramEqualization RGB input");
  });
  if(rgbResult.invalid())
  {
    return rgbResult;
  }
  const DataPath rgbPath = rgbResult.value();
  const auto& rgbArray = dataStructure.getDataRefAs<IDataArray>(rgbPath);
  const usize rgbTupleCount = rgbArray.getNumberOfTuples();
  const std::string rgbFormat = rgbArray.getDataFormat();

  ConvertColorToGrayScaleFilter colorToGrayScaleFilter;
  Arguments args;
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ConversionAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.2125f, 0.7154f, 0.0721f}));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ColorChannel_Key, std::make_any<Int32Parameter::ValueType>(0));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_InputDataArrayPath_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{rgbPath}));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_OutputArrayPrefix_Key, std::make_any<StringParameter::ValueType>("GrayScale_"));

  const IFilter::PreflightResult preflightResult = colorToGrayScaleFilter.preflight(dataStructure, args);
  if(preflightResult.outputActions.invalid())
  {
    return MakeErrorResult<DataPath>(k_InputPreparationFailed, fmt::format("AdaptiveHistogramEqualization benchmark grayscale preflight failed for RGB input '{}': {}", rgbPath.toString(),
                                                                           FirstErrorMessage(preflightResult.outputActions.errors())));
  }
  const IFilter::ExecuteResult executeResult = colorToGrayScaleFilter.execute(dataStructure, args);
  if(executeResult.result.invalid())
  {
    return MakeErrorResult<DataPath>(k_InputPreparationFailed, fmt::format("AdaptiveHistogramEqualization benchmark grayscale conversion failed for RGB input '{}': {}", rgbPath.toString(),
                                                                           FirstErrorMessage(executeResult.result.errors())));
  }

  const DataPath grayPath = rgbPath.replaceName(fmt::format("GrayScale_{}", rgbPath.getTargetName()));
  const auto* grayArray = dataStructure.getDataAs<IDataArray>(grayPath);
  if(grayArray == nullptr)
  {
    return MakeErrorResult<DataPath>(k_InputPreparationFailed,
                                     fmt::format("AdaptiveHistogramEqualization benchmark grayscale conversion did not create the expected uint8 scalar array at '{}'.", grayPath.toString()));
  }
  if(grayArray->getDataType() != DataType::uint8 || grayArray->getNumberOfComponents() != 1 || grayArray->getNumberOfTuples() != rgbTupleCount || grayArray->getDataFormat() != rgbFormat)
  {
    return MakeErrorResult<DataPath>(k_InputPreparationFailed, fmt::format("AdaptiveHistogramEqualization benchmark grayscale array '{}' must be uint8 scalar data with {} tuples in store format "
                                                                           "'{}', but has type '{}', {} component(s), {} tuples, and format '{}'.",
                                                                           grayPath.toString(), rgbTupleCount, rgbFormat, DataTypeToString(grayArray->getDataType()),
                                                                           grayArray->getNumberOfComponents(), grayArray->getNumberOfTuples(), grayArray->getDataFormat()));
  }
  if(!dataStructure.removeData(rgbPath))
  {
    return MakeErrorResult<DataPath>(
        k_InputPreparationFailed,
        fmt::format("AdaptiveHistogramEqualization benchmark could not remove the temporary tiled RGB array '{}' after creating grayscale array '{}'.", rgbPath.toString(), grayPath.toString()));
  }
  return {grayPath};
}

Result<DataPath> PrepareWatershedMarkersInput(DataStructure& dataStructure, const ip_bench::BenchmarkInputContext& context)
{
  Result<DataPath> primaryResult = BuildPrimaryInput(dataStructure, context);
  if(primaryResult.invalid())
  {
    return primaryResult;
  }

  const DataPath primaryPath = primaryResult.value();
  DataPath markerPath;
  Result<> markerResult = ip_bench::AddTiledArray(dataStructure, ip_golden::InputPath("cthead1-marker.png"), context.geometryPath, context.cellAttributeMatrixName, "Marker", context.storeMode,
                                                  markerPath, RequireUint8Values({uint8{0}, uint8{1}, uint8{2}}, "MorphologicalWatershedFromMarkers marker labels"));
  if(markerResult.invalid())
  {
    return ConvertResultTo<DataPath>(std::move(markerResult), {});
  }
  if(primaryPath == markerPath)
  {
    return MakeErrorResult<DataPath>(
        k_InputPreparationFailed,
        fmt::format("MorphologicalWatershedFromMarkers benchmark primary input path '{}' and marker path '{}' must be distinct. Configure the marker as a separately tiled source array.",
                    primaryPath.toString(), markerPath.toString()));
  }
  return {primaryPath};
}

// The three geometry/input/output keys are shared verbatim across all ImageProcessing filters
// ("input_image_geometry_path", "input_image_data_path", "output_array_name"), so one helper drives them all.
template <class FilterT>
void SetStandardKeys(Arguments& args, const DataPath& geom, const DataPath& inputArray, const std::string& outName)
{
  args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputArray));
  args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>(outName));
}

// Sets the structuring-element keys shared by every morphology/reconstruction filter (k_KernelType_Key +
// k_KernelRadius_Key). KernelType choices are {Annulus=0, Ball=1, Box=2, Cross=3}; benchmarks use a moderate,
// non-pathological Ball r{2,2,2} so the timing is representative (not a huge decomposable-Box line filter).
template <class FilterT>
void SetKernel(Arguments& args, uint64 kernelTypeChoice, const std::vector<uint32>& radius)
{
  args.insertOrAssign(FilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelTypeChoice)));
  args.insertOrAssign(FilterT::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
}

// Sets the six keys shared by every axis-projection filter (mirrors the already-wired MaximumProjection): project
// along X (dim 0) into a NEW geometry ("Projected Image") so the tiled input survives repeats (an in-place
// projection would collapse it). RemoveOriginalGeometry=false keeps the original geometry for the next repeat.
template <class FilterT>
void SetProjectionKeys(Arguments& args, const DataPath& geom, const DataPath& inputArray, const std::string& outName)
{
  args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputArray));
  args.insertOrAssign(FilterT::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{0}));
  args.insertOrAssign(FilterT::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(FilterT::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
  args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>(outName));
}

// Convenience: dereference a known-valid UUID string literal.
Uuid Legacy(const char* uuidStr)
{
  return Uuid::FromString(uuidStr).value();
}
} // namespace

namespace ip_bench
{
const std::vector<BenchmarkFilterSpec>& GetBenchmarkFilterRegistry()
{
  static const std::vector<BenchmarkFilterSpec> registry = [] {
    std::vector<BenchmarkFilterSpec> specs;

    // --- Fully wired (arg presets + legacy UUID + input verified against each filter's [ItkGolden]/parity test) ---

    // Abs: SameAsInput, no extra params. Legacy ITKAbsImage.
    specs.push_back({"Abs", FilterTraits<AbsImageFilter>::uuid, Legacy("e9dd12bc-f7fa-4ba2-98b0-fec3326bf620"), "RA-Slice-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<AbsImageFilter>(args, geom, in, out); }});

    // Median: radius {1,1,1} (the ITK golden "defaults" case). Legacy ITKMedianImage.
    specs.push_back({"Median", FilterTraits<MedianImageFilter>::uuid, Legacy("a60ca165-59ac-486b-b4b4-0f0c24d80af8"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MedianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(MedianImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
                     }});

    // GrayscaleMorphologicalOpening: moderate Ball kernel r{2,2,2}, Safe Border = true (the filter default). This is a
    // representative, NON-pathological kernel: the golden test's Box r{20,5,1} kernel makes the legacy ITK filter
    // pathologically slow (a huge decomposable-Box line filter), which distorts the timing comparison. Legacy
    // ITKGrayscaleMorphologicalOpening. KernelType choices are {Annulus=0, Ball=1, Box=2, Cross=3}.
    specs.push_back({"GrayscaleMorphologicalOpening", FilterTraits<GrayscaleMorphologicalOpeningImageFilter>::uuid, Legacy("54433e92-fb7d-40f4-95bf-eb3db76d5caa"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleMorphologicalOpeningImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(ChoicesParameter::ValueType{1})); // Ball
                       args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{2, 2, 2}));
                       args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_SafeBorder_Key, std::make_any<bool>(true)); // filter default
                     }});

    // ConnectedComponent: FullyConnected = false (the ITK golden "default" case). Legacy ITKConnectedComponent.
    specs.push_back({"ConnectedComponent", FilterTraits<ConnectedComponentImageFilter>::uuid, Legacy("905354c1-d55b-4436-b9f7-f4a6e80e5c0f"), "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ConnectedComponentImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ConnectedComponentImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});

    // SignedMaurerDistanceMap: ITK-default params (the golden helper sets no extra params; defaults apply). Legacy
    // ITKSignedMaurerDistanceMap. Integer input.
    specs.push_back({"SignedMaurerDistanceMap", FilterTraits<SignedMaurerDistanceMapImageFilter>::uuid, Legacy("e81f72d3-e806-4afe-ab4c-795c6a3f526f"), "2th_cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<SignedMaurerDistanceMapImageFilter>(args, geom, in, out); }});

    // GradientMagnitude: ITK-default params (UseImageSpacing defaults true). Legacy ITKGradientMagnitude.
    specs.push_back({"GradientMagnitude", FilterTraits<GradientMagnitudeImageFilter>::uuid, Legacy("719df7b2-8db2-43eb-a40c-a015982eec08"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<GradientMagnitudeImageFilter>(args, geom, in, out); }});

    // MaximumProjection: project along X (dim 0) into a NEW geometry so the tiled input survives repeats (in-place
    // would collapse it). Legacy ITKMaximumProjection.
    specs.push_back({"MaximumProjection",
                     FilterTraits<MaximumProjectionImageFilter>::uuid,
                     Legacy("6dfe9167-d77d-41c7-aebf-569d6190645d"),
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
                       args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(in));
                       args.insertOrAssign(MaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{0}));
                       args.insertOrAssign(MaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
                       args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
                       args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(out));
                     },
                     {},
                     true});

    // --- Fully wired (legacy UUID + real input + representative arg preset verified against each filter's
    //     [ItkGolden]/parity test). Legacy UUIDs are the ITKImageProcessing FilterTraits UUIDs (SIMPLNX_DEF_FILTER_TRAITS)
    //     the parity tests resolve at runtime; nullopt only where the legacy filter is commented out of the
    //     ITKImageProcessing build (grep its name in ITKImageProcessing/CMakeLists.txt). ---

    // BinaryMorphologicalClosing: needs a STRICTLY binary input. WhiteDots.png is {0,255} (the WithBorder ITK golden's
    // input), so Foreground = 255 -> internal background 0 satisfies the strictly-{fg,internal-bg} contract; a non-binary
    // input (e.g. 2th_cthead1.png / STAPLE1.png) trips the k_NonBinaryInput safeguard at execute. Moderate Ball r{2,2,2}
    // (the golden's r{5,5,5} is larger than needed), Safe Border = false (the golden's WithBorder=false case). Legacy
    // ITKBinaryMorphologicalClosing. KernelType choices are {Annulus=0, Ball=1, Box=2, Cross=3}.
    specs.push_back({"BinaryMorphologicalClosing", FilterTraits<BinaryMorphologicalClosingImageFilter>::uuid, Legacy("abb27e0c-b049-4f60-8355-178d86bb1de4"), "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryMorphologicalClosingImageFilter>(args, geom, in, out);
                       args.insertOrAssign(BinaryMorphologicalClosingImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(ChoicesParameter::ValueType{1})); // Ball
                       args.insertOrAssign(BinaryMorphologicalClosingImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{2, 2, 2}));
                       args.insertOrAssign(BinaryMorphologicalClosingImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryMorphologicalClosingImageFilter::k_SafeBorder_Key, std::make_any<bool>(false));
                     }});

    // GrayscaleFillhole: RA-Short.nrrd (int16), the GrayscaleFillhole1 ITK golden's input. FullyConnected = false
    // (the ITK default). No kernel. Legacy ITKGrayscaleFillhole.
    specs.push_back({"GrayscaleFillhole", FilterTraits<GrayscaleFillholeImageFilter>::uuid, Legacy("1f1dd9e4-d361-432b-a22b-5535664ee545"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleFillholeImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GrayscaleFillholeImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});

    // CurvatureAnisotropicDiffusion: RA-Float.nrrd (float32 -- this filter rejects integer input). TimeStep 0.01 (the
    // ITK golden "defaults" case value, safely below the CFL stability bound for this image so no stability warning
    // fires); ConductanceParameter/ConductanceScalingUpdateInterval/NumberOfIterations left at their defaults
    // (3.0 / 1 / 5), identical between the new and legacy filter. Legacy ITKCurvatureAnisotropicDiffusion.
    specs.push_back({"CurvatureAnisotropicDiffusion", FilterTraits<CurvatureAnisotropicDiffusionImageFilter>::uuid, Legacy("ada68f29-b1f2-44a2-86dc-f0cd28f54633"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<CurvatureAnisotropicDiffusionImageFilter>(args, geom, in, out);
                       args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(0.01));
                     }});

    // SmoothingRecursiveGaussian: RA-Float.nrrd (float32 -- this filter rejects UNSIGNED input). Sigma {2,2,2} (a
    // moderate representative blur; the recursive Deriche pass is O(N) regardless of sigma), NormalizeAcrossScale =
    // false (a no-op for order-0 smoothing anyway). Legacy ITKSmoothingRecursiveGaussian is DISABLED (commented out in
    // ITKImageProcessing/CMakeLists.txt), so there is no live-ITK baseline -> nullopt.
    specs.push_back({"SmoothingRecursiveGaussian", FilterTraits<SmoothingRecursiveGaussianImageFilter>::uuid, std::nullopt, "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<SmoothingRecursiveGaussianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{2.0, 2.0, 2.0}));
                       args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
                     }});

    // DilateObjectMorphology: RA-Slice-Short.nrrd (int16), the "short" ITK golden's input; ObjectValue 1 (the ITK
    // default, exactly representable in int16). Moderate Ball r{2,2,2}. Legacy ITKDilateObjectMorphology. (The
    // filter's live-legacy parity test uses a computed-expected oracle because the legacy multi-threaded ITK object-
    // morphology filter is nondeterministic, but the legacy filter IS registered, so it still serves as a timing
    // baseline here.) KernelType choices are {Annulus=0, Ball=1, Box=2, Cross=3}.
    specs.push_back({"DilateObjectMorphology", FilterTraits<DilateObjectMorphologyImageFilter>::uuid, Legacy("e3f7c642-4c16-47f6-ac5c-cd276d61bfa6"), "RA-Slice-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<DilateObjectMorphologyImageFilter>(args, geom, in, out);
                       args.insertOrAssign(DilateObjectMorphologyImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(ChoicesParameter::ValueType{1})); // Ball
                       args.insertOrAssign(DilateObjectMorphologyImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{2, 2, 2}));
                       args.insertOrAssign(DilateObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(1.0));
                     }});

    // MorphologicalWatershed: cthead1-grad-mag.nrrd, the ITK golden's input (a gradient-magnitude image -- the natural
    // watershed input). Level 0.0, MarkWatershedLine = true, FullyConnected = false (the golden "defaults" case,
    // identical between the new and legacy filter). Output is a fixed uint32 label image. The legacy ITKMorphological-
    // Watershed IS registered (only ITKMorphologicalWatershedFromMarkers is disabled), so it serves as the live-ITK
    // baseline. Legacy ITKMorphologicalWatershed.
    specs.push_back({"MorphologicalWatershed", FilterTraits<MorphologicalWatershedImageFilter>::uuid, Legacy("f70337e5-4435-41f7-aecc-d79b4b1faccd"), "cthead1-grad-mag.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MorphologicalWatershedImageFilter>(args, geom, in, out);
                       args.insertOrAssign(MorphologicalWatershedImageFilter::k_Level_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(MorphologicalWatershedImageFilter::k_MarkWatershedLine_Key, std::make_any<bool>(true));
                       args.insertOrAssign(MorphologicalWatershedImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});

    // =========================================================================
    // Full-plugin expansion: every remaining registered ImageProcessing filter (the 13 above are the original
    // representative subset). Legacy UUIDs are copied from ImageProcessingLegacyUUIDMapping.hpp (the authoritative
    // ITK->new replacement table); std::nullopt only where the legacy ITK filter is commented out of
    // ITKImageProcessing/CMakeLists.txt (BoundedReciprocal, NormalizeToConstant, StandardDeviationProjection,
    // SumProjection, MorphologicalWatershedFromMarkers, SmoothingRecursiveGaussian). Inputs mirror each filter's
    // [ItkGolden]/parity test; args are representative, NON-pathological values (moderate Ball r{2,2,2} kernels,
    // ITK parameter defaults) -- not correctness-golden pathological kernels.
    // =========================================================================

    // --- Unary pointwise math (SameAsInput; no extra parameters). Domain-restricted ops (Acos/Asin/Log/Log10/Sqrt)
    //     use Ramp-Zero-One-Float so the real input stays in range; Cos/Sin/Tan/Exp/ExpNegative/Square accept any
    //     scalar. Not is integer-only (STAPLE1.png is uint8). ---
    specs.push_back({"Acos", FilterTraits<AcosImageFilter>::uuid, Legacy("e7411c44-95ab-4623-8bf4-59b63d2d08c5"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<AcosImageFilter>(args, geom, in, out); }});
    specs.push_back({"Asin", FilterTraits<AsinImageFilter>::uuid, Legacy("1b463492-041f-4680-abb1-0b94a3019063"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<AsinImageFilter>(args, geom, in, out); }});
    specs.push_back({"Atan", FilterTraits<AtanImageFilter>::uuid, Legacy("39933f50-088c-46ac-a421-d238f1b178fd"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<AtanImageFilter>(args, geom, in, out); }});
    specs.push_back({"Cos", FilterTraits<CosImageFilter>::uuid, Legacy("6fe37f77-ceae-4839-9cf6-3ca7a70e14d0"), "RA-Slice-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<CosImageFilter>(args, geom, in, out); }});
    specs.push_back({"Sin", FilterTraits<SinImageFilter>::uuid, Legacy("06c76c7a-c384-44be-bd01-6fd58070cd65"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<SinImageFilter>(args, geom, in, out); }});
    specs.push_back({"Tan", FilterTraits<TanImageFilter>::uuid, Legacy("7cf3c08e-1af1-4540-aa08-4488a74923fc"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<TanImageFilter>(args, geom, in, out); }});
    specs.push_back({"Exp", FilterTraits<ExpImageFilter>::uuid, Legacy("264977e7-cc0d-4d2b-ba9c-a30765b498b2"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<ExpImageFilter>(args, geom, in, out); }});
    specs.push_back({"ExpNegative", FilterTraits<ExpNegativeImageFilter>::uuid, Legacy("2c84cc7c-01ab-4550-9ba5-b9fa58b74599"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<ExpNegativeImageFilter>(args, geom, in, out); }});
    specs.push_back({"Log", FilterTraits<LogImageFilter>::uuid, Legacy("4b6655ad-4e6c-4e68-a771-55ca0ae40915"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<LogImageFilter>(args, geom, in, out); }});
    specs.push_back({"Log10", FilterTraits<Log10ImageFilter>::uuid, Legacy("900ca377-e79d-4b54-b298-33d518238099"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<Log10ImageFilter>(args, geom, in, out); }});
    specs.push_back({"Sqrt", FilterTraits<SqrtImageFilter>::uuid, Legacy("05c7c812-4e33-4e9a-bf27-d4c17f5dff68"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<SqrtImageFilter>(args, geom, in, out); }});
    specs.push_back({"Square", FilterTraits<SquareImageFilter>::uuid, Legacy("385ca853-626c-43bb-ae86-db8d8b72693b"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<SquareImageFilter>(args, geom, in, out); }});
    specs.push_back({"Not", FilterTraits<NotImageFilter>::uuid, Legacy("6d67ad50-7e89-4678-85db-0b3247a2cfb9"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<NotImageFilter>(args, geom, in, out); }});

    // --- Intensity transforms. Values are ITK parameter defaults (representative, non-pathological). ---
    specs.push_back({"InvertIntensity", FilterTraits<InvertIntensityImageFilter>::uuid, Legacy("9958d587-5698-4ea5-b8ea-fb71428b5d02"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<InvertIntensityImageFilter>(args, geom, in, out);
                       args.insertOrAssign(InvertIntensityImageFilter::k_Maximum_Key, std::make_any<float64>(255.0));
                     }});
    // BoundedReciprocal: legacy ITKBoundedReciprocal is DISABLED (commented out of ITKImageProcessing) -> nullopt.
    specs.push_back({"BoundedReciprocal", FilterTraits<BoundedReciprocalImageFilter>::uuid, std::nullopt, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<BoundedReciprocalImageFilter>(args, geom, in, out); }});
    specs.push_back({"Sigmoid", FilterTraits<SigmoidImageFilter>::uuid, Legacy("cb9ec2b6-80d9-42e6-807b-d908bea6daea"), "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<SigmoidImageFilter>(args, geom, in, out);
                       args.insertOrAssign(SigmoidImageFilter::k_Alpha_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(SigmoidImageFilter::k_Beta_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(SigmoidImageFilter::k_OutputMinimum_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(SigmoidImageFilter::k_OutputMaximum_Key, std::make_any<float64>(255.0));
                     }});
    specs.push_back({"RescaleIntensity", FilterTraits<RescaleIntensityImageFilter>::uuid, Legacy("f08ea34d-9ad8-456c-a81b-9b3790b29379"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<RescaleIntensityImageFilter>(args, geom, in, out);
                       args.insertOrAssign(RescaleIntensityImageFilter::k_OutputMinimum_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(RescaleIntensityImageFilter::k_OutputMaximum_Key, std::make_any<float64>(255.0));
                     }});
    specs.push_back({"Normalize", FilterTraits<NormalizeImageFilter>::uuid, Legacy("9d8ce30e-c75e-4ca8-b6be-0b11baa7e6ce"), "Ramp-Up-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<NormalizeImageFilter>(args, geom, in, out); }});
    // NormalizeToConstant: legacy ITKNormalizeToConstant is DISABLED -> nullopt.
    specs.push_back({"NormalizeToConstant", FilterTraits<NormalizeToConstantImageFilter>::uuid, std::nullopt, "Ramp-Up-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<NormalizeToConstantImageFilter>(args, geom, in, out);
                       args.insertOrAssign(NormalizeToConstantImageFilter::k_Constant_Key, std::make_any<float64>(1.0));
                     }});
    specs.push_back({"IntensityWindowing", FilterTraits<IntensityWindowingImageFilter>::uuid, Legacy("ee317bf6-79aa-4dcc-b59e-b0246dc3fcfa"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<IntensityWindowingImageFilter>(args, geom, in, out);
                       args.insertOrAssign(IntensityWindowingImageFilter::k_WindowMinimum_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(IntensityWindowingImageFilter::k_WindowMaximum_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(IntensityWindowingImageFilter::k_OutputMinimum_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(IntensityWindowingImageFilter::k_OutputMaximum_Key, std::make_any<float64>(255.0));
                     }});

    // --- Thresholding. Inside/Outside are uint8 (fixed uint8 output); thresholds are float64. ---
    specs.push_back({"BinaryThreshold", FilterTraits<BinaryThresholdImageFilter>::uuid, Legacy("ba2494b0-c4f0-43ff-9d08-900395900e0c"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryThresholdImageFilter>(args, geom, in, out);
                       args.insertOrAssign(BinaryThresholdImageFilter::k_LowerThreshold_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(BinaryThresholdImageFilter::k_UpperThreshold_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryThresholdImageFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
                       args.insertOrAssign(BinaryThresholdImageFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
                     }});
    specs.push_back({"Threshold", FilterTraits<ThresholdImageFilter>::uuid, Legacy("ddf222f3-4af2-4583-967d-3eb9b86e77b4"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ThresholdImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ThresholdImageFilter::k_Lower_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(ThresholdImageFilter::k_Upper_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(ThresholdImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
                     }});
    specs.push_back({"DoubleThreshold", FilterTraits<DoubleThresholdImageFilter>::uuid, Legacy("e268a65f-33f1-493f-a6c7-4635e57df3c4"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<DoubleThresholdImageFilter>(args, geom, in, out);
                       args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold1_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold2_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold3_Key, std::make_any<float64>(254.0));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold4_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"OtsuMultipleThresholds", FilterTraits<OtsuMultipleThresholdsImageFilter>::uuid, Legacy("30f37bcd-701f-4e64-aa9d-1181469d3fb5"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<OtsuMultipleThresholdsImageFilter>(args, geom, in, out);
                       args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(uint8{1}));
                       args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(uint8{0}));
                       args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(uint32{128}));
                       args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(false));
                       args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"ThresholdMaximumConnectedComponents", FilterTraits<ThresholdMaximumConnectedComponentsImageFilter>::uuid, Legacy("dc0f6771-87bf-457f-8430-2d943e039a24"), "cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ThresholdMaximumConnectedComponentsImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(uint32{0}));
                       args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_UpperBoundary_Key, std::make_any<float64>(65536.0));
                       args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
                       args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
                     }});
    specs.push_back({"Mask", FilterTraits<MaskImageFilter>::uuid, Legacy("d3138266-3f34-4d6e-8e21-904c94351293"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MaskImageFilter>(args, geom, in, out);
                       args.insertOrAssign(MaskImageFilter::k_MaskImageDataPath_Key, std::make_any<DataPath>(in.getParent().createChildPath("Mask")));
                       args.insertOrAssign(MaskImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
                     },
                     PrepareMaskInput});

    // --- Binary morphology (STRICTLY binary input; WhiteDots.png is {0,255} so fg=255/bg=0). Moderate Ball r{2,2,2}. ---
    specs.push_back({"BinaryDilate", FilterTraits<BinaryDilateImageFilter>::uuid, Legacy("d4e973cb-c501-4c64-af26-fcf791c0f36d"), "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryDilateImageFilter>(args, geom, in, out);
                       SetKernel<BinaryDilateImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BinaryDilateImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryDilateImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(BinaryDilateImageFilter::k_BoundaryToForeground_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"BinaryErode", FilterTraits<BinaryErodeImageFilter>::uuid, Legacy("243dd30b-d1f0-42ad-8b47-77d57f9fc262"), "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryErodeImageFilter>(args, geom, in, out);
                       SetKernel<BinaryErodeImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BinaryErodeImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryErodeImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(BinaryErodeImageFilter::k_BoundaryToForeground_Key, std::make_any<bool>(true)); // erosion default
                     }});
    specs.push_back({"BinaryMorphologicalOpening", FilterTraits<BinaryMorphologicalOpeningImageFilter>::uuid, Legacy("861ccb46-dbce-41bf-a66f-25cc18cd1073"), "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryMorphologicalOpeningImageFilter>(args, geom, in, out);
                       SetKernel<BinaryMorphologicalOpeningImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BinaryMorphologicalOpeningImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryMorphologicalOpeningImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                     }});

    // --- Grayscale morphology (STAPLE1.png, uint8). Moderate Ball r{2,2,2}; SafeBorder=true is the filter default. ---
    specs.push_back({"GrayscaleDilate", FilterTraits<GrayscaleDilateImageFilter>::uuid, Legacy("944f4d1a-adb1-401a-9c0f-e2085ef2f6dc"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleDilateImageFilter>(args, geom, in, out);
                       SetKernel<GrayscaleDilateImageFilter>(args, 1, {2, 2, 2});
                     }});
    specs.push_back({"GrayscaleErode", FilterTraits<GrayscaleErodeImageFilter>::uuid, Legacy("3ceb1e6f-f4a4-4d8f-82d5-ecd20d4fc285"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleErodeImageFilter>(args, geom, in, out);
                       SetKernel<GrayscaleErodeImageFilter>(args, 1, {2, 2, 2});
                     }});
    specs.push_back({"GrayscaleMorphologicalClosing", FilterTraits<GrayscaleMorphologicalClosingImageFilter>::uuid, Legacy("8b859b54-93d4-4341-8fbf-85e1e461d5b5"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleMorphologicalClosingImageFilter>(args, geom, in, out);
                       SetKernel<GrayscaleMorphologicalClosingImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(GrayscaleMorphologicalClosingImageFilter::k_SafeBorder_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"MorphologicalGradient", FilterTraits<MorphologicalGradientImageFilter>::uuid, Legacy("9103009a-8884-4097-8c34-aec7019589ea"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MorphologicalGradientImageFilter>(args, geom, in, out);
                       SetKernel<MorphologicalGradientImageFilter>(args, 1, {2, 2, 2});
                     }});
    specs.push_back({"BlackTopHat", FilterTraits<BlackTopHatImageFilter>::uuid, Legacy("b7471b64-2282-449b-82b4-3ce359e9dda0"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BlackTopHatImageFilter>(args, geom, in, out);
                       SetKernel<BlackTopHatImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BlackTopHatImageFilter::k_SafeBorder_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"WhiteTopHat", FilterTraits<WhiteTopHatImageFilter>::uuid, Legacy("2f377682-d0a8-4dea-8c68-60c2c523a074"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<WhiteTopHatImageFilter>(args, geom, in, out);
                       SetKernel<WhiteTopHatImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(WhiteTopHatImageFilter::k_SafeBorder_Key, std::make_any<bool>(true));
                     }});
    // ErodeObjectMorphology: single-object morphology on RA-Slice-Short.nrrd (int16); ObjectValue 1 is exactly
    // representable in int16.
    specs.push_back({"ErodeObjectMorphology", FilterTraits<ErodeObjectMorphologyImageFilter>::uuid, Legacy("db3fe379-6ce4-4be3-baca-1cbff00aca6a"), "RA-Slice-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ErodeObjectMorphologyImageFilter>(args, geom, in, out);
                       SetKernel<ErodeObjectMorphologyImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                     }});

    // --- Morphological reconstruction (Ball r{2,2,2}). BinaryOpeningByReconstruction needs strictly binary input, so
    //     WhiteDots.png + fg=255/bg=0 (2th_cthead1.png is {0,100,200} -> rejected as non-binary). ---
    specs.push_back({"BinaryOpeningByReconstruction", FilterTraits<BinaryOpeningByReconstructionImageFilter>::uuid, Legacy("02c15392-382c-406d-a174-07ea6fa11b67"), "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryOpeningByReconstructionImageFilter>(args, geom, in, out);
                       SetKernel<BinaryOpeningByReconstructionImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"ClosingByReconstruction", FilterTraits<ClosingByReconstructionImageFilter>::uuid, Legacy("b5ff32a8-e799-4f72-8d13-e2581f748562"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ClosingByReconstructionImageFilter>(args, geom, in, out);
                       SetKernel<ClosingByReconstructionImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(ClosingByReconstructionImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(ClosingByReconstructionImageFilter::k_PreserveIntensities_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"OpeningByReconstruction", FilterTraits<OpeningByReconstructionImageFilter>::uuid, Legacy("c4225a23-0b23-4782-b509-296fb39a672b"), "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<OpeningByReconstructionImageFilter>(args, geom, in, out);
                       SetKernel<OpeningByReconstructionImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(OpeningByReconstructionImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(OpeningByReconstructionImageFilter::k_PreserveIntensities_Key, std::make_any<bool>(false));
                     }});

    // --- Contour / regional extrema / relabel / thinning. ---
    specs.push_back({"BinaryContour", FilterTraits<BinaryContourImageFilter>::uuid, Legacy("ed214e76-6954-49b4-817b-13f92315e722"), "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryContourImageFilter>(args, geom, in, out);
                       args.insertOrAssign(BinaryContourImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(BinaryContourImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryContourImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                     }});
    specs.push_back({"LabelContour", FilterTraits<LabelContourImageFilter>::uuid, Legacy("b64ff45d-3661-4926-9e87-5dd7b379b261"), "2th_cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<LabelContourImageFilter>(args, geom, in, out);
                       args.insertOrAssign(LabelContourImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(LabelContourImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                     }});
    specs.push_back({"RegionalMaxima", FilterTraits<RegionalMaximaImageFilter>::uuid, Legacy("a2b8a295-5730-477a-b97b-8a0b0400397c"), "cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<RegionalMaximaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(RegionalMaximaImageFilter::k_ForegroundValue_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(RegionalMaximaImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(RegionalMaximaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(RegionalMaximaImageFilter::k_FlatIsMaxima_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"RegionalMinima", FilterTraits<RegionalMinimaImageFilter>::uuid, Legacy("7ec0883e-ac48-40e9-8b97-11bdfde721e2"), "cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<RegionalMinimaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(RegionalMinimaImageFilter::k_ForegroundValue_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(RegionalMinimaImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(RegionalMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(RegionalMinimaImageFilter::k_FlatIsMinima_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"ValuedRegionalMaxima", FilterTraits<ValuedRegionalMaximaImageFilter>::uuid, Legacy("2c0bb4f6-69fe-4c43-a32e-21b4b11efcff"), "cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ValuedRegionalMaximaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ValuedRegionalMaximaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"ValuedRegionalMinima", FilterTraits<ValuedRegionalMinimaImageFilter>::uuid, Legacy("38548e01-6a3c-49fb-b7b6-489f965cc61e"), "cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ValuedRegionalMinimaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ValuedRegionalMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"HConvex", FilterTraits<HConvexImageFilter>::uuid, Legacy("620240a1-0b04-4bc7-a4c3-531917de4bc0"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<HConvexImageFilter>(args, geom, in, out);
                       args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(2.0));
                       args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"HMaxima", FilterTraits<HMaximaImageFilter>::uuid, Legacy("40039f72-30b0-4a3f-8ea4-2f76c5f65bc1"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<HMaximaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(HMaximaImageFilter::k_Height_Key, std::make_any<float64>(2.0));
                     }});
    specs.push_back({"HMinima", FilterTraits<HMinimaImageFilter>::uuid, Legacy("9b2eb24b-90e5-41c0-9230-1044170ee8ea"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<HMinimaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(2.0));
                       args.insertOrAssign(HMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"GrayscaleGrindPeak", FilterTraits<GrayscaleGrindPeakImageFilter>::uuid, Legacy("6aa5b193-c290-4fe3-a409-7759a62d48ea"), "RA-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleGrindPeakImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    // RelabelComponent: input is an ALREADY-labeled integer image (simple-label-d.png).
    specs.push_back({"RelabelComponent", FilterTraits<RelabelComponentImageFilter>::uuid, Legacy("37e29d16-1020-478c-a506-c121e8f670ad"), "simple-label-d.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<RelabelComponentImageFilter>(args, geom, in, out);
                       args.insertOrAssign(RelabelComponentImageFilter::k_MinimumObjectSize_Key, std::make_any<uint64>(uint64{0}));
                       args.insertOrAssign(RelabelComponentImageFilter::k_SortByObjectSize_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"BinaryThinning", FilterTraits<BinaryThinningImageFilter>::uuid, Legacy("8fcd24cb-769d-400f-97cf-9b4dad1b8cd2"), "BlackDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<BinaryThinningImageFilter>(args, geom, in, out); }});

    // --- Axis projections (project along X into a NEW geometry; see SetProjectionKeys). StandardDeviation/Sum
    //     projections have DISABLED legacy ITK counterparts -> nullopt. ---
    // BinaryProjection: WhiteDots.png is {0,255}, so fg=255/bg=0 (default fg=1 would map everything to background).
    specs.push_back({"BinaryProjection",
                     FilterTraits<BinaryProjectionImageFilter>::uuid,
                     Legacy("04ea495e-2cf0-4dba-8d29-cf33a38c094d"),
                     "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetProjectionKeys<BinaryProjectionImageFilter>(args, geom, in, out);
                       args.insertOrAssign(BinaryProjectionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryProjectionImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                     },
                     {},
                     true});
    specs.push_back({"MeanProjection",
                     FilterTraits<MeanProjectionImageFilter>::uuid,
                     Legacy("62ffddba-cc57-45fc-a93a-27914eea11ad"),
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<MeanProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});
    specs.push_back({"MedianProjection",
                     FilterTraits<MedianProjectionImageFilter>::uuid,
                     Legacy("00e48f6b-8a00-414f-b3d9-49d48a3f9a00"),
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<MedianProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});
    specs.push_back({"MinimumProjection",
                     FilterTraits<MinimumProjectionImageFilter>::uuid,
                     Legacy("86898336-8680-4c4e-b166-3f8de9e3d4f2"),
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<MinimumProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});
    specs.push_back({"StandardDeviationProjection",
                     FilterTraits<StandardDeviationProjectionImageFilter>::uuid,
                     std::nullopt,
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<StandardDeviationProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});
    specs.push_back({"SumProjection",
                     FilterTraits<SumProjectionImageFilter>::uuid,
                     std::nullopt,
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<SumProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});

    // --- Distance maps + zero crossing. The Danielsson/ASD/SignedDanielsson maps require integer input
    //     (2th_cthead1.png). ZeroCrossing needs a signed input (the signed distance map 2th_cthead1_distance.nrrd). ---
    specs.push_back({"ApproximateSignedDistanceMap", FilterTraits<ApproximateSignedDistanceMapImageFilter>::uuid, Legacy("87ed0d3a-c394-4bb5-ac7f-6cc746984b09"), "2th_cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ApproximateSignedDistanceMapImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_InsideValue_Key, std::make_any<float64>(100.0));
                       args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
                     },
                     [](DataStructure& dataStructure, const ip_bench::BenchmarkInputContext& context) {
                       return BuildPrimaryInput(dataStructure, context, RequireUint8Values({uint8{0}, uint8{100}}, "ApproximateSignedDistanceMap inside/outside classes"));
                     }});
    specs.push_back({"DanielssonDistanceMap", FilterTraits<DanielssonDistanceMapImageFilter>::uuid, Legacy("f0cd4faf-a676-41ed-9ea5-859035f94836"), "2th_cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<DanielssonDistanceMapImageFilter>(args, geom, in, out);
                       args.insertOrAssign(DanielssonDistanceMapImageFilter::k_InputIsBinary_Key, std::make_any<bool>(false));
                       args.insertOrAssign(DanielssonDistanceMapImageFilter::k_SquaredDistance_Key, std::make_any<bool>(false));
                       args.insertOrAssign(DanielssonDistanceMapImageFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"SignedDanielssonDistanceMap", FilterTraits<SignedDanielssonDistanceMapImageFilter>::uuid, Legacy("2f42e771-1d84-4468-8991-9a1fad7eb740"), "2th_cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<SignedDanielssonDistanceMapImageFilter>(args, geom, in, out);
                       args.insertOrAssign(SignedDanielssonDistanceMapImageFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
                       args.insertOrAssign(SignedDanielssonDistanceMapImageFilter::k_SquaredDistance_Key, std::make_any<bool>(false));
                       args.insertOrAssign(SignedDanielssonDistanceMapImageFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"IsoContourDistance", FilterTraits<IsoContourDistanceImageFilter>::uuid, Legacy("e82fa143-7ac2-4c09-a88c-9ea71d47d594"), "2th_cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<IsoContourDistanceImageFilter>(args, geom, in, out);
                       args.insertOrAssign(IsoContourDistanceImageFilter::k_LevelSetValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(IsoContourDistanceImageFilter::k_FarValue_Key, std::make_any<float64>(10.0));
                     }});
    specs.push_back({"ZeroCrossing", FilterTraits<ZeroCrossingImageFilter>::uuid, Legacy("89a14057-776a-4e35-80b6-69361e078394"), "2th_cthead1_distance.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ZeroCrossingImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ZeroCrossingImageFilter::k_ForegroundValue_Key, std::make_any<uint8>(uint8{1}));
                       args.insertOrAssign(ZeroCrossingImageFilter::k_BackgroundValue_Key, std::make_any<uint8>(uint8{0}));
                     }});

    // --- Smoothing / diffusion (RA-Float.nrrd, float32). MinMaxCurvatureFlow + GradientAnisotropicDiffusion reject
    //     integer input; GradientAnisotropicDiffusion uses a sub-CFL time step (0.0625) to suppress the CFL warning. ---
    specs.push_back({"DiscreteGaussian", FilterTraits<DiscreteGaussianImageFilter>::uuid, Legacy("025edc1a-986d-4005-92d1-545dfdc13abd"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<DiscreteGaussianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(DiscreteGaussianImageFilter::k_Variance_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
                       args.insertOrAssign(DiscreteGaussianImageFilter::k_MaximumKernelWidth_Key, std::make_any<uint32>(uint32{32}));
                       args.insertOrAssign(DiscreteGaussianImageFilter::k_MaximumError_Key, std::make_any<std::vector<float64>>(std::vector<float64>{0.01, 0.01, 0.01}));
                       args.insertOrAssign(DiscreteGaussianImageFilter::k_UseImageSpacing_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"GradientMagnitudeRecursiveGaussian", FilterTraits<GradientMagnitudeRecursiveGaussianImageFilter>::uuid, Legacy("32db4ae4-4087-4688-874a-b1d725188f18"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GradientMagnitudeRecursiveGaussianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GradientMagnitudeRecursiveGaussianImageFilter::k_Sigma_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(GradientMagnitudeRecursiveGaussianImageFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"LaplacianRecursiveGaussian", FilterTraits<LaplacianRecursiveGaussianImageFilter>::uuid, Legacy("782d76a4-e3f6-4c2a-a1b0-7456a3e77f24"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<LaplacianRecursiveGaussianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(LaplacianRecursiveGaussianImageFilter::k_Sigma_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(LaplacianRecursiveGaussianImageFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"CurvatureFlow", FilterTraits<CurvatureFlowImageFilter>::uuid, Legacy("fe5b2ed3-54dd-4207-ad88-48a95134684a"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<CurvatureFlowImageFilter>(args, geom, in, out);
                       args.insertOrAssign(CurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(0.05));
                       args.insertOrAssign(CurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(uint32{5}));
                     }});
    specs.push_back({"MinMaxCurvatureFlow", FilterTraits<MinMaxCurvatureFlowImageFilter>::uuid, Legacy("b836c081-6692-411d-81d0-a50afce6b288"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MinMaxCurvatureFlowImageFilter>(args, geom, in, out);
                       args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(0.05));
                       args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(uint32{5}));
                       args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_StencilRadius_Key, std::make_any<int32>(int32{2}));
                     }});
    specs.push_back({"GradientAnisotropicDiffusion", FilterTraits<GradientAnisotropicDiffusionImageFilter>::uuid, Legacy("9dcef77b-e7d2-4a2a-b310-bfa80e8ea7c5"), "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GradientAnisotropicDiffusionImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(0.0625));
                       args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
                       args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(uint32{1}));
                       args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(uint32{5}));
                     }});
    specs.push_back({"AdaptiveHistogramEqualization", FilterTraits<AdaptiveHistogramEqualizationImageFilter>::uuid, Legacy("ea3e7439-8327-4190-8ff7-49ecc321718f"), "sf4.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<AdaptiveHistogramEqualizationImageFilter>(args, geom, in, out);
                       args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{10, 10, 10}));
                       args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Alpha_Key, std::make_any<float32>(1.0f));
                       args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Beta_Key, std::make_any<float32>(0.25f));
                     },
                     PrepareAdaptiveHistogramInput});
    specs.push_back({"MorphologicalWatershedFromMarkers", FilterTraits<MorphologicalWatershedFromMarkersImageFilter>::uuid, std::nullopt, "cthead1-grad-mag.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MorphologicalWatershedFromMarkersImageFilter>(args, geom, in, out);
                       args.insertOrAssign(MorphologicalWatershedFromMarkersImageFilter::k_MarkerImageDataPath_Key, std::make_any<DataPath>(in.getParent().createChildPath("Marker")));
                       args.insertOrAssign(MorphologicalWatershedFromMarkersImageFilter::k_MarkWatershedLine_Key, std::make_any<bool>(true));
                       args.insertOrAssign(MorphologicalWatershedFromMarkersImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     },
                     PrepareWatershedMarkersInput});

    return specs;
  }();
  return registry;
}
} // namespace ip_bench
