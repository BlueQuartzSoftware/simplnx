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
struct ORIENTATIONANALYSIS_EXPORT AlignSectionsMutualInformationInputValues
{
  DataPath ImageGeometryPath;
  bool UseMask;
  DataPath MaskArrayPath;

  float32 MisorientationTolerance;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;

  bool StoreAlignmentShifts;
  DataPath AlignmentAMPath;
  DataPath SlicesArrayPath;
  DataPath RelativeShiftsArrayPath;
  DataPath CumulativeShiftsArrayPath;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT AlignSectionsMutualInformation : public AlignSections
{
public:
  AlignSectionsMutualInformation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 AlignSectionsMutualInformationInputValues* inputValues);
  ~AlignSectionsMutualInformation() noexcept override;

  AlignSectionsMutualInformation(const AlignSectionsMutualInformation&) = delete;
  AlignSectionsMutualInformation(AlignSectionsMutualInformation&&) noexcept = delete;
  AlignSectionsMutualInformation& operator=(const AlignSectionsMutualInformation&) = delete;
  AlignSectionsMutualInformation& operator=(AlignSectionsMutualInformation&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts) override;

private:
  /**
   * @brief Flood-fills a single Z-slice to identify features based on
   * misorientation tolerance. Uses local indices (0 to sliceVoxels-1).
   * Works for both in-core and OOC paths since the caller provides
   * pre-buffered slice data.
   * @param quats Pointer to the slice's quaternion data (4 components per voxel).
   * @param phases Pointer to the slice's phase data.
   * @param mask Pointer to the slice's mask data (nullptr if mask is not used).
   * @param featureIds Output vector of per-voxel feature IDs (must be pre-zeroed, size = dimX*dimY).
   * @param dimX Number of voxels in X dimension.
   * @param dimY Number of voxels in Y dimension.
   * @param misorientationTolerance Misorientation tolerance in radians.
   * @param useMask Whether to use the mask array.
   * @param orientationOps Laue orientation operators.
   * @param crystalStructures Crystal structure IDs indexed by phase.
   * @return The feature count for this slice.
   */
  int32 formFeaturesForSlice(const float32* quats, const int32* phases, const uint8* mask, std::vector<int32>& featureIds, int64 dimX, int64 dimY, float32 misorientationTolerance, bool useMask,
                             const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps, const std::vector<uint32>& crystalStructures);

  DataStructure& m_DataStructure;
  const AlignSectionsMutualInformationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
