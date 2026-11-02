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

} // namespace

namespace ip_bench
{
const std::vector<BenchmarkFilterSpec>& GetBenchmarkFilterRegistry()
{
  static const std::vector<BenchmarkFilterSpec> registry = [] {
    std::vector<BenchmarkFilterSpec> specs;

    // --- Fully wired (arg presets + input verified against each filter's [ItkGolden]/parity test) ---

    // Abs: SameAsInput, no extra params.
    specs.push_back({"Abs", FilterTraits<AbsImageFilter>::uuid, "RA-Slice-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<AbsImageFilter>(args, geom, in, out); }});

    // Median: radius {1,1,1} (the ITK golden "defaults" case).
    specs.push_back({"Median", FilterTraits<MedianImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MedianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(MedianImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
                     }});

    // GrayscaleMorphologicalOpening: moderate Ball kernel r{2,2,2}, Safe Border = true (the filter default). This is a
    // representative, NON-pathological kernel. The golden test's Box r{20,5,1} kernel is a huge decomposable-Box line
    // filter, which distorts the timing. KernelType choices are {Annulus=0, Ball=1, Box=2, Cross=3}.
    specs.push_back({"GrayscaleMorphologicalOpening", FilterTraits<GrayscaleMorphologicalOpeningImageFilter>::uuid, "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleMorphologicalOpeningImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(ChoicesParameter::ValueType{1})); // Ball
                       args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{2, 2, 2}));
                       args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_SafeBorder_Key, std::make_any<bool>(true)); // filter default
                     }});

