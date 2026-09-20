#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

/**
 * @struct FillBadDataInputValues
 * @brief Collects defect classification, phase, and array settings.
 */
struct SIMPLNXCORE_EXPORT FillBadDataInputValues
{
  int32 minAllowedDefectSizeValue;
  bool storeAsNewPhase;
  DataPath featureIdsArrayPath;
  DataPath cellPhasesArrayPath;
  std::vector<DataPath> ignoredDataArrayPaths;
  DataPath inputImageGeometry;
};

/**
 * @class FillBadData
 * @brief Dispatches bad-data filling by participating array storage.
 *
 * Feature ID zero identifies a defect. Large connected defects remain zero.
 * Small defects become negative and are filled from adjacent positive features.
 * Selected sibling arrays copy the same source tuples.
 *
 * Resident execution uses BFS and volume-sized state. CCL uses sequential slice
 * passes plus temporary equivalence and fill-pair records. The dispatcher checks
 * Feature IDs, optional phases, and every cell DataArray, including ignored
 * arrays. An ignored OOC array can therefore select CCL. Storage overrides can
 * force either path.
 */
class SIMPLNXCORE_EXPORT FillBadData
{
public:
  /**
   * @brief Initializes the FillBadData dispatcher.
   * @param dataStructure Contains geometry and cell arrays.
   * @param mesgHandler Receives phase and warning messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects classification, phase, and ignored arrays.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues);
  ~FillBadData() noexcept;

  FillBadData(const FillBadData&) = delete;
  FillBadData(FillBadData&&) noexcept = delete;
  FillBadData& operator=(const FillBadData&) = delete;
  FillBadData& operator=(FillBadData&&) noexcept = delete;

  /**
   * @brief Classifies defect regions and fills small regions.
   * @return Result from the selected implementation.
   * @pre Image dimensions and selected cell-array tuple counts agree.
   * @pre The defect threshold and phase values fit their output types.
   *
   * Both implementations mutate Feature IDs before filling all sibling arrays.
   * Errors or cancellation can therefore leave partial state. See the selected
   * implementation contract for its cancellation result.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
