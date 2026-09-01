#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{
namespace detail
{
static inline constexpr StringLiteral k_DilateString = "Dilate";
static inline constexpr StringLiteral k_ErodeString = "Erode";
static inline const ChoicesParameter::Choices k_OperationChoices = {k_DilateString, k_ErodeString};

static inline constexpr ChoicesParameter::ValueType k_DilateIndex = 0ULL;
static inline constexpr ChoicesParameter::ValueType k_ErodeIndex = 1ULL;
} // namespace detail

/**
 * @struct ErodeDilateBadDataInputValues
 * @brief Collects morphology settings and DataStructure paths.
 */
struct SIMPLNXCORE_EXPORT ErodeDilateBadDataInputValues
{
  ChoicesParameter::ValueType Operation;
  int32 NumIterations;
  bool XDirOn;
  bool YDirOn;
  bool ZDirOn;
  DataPath FeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
  DataPath InputImageGeometry;
};

/**
 * @class ErodeDilateBadData
 * @brief Erodes or dilates Feature ID zero regions on an ImageGeom.
 *
 * Dilation copies bad tuples into adjacent good cells. Erosion copies the most
 * common adjacent good-feature tuple into each bad cell. A tie keeps the first
 * face neighbor in traversal order. Selected sibling arrays follow the same
 * source-to-destination mapping. If several bad cells target one good neighbor,
 * the last scanned source supplies its sibling values.
 *
 * Three Feature ID slices and three mark slices bound spatial scratch. The
 * erosion tally also scales with the maximum positive Feature ID. Deferred
 * writes commit a slice only after later voxels cannot change its marks. Each
 * sibling transfer temporarily adds one destination and up to three source
 * slices for that array.
 */
class SIMPLNXCORE_EXPORT ErodeDilateBadData
{
public:
  /**
   * @brief Initializes Feature ID morphology.
   * @param dataStructure Contains geometry and sibling cell arrays.
   * @param mesgHandler Supplies the common interface. This algorithm emits no messages.
   * @param shouldCancel Supplies the common cancellation interface.
   * @param inputValues Selects operation, directions, iterations, and paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ErodeDilateBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateBadDataInputValues* inputValues);

  ~ErodeDilateBadData() noexcept;

  ErodeDilateBadData(const ErodeDilateBadData&) = delete;
  ErodeDilateBadData(ErodeDilateBadData&&) noexcept = delete;
  ErodeDilateBadData& operator=(const ErodeDilateBadData&) = delete;
  ErodeDilateBadData& operator=(ErodeDilateBadData&&) noexcept = delete;

  /**
   * @brief Applies the selected number of morphology passes.
   * @return Success.
   * @pre Image dimensions and Feature ID tuple count agree and are nonzero.
   * @pre Feature IDs are nonnegative and slice/component products fit usize.
   *
   * The algorithm does not inspect the cancellation flag. It also discards all
   * bulk-transfer Result values. A storage failure can therefore produce partial
   * or invalid sibling output while this function returns success.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel() const;

private:
  DataStructure& m_DataStructure;
  const ErodeDilateBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
