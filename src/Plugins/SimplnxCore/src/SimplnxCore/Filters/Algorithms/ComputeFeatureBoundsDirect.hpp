#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

struct ComputeFeatureBoundsInputValues;

/**
 * @class ComputeFeatureBoundsDirect
 * @brief Computes feature bounds with resident direct access.
 *
 * ImageGeom input uses feature-sized index extrema and row runs. Other supported geometries use
 * direct vertex and connectivity access. The normal dispatcher avoids this path for out-of-core
 * Feature IDs. A forced direct out-of-core run can perform per-element store access.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureBoundsDirect
{
public:
  /**
   * @brief Initializes the direct feature-bound algorithm.
   * @param dataStructure Contains geometry, Feature IDs, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation before computation starts.
   * @param inputValues Selects output layout and required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureBoundsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeFeatureBoundsInputValues* inputValues);
  /**
   * @brief Destroys the direct feature-bound algorithm.
   */
  ~ComputeFeatureBoundsDirect() noexcept;

  ComputeFeatureBoundsDirect(const ComputeFeatureBoundsDirect&) = delete;
  ComputeFeatureBoundsDirect(ComputeFeatureBoundsDirect&&) noexcept = delete;
  ComputeFeatureBoundsDirect& operator=(const ComputeFeatureBoundsDirect&) = delete;
  ComputeFeatureBoundsDirect& operator=(ComputeFeatureBoundsDirect&&) noexcept = delete;

  /**
   * @brief Computes feature bounds with direct element access.
   * @return Success, or a geometry or feature-sizing error.
   *
   * A cancellation signal before execution returns success without output changes. The direct path
   * does not inspect cancellation after it starts.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureBoundsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
