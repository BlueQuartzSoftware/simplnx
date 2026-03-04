#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/AbstractFilter.hpp"
#include "simplnx/Filter/FilterTraits.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{
/**
 * @class ReadBinaryCTNorthstarFilter
 * @brief This filter will import a NorthStar Imaging data set consisting of a single .nsihdr and one
 * or more .nsidat files. The data is read into an Image Geometry. The user can import a subvolume
 * instead of reading the entire data set into memory.  The user should note that when using the
 * subvolume feature that the ending voxels are inclusive. The .nsihdr file will be read during
 * preflight and the .nsidat file(s) will be extracted from there. The expectation is that the .nsidat
 * files are in the same directory as the .nsihdr files.

 */
class SIMPLNXCORE_EXPORT ReadBinaryCTNorthstarFilter : public AbstractFilter
{
public:
  ReadBinaryCTNorthstarFilter();
  ~ReadBinaryCTNorthstarFilter() noexcept override;

  ReadBinaryCTNorthstarFilter(const ReadBinaryCTNorthstarFilter&) = delete;
  ReadBinaryCTNorthstarFilter(ReadBinaryCTNorthstarFilter&&) noexcept = delete;

  ReadBinaryCTNorthstarFilter& operator=(const ReadBinaryCTNorthstarFilter&) = delete;
  ReadBinaryCTNorthstarFilter& operator=(ReadBinaryCTNorthstarFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputHeaderFile_Key = "input_header_file";
  static constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static constexpr StringLiteral k_DensityArrayName_Key = "density_array_name";
  static constexpr StringLiteral k_LengthUnit_Key = "length_unit_index";
  static constexpr StringLiteral k_ImportSubvolume_Key = "import_subvolume";
  static constexpr StringLiteral k_StartVoxelCoord_Key = "start_voxel_coord";
  static constexpr StringLiteral k_EndVoxelCoord_Key = "end_voxel_coord";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  struct ImageGeometryInfo
  {
    std::vector<float32> Origin;
    std::vector<float32> Spacing;
    std::vector<usize> Dimensions;
  };

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
   * @brief Returns the human-readable name of the filter.
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
   * @brief Returns parameters version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;

private:
  int32 m_InstanceId;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadBinaryCTNorthstarFilter, "5469af5c-368a-465b-87b7-7c0dfdf73666");
/* LEGACY UUID FOR THIS FILTER f2259481-5011-5f22-9fcb-c92fb6f8be10 */