    // ConnectedComponent: FullyConnected = false (the ITK golden "default" case).
    specs.push_back({"ConnectedComponent", FilterTraits<ConnectedComponentImageFilter>::uuid, "WhiteDots.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ConnectedComponentImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ConnectedComponentImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});

    // SignedMaurerDistanceMap: ITK-default params (the golden helper sets no extra params; defaults apply). Integer input.
    specs.push_back({"SignedMaurerDistanceMap", FilterTraits<SignedMaurerDistanceMapImageFilter>::uuid, "2th_cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<SignedMaurerDistanceMapImageFilter>(args, geom, in, out); }});

    // GradientMagnitude: ITK-default params (UseImageSpacing defaults true).
    specs.push_back({"GradientMagnitude", FilterTraits<GradientMagnitudeImageFilter>::uuid, "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<GradientMagnitudeImageFilter>(args, geom, in, out); }});

    // MaximumProjection: project along X (dim 0) into a NEW geometry so the tiled input survives repeats (in-place
    // would collapse it).
    specs.push_back({"MaximumProjection",
                     FilterTraits<MaximumProjectionImageFilter>::uuid,
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

    // --- Fully wired (real input + representative arg preset verified against each filter's [ItkGolden]/parity
    //     test). ---

    // BinaryMorphologicalClosing: needs a STRICTLY binary input. WhiteDots.png is {0,255} (the WithBorder ITK golden's
    // input), so Foreground = 255 -> internal background 0 satisfies the strictly-{fg,internal-bg} contract; a non-binary
    // input (e.g. 2th_cthead1.png / STAPLE1.png) trips the k_NonBinaryInput safeguard at execute. Moderate Ball r{2,2,2}
    // (the golden's r{5,5,5} is larger than needed), Safe Border = false (the golden's WithBorder=false case).
    // KernelType choices are {Annulus=0, Ball=1, Box=2, Cross=3}.
    specs.push_back({"BinaryMorphologicalClosing", FilterTraits<BinaryMorphologicalClosingImageFilter>::uuid, "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryMorphologicalClosingImageFilter>(args, geom, in, out);
                       args.insertOrAssign(BinaryMorphologicalClosingImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(ChoicesParameter::ValueType{1})); // Ball
                       args.insertOrAssign(BinaryMorphologicalClosingImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{2, 2, 2}));
                       args.insertOrAssign(BinaryMorphologicalClosingImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryMorphologicalClosingImageFilter::k_SafeBorder_Key, std::make_any<bool>(false));
                     }});

    // GrayscaleFillhole: RA-Short.nrrd (int16), the GrayscaleFillhole1 ITK golden's input. FullyConnected = false
    // (the ITK default). No kernel.
    specs.push_back({"GrayscaleFillhole", FilterTraits<GrayscaleFillholeImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleFillholeImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GrayscaleFillholeImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});

    // CurvatureAnisotropicDiffusion: RA-Float.nrrd (float32 -- this filter rejects integer input). TimeStep 0.01 (the
    // ITK golden "defaults" case value, safely below the CFL stability bound for this image so no stability warning
    // fires); ConductanceParameter/ConductanceScalingUpdateInterval/NumberOfIterations left at their defaults
    // (3.0 / 1 / 5).
    specs.push_back({"CurvatureAnisotropicDiffusion", FilterTraits<CurvatureAnisotropicDiffusionImageFilter>::uuid, "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<CurvatureAnisotropicDiffusionImageFilter>(args, geom, in, out);
                       args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(0.01));
                     }});

    // SmoothingRecursiveGaussian: RA-Float.nrrd (float32 -- this filter rejects UNSIGNED input). Sigma {2,2,2} (a
    // moderate representative blur; the recursive Deriche pass is O(N) regardless of sigma), NormalizeAcrossScale =
    // false (a no-op for order-0 smoothing anyway).
    specs.push_back({"SmoothingRecursiveGaussian", FilterTraits<SmoothingRecursiveGaussianImageFilter>::uuid, "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<SmoothingRecursiveGaussianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{2.0, 2.0, 2.0}));
                       args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
                     }});

    // DilateObjectMorphology: RA-Slice-Short.nrrd (int16), the "short" ITK golden's input; ObjectValue 1 (the ITK
    // default, exactly representable in int16). Moderate Ball r{2,2,2}. KernelType choices are
    // {Annulus=0, Ball=1, Box=2, Cross=3}.
    specs.push_back(
        {"DilateObjectMorphology", FilterTraits<DilateObjectMorphologyImageFilter>::uuid, "RA-Slice-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
           SetStandardKeys<DilateObjectMorphologyImageFilter>(args, geom, in, out);
           args.insertOrAssign(DilateObjectMorphologyImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(ChoicesParameter::ValueType{1})); // Ball
           args.insertOrAssign(DilateObjectMorphologyImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{2, 2, 2}));
           args.insertOrAssign(DilateObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(1.0));
         }});

    // MorphologicalWatershed: cthead1-grad-mag.nrrd, the ITK golden's input (a gradient-magnitude image -- the natural
    // watershed input). Level 0.0, MarkWatershedLine = true, FullyConnected = false (the golden "defaults" case).
    // Output is a fixed uint32 label image.
    specs.push_back({"MorphologicalWatershed", FilterTraits<MorphologicalWatershedImageFilter>::uuid, "cthead1-grad-mag.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MorphologicalWatershedImageFilter>(args, geom, in, out);
                       args.insertOrAssign(MorphologicalWatershedImageFilter::k_Level_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(MorphologicalWatershedImageFilter::k_MarkWatershedLine_Key, std::make_any<bool>(true));
                       args.insertOrAssign(MorphologicalWatershedImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});

    // =========================================================================
    // Full-plugin expansion: every remaining registered ImageProcessing filter (the 13 above are the original
    // representative subset). Inputs mirror each filter's [ItkGolden]/parity test. Args are representative,
    // NON-pathological values (moderate Ball r{2,2,2} kernels, ITK parameter defaults) -- not correctness-golden
    // pathological kernels.
    // =========================================================================

    // --- Unary pointwise math (SameAsInput; no extra parameters). Domain-restricted ops (Acos/Asin/Log/Log10/Sqrt)
    //     use Ramp-Zero-One-Float so the real input stays in range; Cos/Sin/Tan/Exp/ExpNegative/Square accept any
    //     scalar. Not is integer-only (STAPLE1.png is uint8). ---
    specs.push_back({"Acos", FilterTraits<AcosImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<AcosImageFilter>(args, geom, in, out); }});
    specs.push_back({"Asin", FilterTraits<AsinImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<AsinImageFilter>(args, geom, in, out); }});
    specs.push_back({"Atan", FilterTraits<AtanImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<AtanImageFilter>(args, geom, in, out); }});
    specs.push_back({"Cos", FilterTraits<CosImageFilter>::uuid, "RA-Slice-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<CosImageFilter>(args, geom, in, out); }});
    specs.push_back({"Sin", FilterTraits<SinImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<SinImageFilter>(args, geom, in, out); }});
    specs.push_back({"Tan", FilterTraits<TanImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<TanImageFilter>(args, geom, in, out); }});
    specs.push_back({"Exp", FilterTraits<ExpImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<ExpImageFilter>(args, geom, in, out); }});
    specs.push_back({"ExpNegative", FilterTraits<ExpNegativeImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<ExpNegativeImageFilter>(args, geom, in, out); }});
    specs.push_back({"Log", FilterTraits<LogImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<LogImageFilter>(args, geom, in, out); }});
    specs.push_back({"Log10", FilterTraits<Log10ImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<Log10ImageFilter>(args, geom, in, out); }});
    specs.push_back({"Sqrt", FilterTraits<SqrtImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<SqrtImageFilter>(args, geom, in, out); }});
    specs.push_back({"Square", FilterTraits<SquareImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<SquareImageFilter>(args, geom, in, out); }});
    specs.push_back({"Not", FilterTraits<NotImageFilter>::uuid, "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<NotImageFilter>(args, geom, in, out); }});

    // --- Intensity transforms. Values are ITK parameter defaults (representative, non-pathological). ---
    specs.push_back({"InvertIntensity", FilterTraits<InvertIntensityImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<InvertIntensityImageFilter>(args, geom, in, out);
                       args.insertOrAssign(InvertIntensityImageFilter::k_Maximum_Key, std::make_any<float64>(255.0));
                     }});
    specs.push_back({"BoundedReciprocal", FilterTraits<BoundedReciprocalImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<BoundedReciprocalImageFilter>(args, geom, in, out); }});
    specs.push_back({"Sigmoid", FilterTraits<SigmoidImageFilter>::uuid, "Ramp-Zero-One-Float.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<SigmoidImageFilter>(args, geom, in, out);
                       args.insertOrAssign(SigmoidImageFilter::k_Alpha_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(SigmoidImageFilter::k_Beta_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(SigmoidImageFilter::k_OutputMinimum_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(SigmoidImageFilter::k_OutputMaximum_Key, std::make_any<float64>(255.0));
                     }});
    specs.push_back({"RescaleIntensity", FilterTraits<RescaleIntensityImageFilter>::uuid, "RA-Float.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<RescaleIntensityImageFilter>(args, geom, in, out);
                       args.insertOrAssign(RescaleIntensityImageFilter::k_OutputMinimum_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(RescaleIntensityImageFilter::k_OutputMaximum_Key, std::make_any<float64>(255.0));
                     }});
    specs.push_back({"Normalize", FilterTraits<NormalizeImageFilter>::uuid, "Ramp-Up-Short.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<NormalizeImageFilter>(args, geom, in, out); }});
    specs.push_back(
        {"NormalizeToConstant", FilterTraits<NormalizeToConstantImageFilter>::uuid, "Ramp-Up-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
           SetStandardKeys<NormalizeToConstantImageFilter>(args, geom, in, out);
           args.insertOrAssign(NormalizeToConstantImageFilter::k_Constant_Key, std::make_any<float64>(1.0));
         }});
    specs.push_back({"IntensityWindowing", FilterTraits<IntensityWindowingImageFilter>::uuid, "RA-Float.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<IntensityWindowingImageFilter>(args, geom, in, out);
                       args.insertOrAssign(IntensityWindowingImageFilter::k_WindowMinimum_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(IntensityWindowingImageFilter::k_WindowMaximum_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(IntensityWindowingImageFilter::k_OutputMinimum_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(IntensityWindowingImageFilter::k_OutputMaximum_Key, std::make_any<float64>(255.0));
                     }});

    // --- Thresholding. Inside/Outside are uint8 (fixed uint8 output); thresholds are float64. ---
    specs.push_back({"BinaryThreshold", FilterTraits<BinaryThresholdImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryThresholdImageFilter>(args, geom, in, out);
                       args.insertOrAssign(BinaryThresholdImageFilter::k_LowerThreshold_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(BinaryThresholdImageFilter::k_UpperThreshold_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryThresholdImageFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
                       args.insertOrAssign(BinaryThresholdImageFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
                     }});
    specs.push_back({"Threshold", FilterTraits<ThresholdImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ThresholdImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ThresholdImageFilter::k_Lower_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(ThresholdImageFilter::k_Upper_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(ThresholdImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
                     }});
    specs.push_back({"DoubleThreshold", FilterTraits<DoubleThresholdImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<DoubleThresholdImageFilter>(args, geom, in, out);
                       args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold1_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold2_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold3_Key, std::make_any<float64>(254.0));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold4_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
                       args.insertOrAssign(DoubleThresholdImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back(
        {"OtsuMultipleThresholds", FilterTraits<OtsuMultipleThresholdsImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
           SetStandardKeys<OtsuMultipleThresholdsImageFilter>(args, geom, in, out);
           args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(uint8{1}));
           args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(uint8{0}));
           args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(uint32{128}));
           args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(false));
           args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(false));
         }});
    specs.push_back({"ThresholdMaximumConnectedComponents", FilterTraits<ThresholdMaximumConnectedComponentsImageFilter>::uuid, "cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ThresholdMaximumConnectedComponentsImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(uint32{0}));
                       args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_UpperBoundary_Key, std::make_any<float64>(65536.0));
                       args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
                       args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
                     }});
    specs.push_back({"Mask", FilterTraits<MaskImageFilter>::uuid, "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MaskImageFilter>(args, geom, in, out);
                       args.insertOrAssign(MaskImageFilter::k_MaskImageDataPath_Key, std::make_any<DataPath>(in.getParent().createChildPath("Mask")));
                       args.insertOrAssign(MaskImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
                     },
                     PrepareMaskInput});

    // --- Binary morphology (STRICTLY binary input; WhiteDots.png is {0,255} so fg=255/bg=0). Moderate Ball r{2,2,2}. ---
    specs.push_back({"BinaryDilate", FilterTraits<BinaryDilateImageFilter>::uuid, "WhiteDots.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryDilateImageFilter>(args, geom, in, out);
                       SetKernel<BinaryDilateImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BinaryDilateImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryDilateImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(BinaryDilateImageFilter::k_BoundaryToForeground_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"BinaryErode", FilterTraits<BinaryErodeImageFilter>::uuid, "WhiteDots.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryErodeImageFilter>(args, geom, in, out);
                       SetKernel<BinaryErodeImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BinaryErodeImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryErodeImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(BinaryErodeImageFilter::k_BoundaryToForeground_Key, std::make_any<bool>(true)); // erosion default
                     }});
    specs.push_back({"BinaryMorphologicalOpening", FilterTraits<BinaryMorphologicalOpeningImageFilter>::uuid, "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryMorphologicalOpeningImageFilter>(args, geom, in, out);
                       SetKernel<BinaryMorphologicalOpeningImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BinaryMorphologicalOpeningImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryMorphologicalOpeningImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                     }});

    // --- Grayscale morphology (STAPLE1.png, uint8). Moderate Ball r{2,2,2}; SafeBorder=true is the filter default. ---
    specs.push_back({"GrayscaleDilate", FilterTraits<GrayscaleDilateImageFilter>::uuid, "STAPLE1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleDilateImageFilter>(args, geom, in, out);
                       SetKernel<GrayscaleDilateImageFilter>(args, 1, {2, 2, 2});
                     }});
    specs.push_back({"GrayscaleErode", FilterTraits<GrayscaleErodeImageFilter>::uuid, "STAPLE1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleErodeImageFilter>(args, geom, in, out);
                       SetKernel<GrayscaleErodeImageFilter>(args, 1, {2, 2, 2});
                     }});
    specs.push_back({"GrayscaleMorphologicalClosing", FilterTraits<GrayscaleMorphologicalClosingImageFilter>::uuid, "STAPLE1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleMorphologicalClosingImageFilter>(args, geom, in, out);
                       SetKernel<GrayscaleMorphologicalClosingImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(GrayscaleMorphologicalClosingImageFilter::k_SafeBorder_Key, std::make_any<bool>(true));
                     }});
    specs.push_back(
        {"MorphologicalGradient", FilterTraits<MorphologicalGradientImageFilter>::uuid, "STAPLE1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
           SetStandardKeys<MorphologicalGradientImageFilter>(args, geom, in, out);
           SetKernel<MorphologicalGradientImageFilter>(args, 1, {2, 2, 2});
         }});
    specs.push_back({"BlackTopHat", FilterTraits<BlackTopHatImageFilter>::uuid, "STAPLE1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BlackTopHatImageFilter>(args, geom, in, out);
                       SetKernel<BlackTopHatImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BlackTopHatImageFilter::k_SafeBorder_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"WhiteTopHat", FilterTraits<WhiteTopHatImageFilter>::uuid, "STAPLE1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<WhiteTopHatImageFilter>(args, geom, in, out);
                       SetKernel<WhiteTopHatImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(WhiteTopHatImageFilter::k_SafeBorder_Key, std::make_any<bool>(true));
                     }});
    // ErodeObjectMorphology: single-object morphology on RA-Slice-Short.nrrd (int16); ObjectValue 1 is exactly
    // representable in int16.
    specs.push_back(
        {"ErodeObjectMorphology", FilterTraits<ErodeObjectMorphologyImageFilter>::uuid, "RA-Slice-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
           SetStandardKeys<ErodeObjectMorphologyImageFilter>(args, geom, in, out);
           SetKernel<ErodeObjectMorphologyImageFilter>(args, 1, {2, 2, 2});
           args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(1.0));
           args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
         }});

    // --- Morphological reconstruction (Ball r{2,2,2}). BinaryOpeningByReconstruction needs strictly binary input, so
    //     WhiteDots.png + fg=255/bg=0 (2th_cthead1.png is {0,100,200} -> rejected as non-binary). ---
    specs.push_back({"BinaryOpeningByReconstruction", FilterTraits<BinaryOpeningByReconstructionImageFilter>::uuid, "WhiteDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryOpeningByReconstructionImageFilter>(args, geom, in, out);
                       SetKernel<BinaryOpeningByReconstructionImageFilter>(args, 1, {2, 2, 2});
                       args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back(
        {"ClosingByReconstruction", FilterTraits<ClosingByReconstructionImageFilter>::uuid, "STAPLE1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
           SetStandardKeys<ClosingByReconstructionImageFilter>(args, geom, in, out);
           SetKernel<ClosingByReconstructionImageFilter>(args, 1, {2, 2, 2});
           args.insertOrAssign(ClosingByReconstructionImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
           args.insertOrAssign(ClosingByReconstructionImageFilter::k_PreserveIntensities_Key, std::make_any<bool>(false));
         }});
    specs.push_back(
        {"OpeningByReconstruction", FilterTraits<OpeningByReconstructionImageFilter>::uuid, "STAPLE1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
           SetStandardKeys<OpeningByReconstructionImageFilter>(args, geom, in, out);
           SetKernel<OpeningByReconstructionImageFilter>(args, 1, {2, 2, 2});
           args.insertOrAssign(OpeningByReconstructionImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
           args.insertOrAssign(OpeningByReconstructionImageFilter::k_PreserveIntensities_Key, std::make_any<bool>(false));
         }});

    // --- Contour / regional extrema / relabel / thinning. ---
    specs.push_back({"BinaryContour", FilterTraits<BinaryContourImageFilter>::uuid, "WhiteDots.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<BinaryContourImageFilter>(args, geom, in, out);
                       args.insertOrAssign(BinaryContourImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(BinaryContourImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0));
                       args.insertOrAssign(BinaryContourImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                     }});
    specs.push_back({"LabelContour", FilterTraits<LabelContourImageFilter>::uuid, "2th_cthead1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<LabelContourImageFilter>(args, geom, in, out);
                       args.insertOrAssign(LabelContourImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(LabelContourImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                     }});
    specs.push_back({"RegionalMaxima", FilterTraits<RegionalMaximaImageFilter>::uuid, "cthead1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<RegionalMaximaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(RegionalMaximaImageFilter::k_ForegroundValue_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(RegionalMaximaImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(RegionalMaximaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(RegionalMaximaImageFilter::k_FlatIsMaxima_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"RegionalMinima", FilterTraits<RegionalMinimaImageFilter>::uuid, "cthead1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<RegionalMinimaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(RegionalMinimaImageFilter::k_ForegroundValue_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(RegionalMinimaImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(RegionalMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                       args.insertOrAssign(RegionalMinimaImageFilter::k_FlatIsMinima_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"ValuedRegionalMaxima", FilterTraits<ValuedRegionalMaximaImageFilter>::uuid, "cthead1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ValuedRegionalMaximaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ValuedRegionalMaximaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"ValuedRegionalMinima", FilterTraits<ValuedRegionalMinimaImageFilter>::uuid, "cthead1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ValuedRegionalMinimaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ValuedRegionalMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"HConvex", FilterTraits<HConvexImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<HConvexImageFilter>(args, geom, in, out);
                       args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(2.0));
                       args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"HMaxima", FilterTraits<HMaximaImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<HMaximaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(HMaximaImageFilter::k_Height_Key, std::make_any<float64>(2.0));
                     }});
    specs.push_back({"HMinima", FilterTraits<HMinimaImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<HMinimaImageFilter>(args, geom, in, out);
                       args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(2.0));
                       args.insertOrAssign(HMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"GrayscaleGrindPeak", FilterTraits<GrayscaleGrindPeakImageFilter>::uuid, "RA-Short.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GrayscaleGrindPeakImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
                     }});
    // RelabelComponent: input is an ALREADY-labeled integer image (simple-label-d.png).
    specs.push_back({"RelabelComponent", FilterTraits<RelabelComponentImageFilter>::uuid, "simple-label-d.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<RelabelComponentImageFilter>(args, geom, in, out);
                       args.insertOrAssign(RelabelComponentImageFilter::k_MinimumObjectSize_Key, std::make_any<uint64>(uint64{0}));
                       args.insertOrAssign(RelabelComponentImageFilter::k_SortByObjectSize_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"BinaryThinning", FilterTraits<BinaryThinningImageFilter>::uuid, "BlackDots.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetStandardKeys<BinaryThinningImageFilter>(args, geom, in, out); }});

    // --- Axis projections (project along X into a NEW geometry; see SetProjectionKeys). ---
    // BinaryProjection: WhiteDots.png is {0,255}, so fg=255/bg=0 (default fg=1 would map everything to background).
    specs.push_back({"BinaryProjection",
                     FilterTraits<BinaryProjectionImageFilter>::uuid,
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
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<MeanProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});
    specs.push_back({"MedianProjection",
                     FilterTraits<MedianProjectionImageFilter>::uuid,
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<MedianProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});
    specs.push_back({"MinimumProjection",
                     FilterTraits<MinimumProjectionImageFilter>::uuid,
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<MinimumProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});
    specs.push_back({"StandardDeviationProjection",
                     FilterTraits<StandardDeviationProjectionImageFilter>::uuid,
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<StandardDeviationProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});
    specs.push_back({"SumProjection",
                     FilterTraits<SumProjectionImageFilter>::uuid,
                     "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) { SetProjectionKeys<SumProjectionImageFilter>(args, geom, in, out); },
                     {},
                     true});

    // --- Distance maps + zero crossing. The Danielsson/ASD/SignedDanielsson maps require integer input
    //     (2th_cthead1.png). ZeroCrossing needs a signed input (the signed distance map 2th_cthead1_distance.nrrd). ---
    specs.push_back({"ApproximateSignedDistanceMap", FilterTraits<ApproximateSignedDistanceMapImageFilter>::uuid, "2th_cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ApproximateSignedDistanceMapImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_InsideValue_Key, std::make_any<float64>(100.0));
                       args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
                     },
                     [](DataStructure& dataStructure, const ip_bench::BenchmarkInputContext& context) {
                       return BuildPrimaryInput(dataStructure, context, RequireUint8Values({uint8{0}, uint8{100}}, "ApproximateSignedDistanceMap inside/outside classes"));
                     }});
    specs.push_back(
        {"DanielssonDistanceMap", FilterTraits<DanielssonDistanceMapImageFilter>::uuid, "2th_cthead1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
           SetStandardKeys<DanielssonDistanceMapImageFilter>(args, geom, in, out);
           args.insertOrAssign(DanielssonDistanceMapImageFilter::k_InputIsBinary_Key, std::make_any<bool>(false));
           args.insertOrAssign(DanielssonDistanceMapImageFilter::k_SquaredDistance_Key, std::make_any<bool>(false));
           args.insertOrAssign(DanielssonDistanceMapImageFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
         }});
    specs.push_back({"SignedDanielssonDistanceMap", FilterTraits<SignedDanielssonDistanceMapImageFilter>::uuid, "2th_cthead1.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<SignedDanielssonDistanceMapImageFilter>(args, geom, in, out);
                       args.insertOrAssign(SignedDanielssonDistanceMapImageFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
                       args.insertOrAssign(SignedDanielssonDistanceMapImageFilter::k_SquaredDistance_Key, std::make_any<bool>(false));
                       args.insertOrAssign(SignedDanielssonDistanceMapImageFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"IsoContourDistance", FilterTraits<IsoContourDistanceImageFilter>::uuid, "2th_cthead1.png", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<IsoContourDistanceImageFilter>(args, geom, in, out);
                       args.insertOrAssign(IsoContourDistanceImageFilter::k_LevelSetValue_Key, std::make_any<float64>(0.0));
                       args.insertOrAssign(IsoContourDistanceImageFilter::k_FarValue_Key, std::make_any<float64>(10.0));
                     }});
    specs.push_back({"ZeroCrossing", FilterTraits<ZeroCrossingImageFilter>::uuid, "2th_cthead1_distance.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<ZeroCrossingImageFilter>(args, geom, in, out);
                       args.insertOrAssign(ZeroCrossingImageFilter::k_ForegroundValue_Key, std::make_any<uint8>(uint8{1}));
                       args.insertOrAssign(ZeroCrossingImageFilter::k_BackgroundValue_Key, std::make_any<uint8>(uint8{0}));
                     }});

    // --- Smoothing / diffusion (RA-Float.nrrd, float32). MinMaxCurvatureFlow + GradientAnisotropicDiffusion reject
    //     integer input; GradientAnisotropicDiffusion uses a sub-CFL time step (0.0625) to suppress the CFL warning. ---
    specs.push_back({"DiscreteGaussian", FilterTraits<DiscreteGaussianImageFilter>::uuid, "RA-Float.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<DiscreteGaussianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(DiscreteGaussianImageFilter::k_Variance_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
                       args.insertOrAssign(DiscreteGaussianImageFilter::k_MaximumKernelWidth_Key, std::make_any<uint32>(uint32{32}));
                       args.insertOrAssign(DiscreteGaussianImageFilter::k_MaximumError_Key, std::make_any<std::vector<float64>>(std::vector<float64>{0.01, 0.01, 0.01}));
                       args.insertOrAssign(DiscreteGaussianImageFilter::k_UseImageSpacing_Key, std::make_any<bool>(true));
                     }});
    specs.push_back({"GradientMagnitudeRecursiveGaussian", FilterTraits<GradientMagnitudeRecursiveGaussianImageFilter>::uuid, "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GradientMagnitudeRecursiveGaussianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GradientMagnitudeRecursiveGaussianImageFilter::k_Sigma_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(GradientMagnitudeRecursiveGaussianImageFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"LaplacianRecursiveGaussian", FilterTraits<LaplacianRecursiveGaussianImageFilter>::uuid, "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<LaplacianRecursiveGaussianImageFilter>(args, geom, in, out);
                       args.insertOrAssign(LaplacianRecursiveGaussianImageFilter::k_Sigma_Key, std::make_any<float64>(1.0));
                       args.insertOrAssign(LaplacianRecursiveGaussianImageFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
                     }});
    specs.push_back({"CurvatureFlow", FilterTraits<CurvatureFlowImageFilter>::uuid, "RA-Float.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<CurvatureFlowImageFilter>(args, geom, in, out);
                       args.insertOrAssign(CurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(0.05));
                       args.insertOrAssign(CurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(uint32{5}));
                     }});
    specs.push_back({"MinMaxCurvatureFlow", FilterTraits<MinMaxCurvatureFlowImageFilter>::uuid, "RA-Float.nrrd", [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<MinMaxCurvatureFlowImageFilter>(args, geom, in, out);
                       args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(0.05));
                       args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(uint32{5}));
                       args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_StencilRadius_Key, std::make_any<int32>(int32{2}));
                     }});
    specs.push_back({"GradientAnisotropicDiffusion", FilterTraits<GradientAnisotropicDiffusionImageFilter>::uuid, "RA-Float.nrrd",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<GradientAnisotropicDiffusionImageFilter>(args, geom, in, out);
                       args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(0.0625));
                       args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
                       args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(uint32{1}));
                       args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(uint32{5}));
                     }});
    specs.push_back({"AdaptiveHistogramEqualization", FilterTraits<AdaptiveHistogramEqualizationImageFilter>::uuid, "sf4.png",
                     [](Arguments& args, const DataPath& geom, const DataPath& in, const std::string& out) {
                       SetStandardKeys<AdaptiveHistogramEqualizationImageFilter>(args, geom, in, out);
                       args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{10, 10, 10}));
                       args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Alpha_Key, std::make_any<float32>(1.0f));
                       args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Beta_Key, std::make_any<float32>(0.25f));
                     },
                     PrepareAdaptiveHistogramInput});
    specs.push_back({"MorphologicalWatershedFromMarkers", FilterTraits<MorphologicalWatershedFromMarkersImageFilter>::uuid, "cthead1-grad-mag.nrrd",
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
