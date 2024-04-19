#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WriteAbaqusHexahedronFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT WriteAbaqusHexahedronFilter : public IFilter
{
public:
  WriteAbaqusHexahedronFilter() = default;
  ~WriteAbaqusHexahedronFilter() noexcept override = default;

  WriteAbaqusHexahedronFilter(const WriteAbaqusHexahedronFilter&) = delete;
  WriteAbaqusHexahedronFilter(WriteAbaqusHexahedronFilter&&) noexcept = delete;

  WriteAbaqusHexahedronFilter& operator=(const WriteAbaqusHexahedronFilter&) = delete;
  WriteAbaqusHexahedronFilter& operator=(WriteAbaqusHexahedronFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_HourglassStiffness_Key = "hourglass_stiffness";
  static inline constexpr StringLiteral k_JobName_Key = "job_name";
  static inline constexpr StringLiteral k_OutputPath_Key = "output_path";
  static inline constexpr StringLiteral k_FilePrefix_Key = "file_prefix";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteAbaqusHexahedronFilter, "4bc81b8c-1594-409a-89eb-3ea8d8f061b0");
/* LEGACY UUID FOR THIS FILTER 0559aa37-c5ad-549a-82d4-bff4bfcb6cc6 */
