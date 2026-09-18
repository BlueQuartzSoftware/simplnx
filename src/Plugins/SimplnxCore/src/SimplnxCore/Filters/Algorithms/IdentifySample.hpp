#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct IdentifySampleInputValues
 * @brief Collects sample, hole-filling, and slice settings.
 */
struct SIMPLNXCORE_EXPORT IdentifySampleInputValues
{
  BoolParameter::ValueType FillHoles;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  ArraySelectionParameter::ValueType MaskArrayPath;
  BoolParameter::ValueType SliceBySlice;
  ChoicesParameter::ValueType SliceBySlicePlaneIndex;
};

/**
 * @class IdentifySample
 * @brief Dispatches largest-sample identification by mask storage.
 *
 * The largest face-connected true component remains the sample. Other true
 * components become false. Optional hole filling changes enclosed false
 * components to true.
 *
 * Resident execution uses BFS and volume-sized state. CCL uses sequential
 * scans, replay, and temporary equivalence records. Slice mode processes each
 * selected plane independently. The BFS and CCL paths use different slice
 * implementations. Storage overrides can force either path.
 */
class SIMPLNXCORE_EXPORT IdentifySample
{
public:
  /**
   * @brief Initializes the IdentifySample dispatcher.
   * @param dataStructure Contains the ImageGeom and mask.
   * @param mesgHandler Receives phase and slice messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects hole and slice behavior.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  IdentifySample(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IdentifySampleInputValues* inputValues);
  ~IdentifySample() noexcept;

  IdentifySample(const IdentifySample&) = delete;
  IdentifySample(IdentifySample&&) noexcept = delete;
  IdentifySample& operator=(const IdentifySample&) = delete;
  IdentifySample& operator=(IdentifySample&&) noexcept = delete;

  /**
   * @brief Retains the largest component and optionally fills holes.
   * @return Result from the selected implementation.
   * @pre The mask is scalar Bool or UInt8 and matches ImageGeom cell dimensions.
   * @pre SliceBySlicePlaneIndex identifies XY, XZ, or YZ.
   *
   * Both paths modify the mask in place. Errors or cancellation do not restore
   * prior slices or components.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IdentifySampleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
