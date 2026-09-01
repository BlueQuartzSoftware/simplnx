#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

namespace nx::core
{
/**
 * @struct AlignSectionsMutualInformationInputValues
 * @brief Identifies mutual-information alignment inputs.
 *
 * MisorientationTolerance is in degrees. The optional mask excludes cells.
 * StoreAlignmentShifts enables per-slice shift output.
 */
struct ORIENTATIONANALYSIS_EXPORT AlignSectionsMutualInformationInputValues
{
  DataPath ImageGeometryPath;
  bool UseMask = false;
  DataPath MaskArrayPath;

  float32 MisorientationTolerance = 0.0f;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;

  bool StoreAlignmentShifts = false;
  DataPath AlignmentAMPath;
  DataPath SlicesArrayPath;
  DataPath RelativeShiftsArrayPath;
  DataPath CumulativeShiftsArrayPath;
};

/**
 * @class AlignSectionsMutualInformation
 * @brief Aligns adjacent Z sections by maximizing mutual information.
 *
 * The algorithm segments each slice into local orientation features. It scores
 * candidate shifts from the feature-ID maps of adjacent slices. A higher score
 * indicates a better feature overlap.
 *
 * Two local feature-ID buffers limit memory to two slices. The current buffer
 * becomes the next reference buffer. This avoids rereading the prior slice and
 * avoids a full-volume feature-ID array for OOC input.
 */
class ORIENTATIONANALYSIS_EXPORT AlignSectionsMutualInformation : public AlignSections
{
public:
  /**
   * @brief Initializes mutual-information alignment.
   * @param dataStructure Provides selected arrays and the geometry.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies alignment settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  AlignSectionsMutualInformation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 AlignSectionsMutualInformationInputValues* inputValues);
  /**
   * @brief Destroys the mutual-information alignment executor.
   */
  ~AlignSectionsMutualInformation() noexcept override;

  AlignSectionsMutualInformation(const AlignSectionsMutualInformation&) = delete;
  AlignSectionsMutualInformation(AlignSectionsMutualInformation&&) noexcept = delete;
  AlignSectionsMutualInformation& operator=(const AlignSectionsMutualInformation&) = delete;
  AlignSectionsMutualInformation& operator=(AlignSectionsMutualInformation&&) noexcept = delete;

  /**
   * @brief Executes mutual-information alignment.
   * @return Result from base alignment.
   */
  Result<> operator()();

protected:
  /**
   * @brief Computes shifts from local feature-ID buffers.
   * @param xShifts Receives cumulative X shifts.
   * @param yShifts Receives cumulative Y shifts.
   * @return Success.
   *
   * Current slice bulk-I/O Result values are not inspected. Cancellation returns
   * success with completed shifts preserved.
   */
  Result<> findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts) override;

private:
  /**
   * @brief Segments one local Z slice into orientation features.
   * @param quats Provides four quaternion components per cell.
   * @param phases Provides phase IDs for each cell.
   * @param mask Provides mask values, or null when masking is disabled.
   * @param featureIds Receives one feature ID for each cell.
   * @param dimX Specifies the X dimension.
   * @param dimY Specifies the Y dimension.
   * @param misorientationTolerance Specifies the radian tolerance.
   * @param useMask Selects mask use.
   * @param orientationOps Provides symmetry operators.
   * @param crystalStructures Provides crystal structures by phase ID.
   * @pre featureIds contains dimX * dimY zero values.
   * @pre Positive phase IDs are within crystalStructures.
   * @return The next unassigned feature ID.
   *
   * The flood fill reads only local buffers and assigns four-connected cells.
   */
  int32 formFeaturesForSlice(const float32* quats, const int32* phases, const uint8* mask, std::vector<int32>& featureIds, int64 dimX, int64 dimY, float32 misorientationTolerance, bool useMask,
                             const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps, const std::vector<uint32>& crystalStructures);

  DataStructure& m_DataStructure;
  const AlignSectionsMutualInformationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
