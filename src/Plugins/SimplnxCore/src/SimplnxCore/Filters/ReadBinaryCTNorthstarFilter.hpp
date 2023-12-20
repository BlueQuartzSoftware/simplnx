#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

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
class SIMPLNXCORE_EXPORT ReadBinaryCTNorthstarFilter : public IFilter
{
public:
  ReadBinaryCTNorthstarFilter();
  ~ReadBinaryCTNorthstarFilter() noexcept override;

  ReadBinaryCTNorthstarFilter(const ReadBinaryCTNorthstarFilter&) = delete;
  ReadBinaryCTNorthstarFilter(ReadBinaryCTNorthstarFilter&&) noexcept = delete;

  ReadBinaryCTNorthstarFilter& operator=(const ReadBinaryCTNorthstarFilter&) = delete;
  ReadBinaryCTNorthstarFilter& operator=(ReadBinaryCTNorthstarFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputHeaderFile_Key = "input_header_file";
  static inline constexpr StringLiteral k_ImageGeometryPath_Key = "image_geometry_path";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_DensityArrayName_Key = "density_array_name";
  static inline constexpr StringLiteral k_LengthUnit_Key = "length_unit";
  static inline constexpr StringLiteral k_ImportSubvolume_Key = "import_subvolume";
  static inline constexpr StringLiteral k_StartVoxelCoord_Key = "start_voxel_coord";
  static inline constexpr StringLiteral k_EndVoxelCoord_Key = "end_voxel_coord";

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

private:
  int32 m_InstanceId;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadBinaryCTNorthstarFilter, "5469af5c-368a-465b-87b7-7c0dfdf73666");
/* LEGACY UUID FOR THIS FILTER f2259481-5011-5f22-9fcb-c92fb6f8be10 */
