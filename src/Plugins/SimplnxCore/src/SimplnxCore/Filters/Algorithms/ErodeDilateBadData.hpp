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
 * @brief Holds all user-supplied parameters for the ErodeDilateBadData algorithm.
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
 * @brief Erodes or dilates bad (FeatureId == 0) voxels by replacing them with the most common neighbor feature.
 */
class SIMPLNXCORE_EXPORT ErodeDilateBadData
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays
   * @param mesgHandler Handler for sending progress messages to the UI
   * @param shouldCancel Atomic flag checked between iterations to support cancellation
   * @param inputValues User-supplied parameters controlling the algorithm behavior
   */
  ErodeDilateBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateBadDataInputValues* inputValues);

  /**
   * @brief Default destructor.
   */
  ~ErodeDilateBadData() noexcept;

  ErodeDilateBadData(const ErodeDilateBadData&) = delete;
  ErodeDilateBadData(ErodeDilateBadData&&) noexcept = delete;
  ErodeDilateBadData& operator=(const ErodeDilateBadData&) = delete;
  ErodeDilateBadData& operator=(ErodeDilateBadData&&) noexcept = delete;

  /**
   * @brief Executes the erode/dilate bad data algorithm.
   * @return Result<> indicating success or any errors encountered during execution
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
