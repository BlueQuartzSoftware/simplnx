#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT CropImageGeometry : public IFilter
{
public:
  CropImageGeometry();
  ~CropImageGeometry() noexcept override;

  CropImageGeometry(const CropImageGeometry&) = delete;
  CropImageGeometry(CropImageGeometry&&) noexcept = delete;

  CropImageGeometry& operator=(const CropImageGeometry&) = delete;
  CropImageGeometry& operator=(CropImageGeometry&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_UsePhysicalBounds_Key = "use_physical_bounds";
  static inline constexpr StringLiteral k_MinVoxel_Key = "min_voxel";
  static inline constexpr StringLiteral k_MaxVoxel_Key = "max_voxel";
  static inline constexpr StringLiteral k_MinCoord_Key = "min_coord";
  static inline constexpr StringLiteral k_MaxCoord_Key = "max_coord";
  // static inline constexpr StringLiteral k_UpdateOrigin_Key = "update_origin";
  static inline constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";
  static inline constexpr StringLiteral k_CreatedImageGeometry_Key = "created_image_geometry";
  static inline constexpr StringLiteral k_RenumberFeatures_Key = "renumber_features";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids";
  static inline constexpr StringLiteral k_FeatureAttributeMatrix_Key = "cell_feature_attribute_matrix";
  static inline constexpr StringLiteral k_RemoveOriginalGeometry_Key = "remove_original_geometry";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return std::vector<std::string>
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param filterArgs
   * @param messageHandler
   * @param shouldCancel
   * @return PreflightResult
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @param shouldCancel
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;

private:
  int32 m_InstanceId;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, CropImageGeometry, "e6476737-4aa7-48ba-a702-3dfab82c96e2");
