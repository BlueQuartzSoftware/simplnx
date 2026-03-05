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
 * @brief Holds all user-supplied parameters for the ErodeDilateMask algorithm.
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
 * @brief Erodes or dilates a boolean mask array using face-neighbor connectivity.
 */
class SIMPLNXCORE_EXPORT ErodeDilateMask
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays
   * @param mesgHandler Handler for sending progress messages to the UI
   * @param shouldCancel Atomic flag checked between iterations to support cancellation
   * @param inputValues User-supplied parameters controlling the algorithm behavior
   */
  ErodeDilateMask(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateMaskInputValues* inputValues);

  /**
   * @brief Default destructor.
   */
  ~ErodeDilateMask() noexcept;

  ErodeDilateMask(const ErodeDilateMask&) = delete;
  ErodeDilateMask(ErodeDilateMask&&) noexcept = delete;
  ErodeDilateMask& operator=(const ErodeDilateMask&) = delete;
  ErodeDilateMask& operator=(ErodeDilateMask&&) noexcept = delete;

  /**
   * @brief Executes the erode/dilate mask algorithm.
   * @return Result<> indicating success or any errors encountered during execution
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
