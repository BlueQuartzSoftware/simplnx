#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

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
 * @struct ErodeDilateMaskInputValues
 * @brief Collects mask morphology settings and DataStructure paths.
 */
struct SIMPLNXCORE_EXPORT ErodeDilateMaskInputValues
{
  ChoicesParameter::ValueType Operation;
  int32 NumIterations;
  bool XDirOn;
  bool YDirOn;
  bool ZDirOn;
  DataPath MaskArrayPath;
  DataPath InputImageGeometry;
};

/**
 * @class ErodeDilateMask
 * @brief Erodes or dilates a Boolean mask with face neighbors.
 *
 * Dilation changes a false voxel when an enabled face neighbor is true. Erosion
 * changes a true voxel when an enabled face neighbor is false. Separate read
 * and write windows give synchronous state within each pass.
 *
 * Six byte slices and one Boolean transfer slice bound working memory.
 * Byte slices avoid std::vector<bool> bit-packing. Completed slices are written
 * sequentially and become input to the next pass.
 */
class SIMPLNXCORE_EXPORT ErodeDilateMask
{
public:
  /**
   * @brief Initializes mask morphology.
   * @param dataStructure Contains the ImageGeom and mask.
   * @param mesgHandler Receives one message for each pass.
   * @param shouldCancel Supplies the common cancellation interface.
   * @param inputValues Selects operation, directions, iterations, and paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ErodeDilateMask(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateMaskInputValues* inputValues);

  ~ErodeDilateMask() noexcept;

  ErodeDilateMask(const ErodeDilateMask&) = delete;
  ErodeDilateMask(ErodeDilateMask&&) noexcept = delete;
  ErodeDilateMask& operator=(const ErodeDilateMask&) = delete;
  ErodeDilateMask& operator=(ErodeDilateMask&&) noexcept = delete;

  /**
   * @brief Applies the selected number of mask morphology passes.
   * @return Success.
   * @pre Image dimensions and mask tuple count agree and are nonzero.
   * @pre Slice-size products fit usize.
   *
   * The algorithm does not inspect the cancellation flag. It discards all bulk-
   * transfer Result values. A storage failure can therefore produce partial or
   * invalid mask output while this function returns success.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel() const;

private:
  DataStructure& m_DataStructure;
  const ErodeDilateMaskInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
