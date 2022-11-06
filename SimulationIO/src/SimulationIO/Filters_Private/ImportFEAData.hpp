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
  static inline constexpr StringLiteral k_FEAPackage_Key = "f_ea_package";
  static inline constexpr StringLiteral k_odbName_Key = "odb_name";
  static inline constexpr StringLiteral k_odbFilePath_Key = "odb_file_path";
  static inline constexpr StringLiteral k_ABQPythonCommand_Key = "a_bq_python_command";
  static inline constexpr StringLiteral k_InstanceName_Key = "instance_name";
  static inline constexpr StringLiteral k_Step_Key = "step";
  static inline constexpr StringLiteral k_FrameNumber_Key = "frame_number";
  static inline constexpr StringLiteral k_BSAMInputFile_Key = "b_sa_minput_file";
  static inline constexpr StringLiteral k_DEFORMInputFile_Key = "d_ef_or_minput_file";
  static inline constexpr StringLiteral k_DEFORMPointTrackInputFile_Key = "d_ef_or_mpoint_track_input_file";
  static inline constexpr StringLiteral k_ImportSingleTimeStep_Key = "import_single_time_step";
  static inline constexpr StringLiteral k_SingleTimeStepValue_Key = "single_time_step_value";
  static inline constexpr StringLiteral k_TimeSeriesBundleName_Key = "time_series_bundle_name";
  static inline constexpr StringLiteral k_DataContainerName_Key = "data_container_name";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName_Key = "vertex_attribute_matrix_name";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";

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
