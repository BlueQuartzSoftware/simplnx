#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

/**
 * @struct ErodeDilateCoordinationNumberInputValues
 * @brief Collects coordination settings and DataStructure paths.
 */
struct SIMPLNXCORE_EXPORT ErodeDilateCoordinationNumberInputValues
{
  int32 CoordinationNumber;
  bool Loop;
  DataPath FeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
  DataPath InputImageGeometry;
};

/**
 * @class ErodeDilateCoordinationNumber
 * @brief Smooths good/bad boundaries by face-neighbor coordination.
 *
 * A voxel changes when at least CoordinationNumber face neighbors have the
 * opposite good/bad state and the coordination is nonzero. Feature ID zero is
 * bad; positive IDs are good. The feature tally and face order choose the source.
 * Decisions and sibling copies see earlier changes in Z/Y/X order, with X fastest.
 * Arrays that share a selected store see the same changes, including ignored aliases.
 *
 * Three Feature ID slices and one mark slice bound spatial scratch.
 * The feature tally scales with the maximum positive Feature ID.
 * Each selected store transfer adds one destination and up to two adjacent source slices.
 */
class SIMPLNXCORE_EXPORT ErodeDilateCoordinationNumber
{
public:
  /**
   * @brief Initializes coordination-based smoothing.
   * @param dataStructure Contains geometry and sibling cell arrays.
   * @param mesgHandler Supplies the common interface. This algorithm emits no messages.
   * @param shouldCancel Supplies the common cancellation interface.
   * @param inputValues Selects the threshold, loop mode, and paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ErodeDilateCoordinationNumber(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateCoordinationNumberInputValues* inputValues);

  ~ErodeDilateCoordinationNumber() noexcept;

  ErodeDilateCoordinationNumber(const ErodeDilateCoordinationNumber&) = delete;
  ErodeDilateCoordinationNumber(ErodeDilateCoordinationNumber&&) noexcept = delete;
  ErodeDilateCoordinationNumber& operator=(const ErodeDilateCoordinationNumber&) = delete;
  ErodeDilateCoordinationNumber& operator=(ErodeDilateCoordinationNumber&&) noexcept = delete;

  /**
   * @brief Applies one pass or repeats while a pass selects qualifying voxels.
   * @return Success or the first invalid bulk read/write Result.
   * @pre Image dimensions and Feature ID tuple count agree and are nonzero.
   * @pre Feature IDs are nonnegative and slice/component products fit usize.
   *
   * The algorithm does not inspect the cancellation flag or emit progress messages.
   * A storage failure can leave earlier slices or sibling stores modified.
   * A failed write can also leave partial output. No rollback occurs here.
   * Loop has no independent pass limit or general convergence guarantee.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel() const;

private:
  DataStructure& m_DataStructure;
  const ErodeDilateCoordinationNumberInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
