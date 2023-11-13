#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ReadEnsembleInfoFilter
 * @brief This filter reads in information about the crystal structure and phase types of all the Features that are contained in a Cell based volume.
 */
class ORIENTATIONANALYSIS_EXPORT ReadEnsembleInfoFilter : public IFilter
{
public:
  ReadEnsembleInfoFilter() = default;
  ~ReadEnsembleInfoFilter() noexcept override = default;

  ReadEnsembleInfoFilter(const ReadEnsembleInfoFilter&) = delete;
  ReadEnsembleInfoFilter(ReadEnsembleInfoFilter&&) noexcept = delete;

  ReadEnsembleInfoFilter& operator=(const ReadEnsembleInfoFilter&) = delete;
  ReadEnsembleInfoFilter& operator=(ReadEnsembleInfoFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFile_Key = "input_file";
  static inline constexpr StringLiteral k_DataContainerName_Key = "data_container_name";
  static inline constexpr StringLiteral k_CellEnsembleAttributeMatrixName_Key = "cell_ensemble_attribute_matrix_name";
  static inline constexpr StringLiteral k_CrystalStructuresArrayName_Key = "crystal_structures_array_name";
  static inline constexpr StringLiteral k_PhaseTypesArrayName_Key = "phase_types_array_name";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ReadEnsembleInfoFilter, "ecf1ec45-bc27-4ddb-b2c0-3d465b8c182a");
/* LEGACY UUID FOR THIS FILTER 33a37a47-d002-5c18-b270-86025881fe1e */
