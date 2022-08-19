#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ImportFEAData
 * @brief This filter will ....
 */
class SIMULATIONIO_EXPORT ImportFEAData : public IFilter
{
public:
  ImportFEAData() = default;
  ~ImportFEAData() noexcept override = default;

  ImportFEAData(const ImportFEAData&) = delete;
  ImportFEAData(ImportFEAData&&) noexcept = delete;

  ImportFEAData& operator=(const ImportFEAData&) = delete;
  ImportFEAData& operator=(ImportFEAData&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FEAPackage_Key = "FEAPackage";
  static inline constexpr StringLiteral k_odbName_Key = "odbName";
  static inline constexpr StringLiteral k_odbFilePath_Key = "odbFilePath";
  static inline constexpr StringLiteral k_ABQPythonCommand_Key = "ABQPythonCommand";
  static inline constexpr StringLiteral k_InstanceName_Key = "InstanceName";
  static inline constexpr StringLiteral k_Step_Key = "Step";
  static inline constexpr StringLiteral k_FrameNumber_Key = "FrameNumber";
  static inline constexpr StringLiteral k_BSAMInputFile_Key = "BSAMInputFile";
  static inline constexpr StringLiteral k_DEFORMInputFile_Key = "DEFORMInputFile";
  static inline constexpr StringLiteral k_DEFORMPointTrackInputFile_Key = "DEFORMPointTrackInputFile";
  static inline constexpr StringLiteral k_ImportSingleTimeStep_Key = "ImportSingleTimeStep";
  static inline constexpr StringLiteral k_SingleTimeStepValue_Key = "SingleTimeStepValue";
  static inline constexpr StringLiteral k_TimeSeriesBundleName_Key = "TimeSeriesBundleName";
  static inline constexpr StringLiteral k_DataContainerName_Key = "DataContainerName";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName_Key = "VertexAttributeMatrixName";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "CellAttributeMatrixName";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ImportFEAData, "1489ef78-b05f-4bbd-a35d-892ce7521dc2");
