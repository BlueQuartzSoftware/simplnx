#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

#include <vector>

namespace nx::core
{

/**
 * @struct AlignSectionsMisorientationInputValues
 * @brief Identifies misorientation-based alignment inputs.
 *
 * misorientationTolerance is in degrees. The optional mask excludes samples.
 * StoreAlignmentShifts enables output of per-slice relative and cumulative
 * shifts.
 */
struct ORIENTATIONANALYSIS_EXPORT AlignSectionsMisorientationInputValues
{
  DataPath ImageGeometryPath;
  bool UseMask = false;
  DataPath MaskArrayPath;

  float32 misorientationTolerance = 0.0f;
  DataPath quatsArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath crystalStructuresArrayPath;

  bool StoreAlignmentShifts = false;
  DataPath AlignmentAMPath;
  DataPath SlicesArrayPath;
  DataPath RelativeShiftsArrayPath;
  DataPath CumulativeShiftsArrayPath;
};

/**
 * @class AlignSectionsMisorientation
 * @brief Aligns adjacent EBSD sections by minimizing misorientation.
 *
 * The algorithm evaluates a 7-by-7 grid of X-Y offsets for each adjacent Z
 * pair. It samples every fourth cell and moves the grid until the mismatched
 * fraction reaches a minimum.
 *
 * Same-phase pairs use EbsdLib symmetry operators. Cross-phase pairs are
 * mismatched. The OOC path bulk-reads two adjacent slices and swaps their
 * local buffers after each pair. This avoids repeated random chunk reads in
 * the convergence loop.
 */
class ORIENTATIONANALYSIS_EXPORT AlignSectionsMisorientation : public AlignSections
{
public:
  /**
   * @brief Initializes misorientation-based alignment.
   * @param dataStructure Provides selected arrays and the geometry.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies alignment settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  AlignSectionsMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignSectionsMisorientationInputValues* inputValues);
  /**
   * @brief Destroys the misorientation alignment executor.
   */
  ~AlignSectionsMisorientation() noexcept override;

  AlignSectionsMisorientation(const AlignSectionsMisorientation&) = delete;
  AlignSectionsMisorientation(AlignSectionsMisorientation&&) noexcept = delete;
  AlignSectionsMisorientation& operator=(const AlignSectionsMisorientation&) = delete;
  AlignSectionsMisorientation& operator=(AlignSectionsMisorientation&&) noexcept = delete;

  /**
   * @brief Executes section alignment.
   * @return Result from alignment and bulk-I/O operations.
   */
  Result<> operator()();

protected:
  /**
   * @brief Computes shifts for adjacent Z sections.
   * @param xShifts Receives cumulative X shifts.
   * @param yShifts Receives cumulative Y shifts.
   * @return Success, or a mask or bulk-I/O error.
   *
   * The method selects local slice buffers when an input or shifted cell array
   * is out-of-core. The direct path uses array access.
   */
  Result<> findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts) override;

private:
  /**
   * @brief Computes shifts with two local Z-slice buffers.
   * @param xShifts Receives cumulative X shifts.
   * @param yShifts Receives cumulative Y shifts.
   * @return Success, or a mask or bulk-I/O error.
   *
   * The current buffer becomes the reference after each pair. This reuse avoids
   * a redundant slice read.
   */
  Result<> findShiftsOoc(std::vector<int64>& xShifts, std::vector<int64>& yShifts);

  DataStructure& m_DataStructure;
  const AlignSectionsMisorientationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
