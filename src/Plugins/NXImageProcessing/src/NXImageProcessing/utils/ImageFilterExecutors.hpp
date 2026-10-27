#pragma once

#include "NXImageProcessing/NXImageProcessing_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingOperationTypes.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <array>
#include <atomic>
#include <vector>

namespace nx::core::ImageProcessing
{
/** @brief Policy for native marker IDs that change during uint32 encoding and restoration. */
enum class MarkerIdConversionHandling : uint8
{
  Error = 0,
  WarnAndContinueLegacy = 1
};

/**
 * @brief The composite grayscale-morphology operation to apply, each built from the Dilate/Erode
 *        primitives (and, for the three difference forms, a single elementwise store subtraction):
 *          - Opening     = Dilate(Erode(in))            (removes bright specks smaller than the SE)
 *          - Closing     = Erode(Dilate(in))            (fills dark specks/holes smaller than the SE)
 *          - WhiteTopHat = in - Opening(in)             (the bright features the opening removed)
 *          - BlackTopHat = Closing(in) - in             (the dark features the closing filled)
 *          - Gradient    = Dilate(in) - Erode(in)       (the local intensity range == edge strength)
 */
enum class MorphCompositeOp
{
  Opening,
  Closing,
  WhiteTopHat,
  BlackTopHat,
  Gradient
};

/**
 * @brief The composite BINARY-morphology operation to apply, each built from the binary Dilate/Erode
 *        primitives (@ref ApplyBinaryMorphology):
 *          - Opening = binaryErode(boundaryToForeground=true) -> binaryDilate(boundaryToForeground=false)
 *                      (removes foreground specks/protrusions smaller than the structuring element). No padding.
 *          - Closing = binaryDilate(boundaryToForeground=false) -> binaryErode(boundaryToForeground=true)
 *                      (fills background holes/gaps smaller than the structuring element). Optional SafeBorder
 *                      pad/crop by the kernel radius.
 * These mirror the legacy ITK Binary Morphological Opening/Closing composites. On the strictly-{fg, bg} input
 * the binary morphology façade requires, ITK's restore-original-where-not-foreground post-pass is a proven
 * no-op, so it is intentionally omitted.
 */
enum class BinaryCompositeOp
{
  Opening,
  Closing
};

namespace detail
{
/**
 * @brief Which marker image a grayscale reconstruction filter derives from its single input, matching the ITK
 *        consumer it replaces:
 *          - FillholeMaxBorder: the input's global maximum in the interior and the input value on the effective-
 *            dimensionality border (ITK GrayscaleFillholeImageFilter marker); reconstruct-by-erosion.
 *          - GrindPeakMinBorder: the input's global minimum in the interior and the input value on the effective-
 *            dimensionality border (ITK GrayscaleGrindPeakImageFilter marker); reconstruct-by-dilation. Dual of
 *            FillholeMaxBorder (min interior + dilation instead of max interior + erosion).
 *          - MinusHeight: saturating (input - height) (ITK HMaximaImageFilter marker); reconstruct-by-dilation.
 *          - PlusHeight:  saturating (input + height) (ITK HMinimaImageFilter marker); reconstruct-by-erosion.
 */
enum class ReconMarker
{
  FillholeMaxBorder,
  GrindPeakMinBorder,
  MinusHeight,
  PlusHeight
};

} // namespace detail

/**
 * @brief Execute for a two-input mask filter: output[t] = (mask[t] != 0) ? input[t] : outsideValue,
 *        applied to all components of each tuple. Output type == input type. Single-implementation.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteMaskImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& maskArrayPath, const DataPath& outputArrayPath,
                                                         float64 outsideValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute for the Adaptive Histogram Equalization filter (Stark 2000). Reads the input dims from the
 *        ImageGeom. Dispatches over the 10 numeric scalar types in @c AllNumeric. Resolves the in/out stores
 *        and runs the engine (@ref ApplyAdaptiveHistogramEqualization).
 *        Output type == input type (SameAsInput). Single-implementation: identical code path in-core and OOC,
 *        so no DispatchAlgorithm and no in/out IDataArray refs are needed. @p radius is the per-axis box-window
 *        radius; @p alpha / @p beta are the Stark parameters (no range validation -- ITK imposes none).
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteAdaptiveHistogramEqualizationImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                                  const DataPath& outputArrayPath, const std::array<usize, 3>& radius, float32 alpha, float32 beta,
                                                                                  const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute for a flat-structuring-element grayscale morphology filter (Dilate/Erode). Reads the input
 *        dims from the ImageGeom. Dispatches over the numeric types in @c AllNumeric. Resolves the in/out stores
 *        and runs the morphology engine (@ref ApplyMorphology).
 *        The engine internally routes the in-core moving-histogram Direct path vs the out-of-core Scanline path
 *        via DispatchAlgorithm. Output type == input type (SameAsInput). @p se is the rasterized structuring
 *        element (built once by the caller from the kernel type + radius); @p op selects Dilate (max fold)
 *        or Erode (min fold). Single façade shared by every morphology filter (Task 6 composites reuse it).
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                               const StructuringElement& se, MorphOp op, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute for a flat-structuring-element OBJECT morphology filter (Dilate/Erode Object Morphology).
 *        Object morphology is a boundary SCATTER, distinct from the grayscale/binary SE min/max gather: only
 *        the boundary voxels of the object paint the structuring element around themselves into a separate
 *        output (see @ref ApplyObjectMorphology). Reads the input dims from the ImageGeom, builds the fixed
 *        radius-1 full-box boundary neighborhood once. Dispatches over the 10 numeric scalar types in
 *        @c AllNumeric. Resolves the in/out stores and runs the engine.
 *        The engine routes the ITK-faithful in-core Scatter path vs the OOC streamed Gather path via
 *        DispatchAlgorithm. Output type == input type (SameAsInput).
 *
 * @p se is the paint structuring element (built once by the caller from the kernel type + radius); @p op selects
 * Dilate (paint the object value) or Erode (paint the background value). @p objectValue / @p backgroundValue are
 * the Float64 parameter values cast to the element type inside the dispatched functor; the caller MUST have
 * validated them against the element type at preflight (see @ref ValidateScalarValueForType) so the narrowing
 * conversion is well-defined. Single façade shared by both object-morphology filters.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteObjectMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                     const StructuringElement& se, ObjectMorphOp op, float64 objectValue, float64 backgroundValue, const std::atomic_bool& shouldCancel,
                                                                     const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute for a flat-structuring-element BINARY morphology filter (Binary Dilate/Erode). The binary
 *        analogue of @ref ExecuteMorphologyImageFilter: reads the input dims from the ImageGeom.
 *        Dispatches over the integer scalar types in @c IntegerOnly. Binary morphology is defined on integer
 *        label images. Resolves the in/out stores and runs the binary morphology engine
 *        (@ref ApplyBinaryMorphology). The engine internally routes the in-core moving fg-count Direct path vs
 *        the out-of-core Scanline path via DispatchAlgorithm. Output type == input type (SameAsInput).
 *
 * @p se is the rasterized structuring element (built once by the caller from the kernel type + radius); @p op
 * selects Dilate (any foreground neighbor -> foreground) or Erode (all foreground neighbors -> foreground).
 * Membership is equality-based: a voxel is foreground iff it equals @p foreground, so an arbitrary-label input
 * is handled correctly and the output is strictly {foreground, background}. @p boundaryToForeground controls
 * whether an out-of-image neighbor counts as foreground. @p foreground / @p background are passed as Float64
 * (the parameter type) and cast to the integer element type inside the dispatched functor; the caller MUST
 * have validated them against the element type's range at preflight (see the Binary Dilate/Erode filters'
 * fg/bg guard) so the narrowing conversion is well-defined. Single façade shared by both binary morphology
 * filters (and reusable by later binary composites).
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteBinaryMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                     const StructuringElement& se, MorphOp op, float64 foreground, float64 background, bool boundaryToForeground,
                                                                     const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/// @brief Executes a composite grayscale-morphology filter.
/// @details Dispatches over the numeric types in @c AllNumeric.
/// The output element type equals the input element type.
/// Each pass selects the in-core direct engine or OOC scanline engine.
/// Scratch stores inherit the input format, so OOC passes remain streamed.
/// Without SafeBorder, composites use one original-sized scratch store.
/// With SafeBorder, composites use two padded scratch stores.
/// Padding uses @ref SafeBorderPadRadius; cropping follows both passes.
/// Thus peak scratch is one original-sized store or two padded stores.
/// Morphological gradient ignores @p safeBorder and uses no scratch store.
/// Unpadded top hats finalize with @ref SubtractStores.
/// SafeBorder top hats fuse crop and subtraction with @ref CropSubtractFromStore.
/// SafeBorder opening and closing only crop the final padded result.
NXIMAGEPROCESSING_EXPORT Result<> ExecuteMorphologyCompositeImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                        const StructuringElement& se, MorphCompositeOp op, bool safeBorder, const std::atomic_bool& shouldCancel,
                                                                        const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute for a COMPOSITE BINARY-morphology filter (Binary Morphological Opening/Closing). The binary
 *        analogue of @ref ExecuteMorphologyCompositeImageFilter: reads the input dims from the ImageGeom.
 *        Dispatches over the integer scalar types in @c IntegerOnly. Binary morphology is defined on integer
 *        label images. Runs the binary-input safeguard EXACTLY ONCE on the original input.
 *        Then composes the two-pass sequence for @p op through @ref ApplyBinaryMorphology.
 *        The engine routes the in-core moving fg-count Direct path vs the out-of-core Scanline path per pass.
 *        Output type == input type (SameAsInput). @p se is the rasterized structuring element (built once by
 *        the caller). Scratch stores are allocated per run inheriting the active OOC endpoint format, so an OOC
 *        endpoint keeps the composite
 *        bounded-memory and streamed: Opening and the plain (SafeBorder == false) Closing use ONE same-extent
 *        scratch, while Closing's SafeBorder == true path allocates TWO ping-pong scratches sized to the volume
 *        padded by the kernel radius.
 *
 * @p foreground / @p background are the Float64 parameter values (the caller MUST have range-validated them at
 * preflight so the narrowing cast to the integer element type is well-defined). @p background is used only by
 * Opening; Closing derives its own internal background (0, or the type max iff foreground is 0). @p safeBorder
 * selects Closing's boundary convention (ignored by Opening, which never pads): true (the ITK default) pads by
 * the kernel radius with the internal background, runs both passes on the padded volume, and crops back --
 * bit-identical to the legacy ITK filter; false runs the plain two-pass composition on the original extent.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteBinaryMorphologyCompositeImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                              const DataPath& outputArrayPath, const StructuringElement& se, BinaryCompositeOp op, float64 foreground,
                                                                              float64 background, bool safeBorder, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute a grayscale morphological reconstruction filter (GrayscaleFillhole / GrayscaleGrindPeak / HMaxima
 *        / HMinima / HConvex). Reads the volume dims from @p imageGeomPath, derives the marker selected by
 *        @p markerKind from the single input array, and reconstructs it under the input (mask) via the D3-split
 *        engine (in-core Vincent hybrid vs streamed out-of-core sweep, dispatched on the input/output storage
 *        mode). @p op / @p fullyConnected pick direction + connectivity; @p height feeds the HMaxima/HMinima
 *        markers (ignored by the border markers). When @p differenceFromInput is true (HConvex: the h-dome), the
 *        reconstruction result is subtracted from the input in place after reconstruction (output = input -
 *        reconstruction). Output type == input type (SameAsInput). The caller's preflight must have enforced a
 *        scalar input (@ref PreflightImageFilter requireScalar=true) and, for the height markers, a finite
 *        @p height. The opt-in @p exactIntegerHeight also requires a nonnegative whole Height for integer images;
 *        it preserves native integer units and saturates markers at the type bounds. Other callers default to their existing arithmetic.
 * @param dataStructure Owns the selected arrays and geometry.
 * @param imageGeomPath Image geometry used for dimensions.
 * @param inputArrayPath Scalar input image and reconstruction mask.
 * @param outputArrayPath Receives reconstruction or contrast in the input type.
 * @param op Selects reconstruction by dilation or erosion.
 * @param fullyConnected Enables face, edge and vertex neighbors instead of faces only.
 * @param markerKind Selects a height or border marker.
 * @param height Validated finite height in intensity units.
 * @param shouldCancel Requests cancellation during reconstruction and between transfers.
 * @param messageHandler Receives progress messages.
 * @param differenceFromInput Selects HConvex contrast with checked signed subtraction.
 * @param exactIntegerHeight Selects exact native integer markers for a validated whole, nonnegative Height.
 * @return A reconstruction, contrast or transfer error, or a valid result.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteMorphologicalReconstructionImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                                const DataPath& outputArrayPath, ReconstructOp op, bool fullyConnected, detail::ReconMarker markerKind, float64 height,
                                                                                const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, bool differenceFromInput = false,
                                                                                bool exactIntegerHeight = false);

/**
 * @brief Execute opening-by-reconstruction (@p firstOp == Erode) or closing-by-reconstruction (@p firstOp ==
 *        Dilate): a grayscale morphological erode/dilate with @p se followed by a geodesic reconstruction of that
 *        result under the input, optionally re-valued to preserve original intensities (@p preserveIntensities).
 *        Output type == input type (SameAsInput). The caller's preflight must have enforced a scalar input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteReconstructionOpenCloseImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                            const DataPath& outputArrayPath, const StructuringElement& se, MorphOp firstOp, bool fullyConnected,
                                                                            bool preserveIntensities, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute binary opening by reconstruction (integer element types): a binary erosion with @p se followed by
 *        a binary reconstruction-by-dilation retaining the foreground (@p foreground) components of the input that
 *        survive the erosion; all else @p background. Output type == input type. The caller's preflight must have
 *        enforced a scalar integer input and a valid fg/bg range.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteBinaryOpeningByReconstructionImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                                  const DataPath& outputArrayPath, const StructuringElement& se, float64 foreground, float64 background,
                                                                                  bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute a valued regional-extrema filter (ValuedRegionalMaxima if @p op == Maxima, ValuedRegionalMinima if
 *        Minima): pixels belonging to a regional extremum keep their value; every other pixel is set to the marker
 *        value (type lowest for maxima, type max for minima). Routes the in-core flood vs the out-of-core sweep via
 *        the D3-split engine. Output type == input type. The caller's preflight must have enforced a scalar input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteValuedRegionalExtremaImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                          RegionalExtremaOp op, bool fullyConnected, const std::atomic_bool& shouldCancel,
                                                                          const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute a binary-output regional-extrema filter (RegionalMaxima if @p op == Maxima, RegionalMinima if
 *        Minima): the regional extrema become @p foreground and everything else @p background. A flat image becomes
 *        all foreground (if @p flatIsExtremum) or all background. The output is a fixed uint32 label image (matching
 *        the legacy ITK FilterOutputType), not the input element type. The caller's preflight must have enforced a
 *        scalar input and a valid @p foreground / @p background for the element type.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteRegionalExtremaImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                    RegionalExtremaOp op, bool fullyConnected, float64 foreground, float64 background, bool flatIsExtremum,
                                                                    const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the Signed Maurer Distance Map filter: computes the exact signed Euclidean (or squared-Euclidean)
 *        distance transform of the object defined by the input image, where a voxel is "inside" the object iff its
 *        value is not @p backgroundValue. Distances are negative inside and positive outside the object (or the
 *        reverse when @p insideIsPositive), optionally scaled by the voxel spacing (@p useSpacing) and optionally
 *        left as squared distances (@p squaredDistance). The output is a FIXED float32 distance image (matching the
 *        legacy ITK FilterOutputType), not the input element type. The caller's preflight must have enforced a
 *        scalar integer input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteSignedMaurerDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                            const DataPath& outputArrayPath, bool insideIsPositive, bool squaredDistance, bool useSpacing, float64 backgroundValue,
                                                                            const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the (unsigned) Danielsson Distance Map filter: computes the Danielsson 4SED approximate Euclidean (or
 *        squared-Euclidean) distance from each background (zero) voxel to the nearest foreground (nonzero) voxel,
 *        optionally scaled by the voxel spacing (@p useSpacing). The output is a FIXED float32 distance image
 *        (matching the legacy ITK FilterOutputType), not the input element type. @p inputIsBinary is carried for
 *        interface/SIMPL parity but does not affect the distance output. The caller's preflight must have enforced a
 *        scalar integer input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteDanielssonDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                          bool inputIsBinary, bool squaredDistance, bool useSpacing, const std::atomic_bool& shouldCancel,
                                                                          const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the Signed Danielsson Distance Map filter: the ITK composite of two Danielsson passes (on the input
 *        and on the dilated inverted input) subtracted to produce a SIGNED distance -- negative inside the object and
 *        positive outside (or the reverse when @p insideIsPositive), optionally squared (@p squaredDistance) and
 *        optionally spacing-weighted (@p useSpacing). The output is a FIXED float32 distance image. The caller's
 *        preflight must have enforced a scalar integer input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteSignedDanielssonDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                                const DataPath& outputArrayPath, bool insideIsPositive, bool squaredDistance, bool useSpacing,
                                                                                const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the IsoContourDistance filter: a narrow-band SIGNED distance around the @p levelSetValue iso-contour
 *        of the input, with far voxels set to +-@p farValue. The output is a FIXED float32 image, not the input
 *        element type. Input may be ANY scalar type (integer OR floating point). The caller's preflight must have
 *        enforced a scalar input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteIsoContourDistanceImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                       float64 levelSetValue, float64 farValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the GradientMagnitudeRecursiveGaussian filter: sqrt(sum over filtered axes of (d/dx_i of the
 *        Gaussian-smoothed image)^2), computed with the separable Deriche recursive-Gaussian engine. Sigma is a single
 *        scalar shared by every axis. The output is a FIXED float32 image, not the input element type. Input may be ANY
 *        scalar type (integer OR floating point). The caller's preflight must have enforced a scalar input and the
 *        >= 4-pixels-per-axis dimension requirement.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteGradientMagnitudeRecursiveGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                                       const DataPath& outputArrayPath, float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel,
                                                                                       const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the (ITK-free) GradientMagnitude filter: a single-pass central-difference (order-1, [0.5, 0, -0.5])
 *        gradient magnitude, computed by the streaming @ref ApplyGradientMagnitude engine rather than the separable
 *        recursive-Gaussian engine used by @ref ExecuteGradientMagnitudeRecursiveGaussianImageFilter above. When @p
 *        useImageSpacing is set, each axis's tap is scaled by 1/spacing[axis] (physical-space derivative); otherwise
 *        the derivative is taken in isotropic voxel space. The output is a FIXED float32 image, not the input element
 *        type. Input may be ANY scalar type (integer OR floating point). The caller's preflight must have enforced a
 *        scalar input; there is no minimum-dimension requirement (unlike the recursive-Gaussian family).
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteGradientMagnitudeImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                      bool useImageSpacing, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the LaplacianRecursiveGaussian filter: the sum over filtered axes of the second derivative
 *        (d^2/dx_i^2 of the Gaussian-smoothed image), computed with the separable Deriche recursive-Gaussian engine.
 *        Sigma is a single scalar shared by every axis. The output is a FIXED float32 image, not the input element
 *        type. Input may be ANY scalar type (integer OR floating point). Unlike GradientMagnitude there is no squaring
 *        and no final sqrt. The caller's preflight must have enforced a scalar input and the >= 4-pixels-per-axis
 *        dimension requirement.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteLaplacianRecursiveGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                               const DataPath& outputArrayPath, float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel,
                                                                               const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the SmoothingRecursiveGaussian filter: a separable Gaussian smoothing (per-axis sigma) computed with
 *        the Deriche recursive-Gaussian engine. The output has the SAME type as the input (type-preserving), so this
 *        uses the SignedScalar policy (signed integer + floating point; unsigned excluded because the Deriche IIR has a
 *        small negative overshoot that would underflow an unsigned round-trip). The caller's preflight must have
 *        enforced a scalar input and the >= 4-pixels-per-axis dimension requirement.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteSmoothingRecursiveGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                               const DataPath& outputArrayPath, const std::vector<float64>& sigma, bool normalizeAcrossScale,
                                                                               const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the (ITK-free) DiscreteGaussian filter: a separable FIR discrete-Gaussian convolution (verbatim
 *        GaussianOperator modified-Bessel kernel per axis, descending axis order, ZeroFluxNeumann boundary), computed
 *        by the streaming @ref ApplyDiscreteGaussian engine. The output has the SAME type as the input, so intermediate
 *        passes are T-typed and integer types truncate between passes (matching ITK's actual behavior). When @p
 *        useImageSpacing is set, each axis's variance is divided by spacing[axis]^2 before the kernel is built. Input
 *        may be ANY scalar type (integer OR floating point); the caller's preflight must have enforced a scalar input.
 *        There is no minimum-dimension requirement.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteDiscreteGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                     const std::vector<float64>& variance, uint32 maximumKernelWidth, const std::vector<float64>& maximumError, bool useImageSpacing,
                                                                     const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the ApproximateSignedDistanceMap filter: the ITK composite (IsoContourDistance with
 *        levelSet=(InsideValue+OutsideValue)/2 then FastChamferDistance) producing an approximate SIGNED distance to
 *        the boundary between the InsideValue and OutsideValue regions of an integer mask. Output is a FIXED float32
 *        image. The caller's preflight must have enforced a scalar integer input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteApproximateSignedDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                                 const DataPath& outputArrayPath, float64 insideValue, float64 outsideValue, const std::atomic_bool& shouldCancel,
                                                                                 const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the (ITK-free) ConnectedComponent filter: labels foreground (non-zero) voxels of an integer scalar
 *        image into consecutive uint32 components (background 0), via the streaming scanline union-find @ref
 *        LabelConnectedComponents engine. The output is a FIXED uint32 label image, not the input element type.
 *        Below the provisional uint32 capacity limit, preserves the legacy label values. Only this filter exposes
 *        legacy overflow continuation; shared consumers retain Error. The caller must enforce an integer scalar input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteConnectedComponentImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                       bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler,
                                                                       ConnectedComponentLabelLimitHandling labelLimitHandling = ConnectedComponentLabelLimitHandling::Error);

/**
 * @brief Execute the (ITK-free) ZeroCrossing filter: marks the voxels closest to sign changes (zero crossings) among
 *        their axial face neighbors with @p foregroundValue and all others with @p backgroundValue, via the streamed
 *        axial sign-change stencil @ref ApplyZeroCrossing engine. The output is a FIXED uint8 label image, not the
 *        input element type. Reproduces legacy ITK's ZeroCrossingImageFilter output VALUES exactly (see the Parity
 *        model). The caller's preflight must have enforced a signed scalar input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteZeroCrossingImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                 uint8 foregroundValue, uint8 backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the (ITK-free) DoubleThreshold filter: thresholds the input into a narrow marker band [T2,T3] and
 *        a wide mask band [T1,T4] (two streamed BinaryThresholds into uint8 scratch), then reconstructs the marker
 *        under the mask by dilation to convergence (@ref ApplyMorphologicalReconstruction). The output is a FIXED
 *        uint8 label image, not the input element type. Reproduces legacy ITK's DoubleThresholdImageFilter output
 *        VALUES exactly (see the Parity model): the thresholds are cast to the input element type T before the band
 *        test, matching ITK's InputPixelType-typed thresholds. The caller's preflight must have enforced a scalar
 *        input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteDoubleThresholdImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                    float64 threshold1, float64 threshold2, float64 threshold3, float64 threshold4, uint8 insideValue, uint8 outsideValue,
                                                                    bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the (ITK-free) RelabelComponent filter: given an ALREADY-LABELED integer scalar image, computes
 *        each label's pixel count, optionally sorts by size (descending, ties by ascending original label -- see
 *        @ref detail::RelabelComponentExecuteFn), drops components smaller than @p minimumObjectSize (mapped to
 *        background), and reassigns consecutive labels 1,2,... in that order. Output is SameAsInput -- this does
 *        NOT run the ConnectedComponent union-find engine. Reproduces legacy ITK's RelabelComponentImageFilter
 *        label VALUES exactly (see the Parity model). The caller's preflight must have enforced an integer scalar
 *        input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteRelabelComponentImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, uint64 minimumObjectSize,
                                                                     bool sortByObjectSize, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the (ITK-free) BinaryThinning filter: thins the foreground (non-zero) voxels of an integer scalar
 *        image to a 1-pixel-wide skeleton (values 0/1) via the per-z-slice 2D sequential thinning @ref
 *        ApplyBinaryThinning engine. A 3D image is thinned per-z-slice independently (ITK's neighbor offsets are 2D),
 *        so the output is SameAsInput (same type as input). Reproduces legacy ITK's BinaryThinningImageFilter output
 *        VALUES exactly (see the Parity model). The caller's preflight must have enforced an integer scalar input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteBinaryThinningImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                   const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);

/**
 * @brief Execute the native ThresholdMaximumConnectedComponents heuristic using component scores (size >= MinimumObjectSizeInPixels) via @ref
 *        detail::ThresholdMaxCCExecuteFn (which uses @ref CountConnectedComponents each iteration), then writes the
 *        final binary threshold [ThresholdValue, UpperBoundary] as a FIXED uint8 image (InsideValue/OutsideValue).
 *        Uses native safe arithmetic, the absolute two-unit stop and an explicit image nonfinite policy.
 *        The caller's preflight must have enforced a scalar input.
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteThresholdMaximumConnectedComponentsImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                                        const DataPath& outputArrayPath, uint32 minimumObjectSizeInPixels, float64 upperBoundary, uint8 insideValue,
                                                                                        uint8 outsideValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler,
                                                                                        bool warnOnNonfinite = false);

/**
 * @brief Execute the (ITK-free, out-of-core) MorphologicalWatershedFromMarkers filter: a marker-controlled
 *        hierarchical-queue (FAH) watershed flood (see @ref ApplyWatershedFromMarkers). Dispatches on the grayscale
 *        input element type (all 10 scalar types); the marker array is
 *        any integer type and the output is created of the marker's type by preflight. Error protects native marker
 *        identity before flooding; Warn retains the legacy conversion and flood with one conversion diagnostic.
 *        The caller's preflight must have validated the grayscale (scalar,
 *        allowed type, tuple==cells) and the marker (integer scalar, tuple==grayscale).
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteMorphologicalWatershedFromMarkersImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath,
                                                                                      const DataPath& markerArrayPath, const DataPath& outputArrayPath, bool markWatershedLine, bool fullyConnected,
                                                                                      const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler,
                                                                                      MarkerIdConversionHandling conversionHandling = MarkerIdConversionHandling::Error);

/**
 * @brief Execute the (ITK-free, out-of-core) MorphologicalWatershed filter: the ITK composite (HMinima ->
 *        RegionalMinima -> ConnectedComponent -> marker-controlled FAH watershed flood; see @ref
 *        MorphologicalWatershedExecuteFn), reproducing legacy ITK's MorphologicalWatershedImageFilter output EXACTLY
 *        (label values included; see the Parity model and the live-ITK test).
 *        Dispatches the grayscale input over the 10 numeric scalar types in @c AllNumeric. The output is a FIXED
 *        uint32 label image created by the filter's preflight (AlwaysUInt32). The caller's preflight must have
 *        validated the input (scalar, allowed type, tuple==cells).
 */
NXIMAGEPROCESSING_EXPORT Result<> ExecuteMorphologicalWatershedImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                           float64 level, bool markWatershedLine, bool fullyConnected, const std::atomic_bool& shouldCancel,
                                                                           const IFilter::MessageHandler& messageHandler);
} // namespace nx::core::ImageProcessing
