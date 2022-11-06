#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ExportDAMASKFiles
 * @brief This filter will ....
 */
class SIMULATIONIO_EXPORT ExportDAMASKFiles : public IFilter
{
public:
  ExportDAMASKFiles() = default;
  ~ExportDAMASKFiles() noexcept override = default;

  ExportDAMASKFiles(const ExportDAMASKFiles&) = delete;
  ExportDAMASKFiles(ExportDAMASKFiles&&) noexcept = delete;

  ExportDAMASKFiles& operator=(const ExportDAMASKFiles&) = delete;
  ExportDAMASKFiles& operator=(ExportDAMASKFiles&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_DataFormat_Key = "data_format";
  static inline constexpr StringLiteral k_OutputPath_Key = "output_path";
  static inline constexpr StringLiteral k_GeometryFileName_Key = "geometry_file_name";
  static inline constexpr StringLiteral k_HomogenizationIndex_Key = "homogenization_index";
  static inline constexpr StringLiteral k_CompressGeomFile_Key = "compress_geom_file";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ExportDAMASKFiles, "ab4b77ef-973c-48dc-977e-2771e3163095");
