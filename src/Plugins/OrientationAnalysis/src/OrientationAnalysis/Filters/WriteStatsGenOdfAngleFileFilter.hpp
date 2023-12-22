#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WriteStatsGenOdfAngleFileFilter
 * @brief This filter will generate a synthetic microstructure with an ODF that matches (as closely as possible) an existing experimental data set or other data set that is being mimicked.
 */
class ORIENTATIONANALYSIS_EXPORT WriteStatsGenOdfAngleFileFilter : public IFilter
{
public:
  WriteStatsGenOdfAngleFileFilter() = default;
  ~WriteStatsGenOdfAngleFileFilter() noexcept override = default;

  WriteStatsGenOdfAngleFileFilter(const WriteStatsGenOdfAngleFileFilter&) = delete;
  WriteStatsGenOdfAngleFileFilter(WriteStatsGenOdfAngleFileFilter&&) noexcept = delete;

  WriteStatsGenOdfAngleFileFilter& operator=(const WriteStatsGenOdfAngleFileFilter&) = delete;
  WriteStatsGenOdfAngleFileFilter& operator=(WriteStatsGenOdfAngleFileFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_OutputFile_Key = "output_file";
  static inline constexpr StringLiteral k_Weight_Key = "weight";
  static inline constexpr StringLiteral k_Sigma_Key = "sigma";
  static inline constexpr StringLiteral k_Delimiter_Key = "delimiter";
  static inline constexpr StringLiteral k_ConvertToDegrees_Key = "convert_to_degrees";
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  static inline constexpr uint64 k_CommaDelimiter = 0;
  static inline constexpr uint64 k_SemiColonDelimiter = 1;
  static inline constexpr uint64 k_SpaceDelimiter = 2;
  static inline constexpr uint64 k_ColonDelimiter = 3;
  static inline constexpr uint64 k_TabDelimiter = 4;

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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteStatsGenOdfAngleFileFilter, "aa6d399b-715e-44f1-9902-f1bd18faf1c5");
/* LEGACY UUID FOR THIS FILTER a4952f40-22dd-54ec-8c38-69c3fcd0e6f7 */
