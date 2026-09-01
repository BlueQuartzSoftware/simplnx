#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include <vector>

namespace nx::core
{

/**
 * @struct ComputeIPFColorsInputValues
 * @brief Identifies IPF color inputs.
 *
 * referenceDirection identifies the sample direction. Cell phases use zero for
 * unindexed cells.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsInputValues
{
  std::vector<float> referenceDirection;
  bool useMask = false;
  DataPath maskArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath cellEulerAnglesArrayPath;
  DataPath crystalStructuresArrayPath;
  DataPath cellIpfColorsArrayPath;
  ebsdlib::ColorKeyKind colorKey = ebsdlib::ColorKeyKind::TSL;
};

/**
 * @class ComputeIPFColors
 * @brief Dispatches IPF color computation.
 *
 * The direct executor uses the resident path. The scanline executor uses fixed
 * bulk-I/O pages for OOC targets. The direct executor gives no generic
 * DataArray or DataStore thread-safety guarantee.
 *
 * @see ComputeIPFColorsDirect
 * @see ComputeIPFColorsScanline
 */
class ORIENTATIONANALYSIS_EXPORT ComputeIPFColors
{
public:
  /**
   * @brief Initializes IPF color dispatch.
   * @param dataStructure Provides selected arrays.
   * @param msgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and color settings.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeIPFColors(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeIPFColorsInputValues* inputValues);
  /**
   * @brief Destroys the IPF color dispatcher.
   */
  ~ComputeIPFColors() noexcept;

  ComputeIPFColors(const ComputeIPFColors&) = delete;
  ComputeIPFColors(ComputeIPFColors&&) = delete;
  ComputeIPFColors& operator=(const ComputeIPFColors&) = delete;
  ComputeIPFColors& operator=(ComputeIPFColors&&) = delete;

  /**
   * @brief Dispatches IPF color computation.
   * @return Result from the selected executor.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeIPFColorsInputValues* m_InputValues = nullptr;
};

} // namespace nx::core
