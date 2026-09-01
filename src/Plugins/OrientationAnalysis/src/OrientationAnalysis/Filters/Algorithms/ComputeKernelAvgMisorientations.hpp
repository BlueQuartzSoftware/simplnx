#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <chrono>

namespace nx::core
{

/**
 * @struct ComputeKernelAvgMisorientationsInputValues
 * @brief Identifies KAM inputs.
 *
 * KernelSize stores X, Y, Z radii. The output angle is in degrees.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeKernelAvgMisorientationsInputValues
{
  VectorInt32Parameter::ValueType KernelSize;
  bool UseFeatureIds = true;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath KernelAverageMisorientationsArrayName;
  DataPath InputImageGeometry;
};

/**
 * @class ComputeKernelAvgMisorientations
 * @brief Computes Kernel Average Misorientation for each Image Geometry cell.
 *
 * Valid cells average admitted neighbor angles in degrees. Same-feature mode
 * uses the focal feature. Same-phase mode uses positive feature IDs in the
 * focal phase.
 *
 * The facade dispatches to direct or cache-budgeted scanline traversal.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeKernelAvgMisorientations
{
public:
  /**
   * @brief Initializes KAM dispatch.
   * @param dataStructure Provides selected arrays and the geometry.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and KAM settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  ComputeKernelAvgMisorientationsInputValues* inputValues);
  /**
   * @brief Destroys the KAM dispatcher.
   */
  ~ComputeKernelAvgMisorientations() noexcept;

  ComputeKernelAvgMisorientations(const ComputeKernelAvgMisorientations&) = delete;
  ComputeKernelAvgMisorientations(ComputeKernelAvgMisorientations&&) noexcept = delete;
  ComputeKernelAvgMisorientations& operator=(const ComputeKernelAvgMisorientations&) = delete;
  ComputeKernelAvgMisorientations& operator=(ComputeKernelAvgMisorientations&&) noexcept = delete;

  /**
   * @brief Dispatches KAM computation.
   * @return Result from the selected executor.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
