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
  bool use_mask{};
  uint64 interpolation_technique{};
  DataPath vertex_geom_path;
  DataPath image_geom_path;
  std::vector<DataPath> interpolated_data_paths;
  std::vector<DataPath> copy_data_paths;
  std::vector<float32> kernel_size;
  std::vector<float32> sigmas;
  DataPath mask_data_path;

  bool find_length{};
  bool find_min{};
  bool find_max{};
  bool find_mean{};
  bool find_std_deviation{};
  bool find_summation{};

  std::string length_suffix;
  std::string min_suffix;
  std::string max_suffix;
  std::string mean_suffix;
  std::string std_deviation_suffix;
  std::string summation_suffix;
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
  ~InterpolatePointCloudToRegularGrid() noexcept = default;

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
