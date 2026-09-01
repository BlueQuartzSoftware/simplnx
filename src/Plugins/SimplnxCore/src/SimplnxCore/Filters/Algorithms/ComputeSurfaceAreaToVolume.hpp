#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeSurfaceAreaToVolumeInputValues
 * @brief Stores validated paths and the optional sphericity selection.
 */
struct SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolumeInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath NumCellsArrayPath;
  DataPath SurfaceAreaVolumeRatioArrayName;
  bool CalculateSphericity;
  DataPath SphericityArrayName;
  DataPath InputImageGeometry;
};

/**
 * @class ComputeSurfaceAreaToVolume
 * @brief Dispatches surface metrics by Feature Id storage.
 *
 * The cell-scale Feature Id array drives dispatch because six-neighbor reads can
 * thrash disk chunks. Feature-level arrays do not affect selection.
 *
 * @see ComputeSurfaceAreaToVolumeDirect, ComputeSurfaceAreaToVolumeScanline, DispatchAlgorithm
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolume
{
public:
  /**
   * @brief Creates a surface-metric dispatcher.
   * @param dataStructure Provides the selected arrays and geometry.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later slices when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the dispatcher lifetime.
   */
  ComputeSurfaceAreaToVolume(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeSurfaceAreaToVolumeInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~ComputeSurfaceAreaToVolume() noexcept;

  ComputeSurfaceAreaToVolume(const ComputeSurfaceAreaToVolume&) = delete;
  ComputeSurfaceAreaToVolume(ComputeSurfaceAreaToVolume&&) noexcept = delete;
  ComputeSurfaceAreaToVolume& operator=(const ComputeSurfaceAreaToVolume&) = delete;
  ComputeSurfaceAreaToVolume& operator=(ComputeSurfaceAreaToVolume&&) noexcept = delete;

  /**
   * @brief Dispatches the surface-metric calculation.
   * @return Error from the selected implementation.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceAreaToVolumeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
