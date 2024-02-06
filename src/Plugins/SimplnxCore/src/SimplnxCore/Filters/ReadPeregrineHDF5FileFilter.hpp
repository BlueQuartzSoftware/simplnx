#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Parameters.hpp"

namespace nx::core
{
/**
 * @class ReadPeregrineHDF5FileFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT ReadPeregrineHDF5FileFilter : public IFilter
{
public:
  ReadPeregrineHDF5FileFilter() = default;
  ~ReadPeregrineHDF5FileFilter() noexcept override = default;

  ReadPeregrineHDF5FileFilter(const ReadPeregrineHDF5FileFilter&) = delete;
  ReadPeregrineHDF5FileFilter(ReadPeregrineHDF5FileFilter&&) noexcept = delete;

  ReadPeregrineHDF5FileFilter& operator=(const ReadPeregrineHDF5FileFilter&) = delete;
  ReadPeregrineHDF5FileFilter& operator=(ReadPeregrineHDF5FileFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFilePath_Key = "input_file_path";
  static inline constexpr StringLiteral k_ReadSubvolume_Key = "read_subvolume";
  static inline constexpr StringLiteral k_SubvolumeDimensions_Key = "subvolume_dimensions";
  static inline constexpr StringLiteral k_ReadCameraData_Key = "read_camera_data";
  static inline constexpr StringLiteral k_ReadPartIds_Key = "read_part_ids";
  static inline constexpr StringLiteral k_ReadSampleIds_Key = "read_sample_ids";
  static inline constexpr StringLiteral k_SegmentationResults_Key = "segmentation_results";
  static inline constexpr StringLiteral k_SliceData_Key = "slice_data";
  static inline constexpr StringLiteral k_SliceDataCellAttrMat_Key = "slice_data_cell_attr_mat";
  static inline constexpr StringLiteral k_CameraData0ArrayName_Key = "camera_data_0_array_name";
  static inline constexpr StringLiteral k_CameraData1ArrayName_Key = "camera_data_1_array_name";
  static inline constexpr StringLiteral k_PartIdsArrayName_Key = "part_ids_array_name";
  static inline constexpr StringLiteral k_SampleIdsArrayName_Key = "sample_ids_array_name";
  static inline constexpr StringLiteral k_RegisteredData_Key = "registered_data";
  static inline constexpr StringLiteral k_RegisteredDataCellAttrMat_Key = "registered_data_cell_attr_mat";

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
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadPeregrineHDF5FileFilter, "974575ea-74d4-44ae-9277-fa8e68ccea1c");
