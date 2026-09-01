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
 * opposite good/bad state. Feature ID zero is bad. The most common opposite
 * Feature ID supplies all selected sibling values. A tie keeps the first face
 * neighbor in traversal order.
 *
 * Three Feature ID slices, three mark slices, and three coordination slices
 * bound spatial scratch. The feature tally scales with the maximum positive
 * Feature ID. Each sibling transfer adds one destination and up to three source
 * slices for that array. Loop repeats until no qualifying voxel remains.
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
   * @brief Applies one pass or loops until no voxel changes.
   * @return Success.
   * @pre Image dimensions and Feature ID tuple count agree and are nonzero.
   * @pre Feature IDs are nonnegative and slice/component products fit usize.
   *
   * The algorithm does not inspect the cancellation flag. It discards all bulk-
   * transfer Result values. A storage failure can therefore produce partial or
   * invalid sibling output while this function returns success. Loop has no
   * independent pass limit.
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
