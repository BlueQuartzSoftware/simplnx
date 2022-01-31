#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT CropImageGeometry : public IFilter
{
public:
  CropImageGeometry() = default;
  ~CropImageGeometry() noexcept override = default;

  CropImageGeometry(const CropImageGeometry&) = delete;
  CropImageGeometry(CropImageGeometry&&) noexcept = delete;

  CropImageGeometry& operator=(const CropImageGeometry&) = delete;
  CropImageGeometry& operator=(CropImageGeometry&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MinX_Key = "min_x";
  static inline constexpr StringLiteral k_MinY_Key = "min_y";
  static inline constexpr StringLiteral k_MinZ_Key = "min_z";
  static inline constexpr StringLiteral k_MaxX_Key = "max_x";
  static inline constexpr StringLiteral k_MaxY_Key = "max_y";
  static inline constexpr StringLiteral k_MaxZ_Key = "max_z";
  static inline constexpr StringLiteral k_UpdateOrigin_Key = "update_origin";
  static inline constexpr StringLiteral k_ImageGeom_Key = "image_geom";
  static inline constexpr StringLiteral k_NewImageGeom_Key = "new_image_geom";
  static inline constexpr StringLiteral k_VoxelArrays_Key = "voxel_arrays";
  static inline constexpr StringLiteral k_RenumberFeatures_Key = "renumber_features";
  static inline constexpr StringLiteral k_FeatureIds_Key = "feature_ids";
  static inline constexpr StringLiteral k_NewFeaturesName_Key = "new_features_group_name";

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
   * @param data
   * @param args
   * @param messageHandler
   * @return PreflightResult
   */
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, CropImageGeometry, "baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf");
