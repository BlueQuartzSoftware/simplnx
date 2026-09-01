#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

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
  Result<> operator()();

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
