#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeBoundaryCellsInputValues
 * @brief Defines geometry, arrays, and feature-zero and volume-boundary policies.
 */
struct SIMPLNXCORE_EXPORT ComputeBoundaryCellsInputValues
{
  bool IgnoreFeatureZero;
  bool IncludeVolumeBoundary;
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath BoundaryCellsArrayName;
};

/**
 * @class ComputeBoundaryCells
 * @brief Dispatches six-face boundary counting from Feature IDs storage.
 *
 * Each output Int8 value is the number of face neighbors in another permitted
 * feature, with optional image-volume boundary contributions. Values are in the
 * range [0, 6]. A volume face contributes only when its axis has more than two
 * cells. Negative current Feature IDs and feature-zero volume faces produce zero.
 *
 * Dispatch inspects only FeatureIds. It does not include the output store. A
 * resident input and disk-backed output can select direct per-element writes.
 * Test overrides can also force either path.
 */
class SIMPLNXCORE_EXPORT ComputeBoundaryCells
{
public:
  /**
   * @brief Initializes boundary-cell dispatch.
   * @param dataStructure Provides geometry, input, and output arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between slices.
   * @param inputValues Defines paths and counting policies.
   * @pre All arguments outlive this dispatcher.
   */
  ComputeBoundaryCells(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCells() noexcept;

  ComputeBoundaryCells(const ComputeBoundaryCells&) = delete;
  ComputeBoundaryCells(ComputeBoundaryCells&&) noexcept = delete;
  ComputeBoundaryCells& operator=(const ComputeBoundaryCells&) = delete;
  ComputeBoundaryCells& operator=(ComputeBoundaryCells&&) noexcept = delete;

  /**
   * @brief Dispatches from the Feature IDs store type.
   * @return Success. Scanline bulk-I/O failures are not returned.
   * @pre Geometry dimensions are positive and arrays match the image cell count.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryCellsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
