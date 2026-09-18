#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"

namespace nx::core
{

/**
 * @struct RodriguesConvertorInputValues
 * @brief Identifies the Rodrigues input, four-component output, and cleanup option.
 */
struct ORIENTATIONANALYSIS_EXPORT RodriguesConvertorInputValues
{
  DataPath RodriguesDataArrayPath;
  DataPath OutputDataArrayPath;
  bool DeleteOriginalData;
};

/**
 * @class RodriguesConvertor
 * @brief Converts Rodrigues triples to a unit axis and magnitude.
 *
 * Resident arrays use direct parallel indexing. An out-of-core input or output
 * selects a sequential 65,536-tuple bulk-I/O path. Test overrides can force
 * either path.
 *
 * The direct path has no generic DataArray or DataStore thread-safety guarantee.
 * Cancellation returns success and preserves completed tuples or pages.
 */
class ORIENTATIONANALYSIS_EXPORT RodriguesConvertor
{
public:
  /**
   * @brief Initializes a Rodrigues conversion executor.
   * @param dataStructure Provides input and output arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies arrays and cleanup settings.
   * @pre All arguments outlive this executor.
   */
  RodriguesConvertor(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RodriguesConvertorInputValues* inputValues);
  ~RodriguesConvertor() noexcept;

  RodriguesConvertor(const RodriguesConvertor&) = delete;
  RodriguesConvertor(RodriguesConvertor&&) noexcept = delete;
  RodriguesConvertor& operator=(const RodriguesConvertor&) = delete;
  RodriguesConvertor& operator=(RodriguesConvertor&&) noexcept = delete;

  /**
   * @brief Converts all input tuples.
   * @return Bulk-I/O errors from the out-of-core path.
   * @pre Input tuples have three components and output tuples have four components.
   * @pre Input and output tuple counts match.
   * @pre Each Rodrigues triple has nonzero magnitude.
   *
   * A zero triple produces nonfinite axis components in the current implementation.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RodriguesConvertorInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
