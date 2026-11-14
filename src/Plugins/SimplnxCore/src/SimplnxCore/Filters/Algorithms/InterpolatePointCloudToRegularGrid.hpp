#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <array>

namespace nx::core
{

/**
 * @struct InterpolatePointCloudToRegularGridInputValues
 * @brief Stores interpolation paths and options.
 */
struct SIMPLNXCORE_EXPORT InterpolatePointCloudToRegularGridInputValues
{
  bool useMask;
  uint64 interpolationTechnique;
  DataPath vertexGeomPath;
  DataPath imageGeomPath;
  std::vector<DataPath> interpolatedDataPaths;
  std::vector<DataPath> copyDataPaths;
  std::vector<float32> kernelSize;
  std::vector<float32> sigmas;
  DataPath maskDataPath;

  bool findLength;
  bool findMin;
  bool findMax;
  bool findMean;
  bool findStdDeviation;
  bool findSummation;

  std::string lengthSuffix;
  std::string minSuffix;
  std::string maxSuffix;
  std::string meanSuffix;
  std::string stdDeviationSuffix;
  std::string summationSuffix;
};

/**
 * @struct InterpolationKernelLayout
 * @brief Stores checked kernel radii, dimensions and element count without allocating weights.
 */
struct InterpolationKernelLayout
{
  std::array<int64, 3> Radii = {0, 0, 0};
  SizeVec3 Dimensions = {1, 1, 1};
  usize ElementCount = 1;
};

/**
 * @class InterpolatePointCloudToRegularGrid
 * @brief Accumulates point values onto a regular ImageGeom using a uniform or
 * Gaussian kernel and optionally emits per-voxel statistics.
 *
 * Voxel-scale accumulator arrays use temporary record stores with bounded page
 * caches whenever output dispatch is out-of-core. This avoids multiplying the
 * resident footprint by the number of requested source arrays and statistics.
 * Selected numeric source arrays are still materialized as float64 vectors.
 * Boolean selected arrays are ignored.
 */
class SIMPLNXCORE_EXPORT InterpolatePointCloudToRegularGrid
{
public:
  /**
   * @brief Creates a point-cloud interpolation algorithm.
   * @param dataStructure Provides selected geometries and arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later vertices or output pages when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  InterpolatePointCloudToRegularGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     InterpolatePointCloudToRegularGridInputValues* inputValues);
  /**
   * @brief Destroys the non-owning interpolation algorithm.
   */
  ~InterpolatePointCloudToRegularGrid() noexcept;

  InterpolatePointCloudToRegularGrid(const InterpolatePointCloudToRegularGrid&) = delete;
  InterpolatePointCloudToRegularGrid(InterpolatePointCloudToRegularGrid&&) noexcept = delete;
  InterpolatePointCloudToRegularGrid& operator=(const InterpolatePointCloudToRegularGrid&) = delete;
  InterpolatePointCloudToRegularGrid& operator=(InterpolatePointCloudToRegularGrid&&) noexcept = delete;

  /**
   * @brief Interpolates selected point arrays to image cells.
   * @return Error from temporary storage or output writes, or success after cancellation.
   *
   * Cancellation can retain outputs written before the current output page.
   */
  /**
   * @brief Accumulates point values in image cells.
   * @pre Kernel Size has three finite nonnegative components; active Gaussian Sigmas have finite positive components and finite nonzero float32 squares.
   * @return Temporary-storage or output-write errors, otherwise success.
   */
  Result<> operator()();

  /**
   * @brief Identifies an unrepresentable kernel radius, width or consumed spacing.
   */
  static constexpr int32 k_KernelRadiusRepresentation = -86165;
  /**
   * @brief Identifies overflow in the kernel element product.
   */
  static constexpr int32 k_KernelProductRepresentation = -86166;
  /**
   * @brief Identifies an unrepresentable kernel byte count or container size.
   */
  static constexpr int32 k_KernelStorageRepresentation = -86167;

  /**
   * @brief Computes representable kernel extents from final image spacing.
   * @param image Provides physical cell spacing, possibly provisional during preflight.
   * @param inputValues Provides finite nonnegative physical kernel extents and the image path.
   * @pre kernelSize has three finite nonnegative components.
   * @return The layout, or a radius, product, byte-count or container-size error.
   */
  static Result<InterpolationKernelLayout> ComputeKernelLayout(const ImageGeom& image, const InterpolatePointCloudToRegularGridInputValues& inputValues);

  /**
   * @brief Squares a signed kernel offset without signed integer overflow.
   * @param offset Gives a displacement in cells.
   * @return The squared displacement in the existing float32 numerator representation.
   */
  static float32 ComputeSquaredKernelOffset(int64 offset);

  const std::atomic_bool& getCancel();

  /**
   * @brief Selects equal kernel weights.
   */
  static constexpr uint64 k_Uniform = 0;
  /**
   * @brief Selects Gaussian kernel weights.
   */
  static constexpr uint64 k_Gaussian = 1;

private:
  DataStructure& m_DataStructure;
  const InterpolatePointCloudToRegularGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
