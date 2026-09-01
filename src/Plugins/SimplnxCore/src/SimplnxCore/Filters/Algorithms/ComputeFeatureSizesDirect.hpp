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

struct ComputeFeatureSizesInputValues;

/**
 * @class ComputeFeatureSizesDirect
 * @brief Computes feature sizes with direct parallel accumulation.
 *
 * Workers use thread-local feature counts and RectGrid Kahan volume sums. The normal dispatcher
 * selects this path for resident Feature IDs. requireStoresInMemory() only disables parallel
 * scheduling for a nonresident Feature ID store. It does not make generic DataArray or DataStore
 * concurrent access safe.
 *
 * RectGrid workers also access generated element sizes. The direct scheduling guard does not include
 * that store. A forced direct out-of-core run can use per-element access.
 *
 * @see ComputeFeatureSizesScanline.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureSizesDirect
{
public:
  /**
   * @brief Initializes the direct feature-size algorithm.
   * @param dataStructure Contains geometry, Feature IDs, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation between Z slices or features.
   * @param inputValues Selects outputs and required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureSizesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeFeatureSizesInputValues* inputValues);
  /**
   * @brief Destroys the direct feature-size algorithm.
   */
  ~ComputeFeatureSizesDirect() noexcept;

  ComputeFeatureSizesDirect(const ComputeFeatureSizesDirect&) = delete;
  ComputeFeatureSizesDirect(ComputeFeatureSizesDirect&&) noexcept = delete;
  ComputeFeatureSizesDirect& operator=(const ComputeFeatureSizesDirect&) = delete;
  ComputeFeatureSizesDirect& operator=(ComputeFeatureSizesDirect&&) noexcept = delete;

  /**
   * @brief Computes feature sizes with direct accumulation.
   * @return Success, or a validation, geometry, or feature-count error.
   *
   * Cancellation stops workers at Z-slice checks and output loops at feature checks. Earlier
   * feature output remains written. Generated RectGrid element sizes can remain after cancellation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureSizesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
