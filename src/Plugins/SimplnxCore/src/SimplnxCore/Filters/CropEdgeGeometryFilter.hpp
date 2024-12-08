#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT CropEdgeGeometryFilter : public IFilter
{
public:
  CropEdgeGeometryFilter();
  ~CropEdgeGeometryFilter() noexcept override;

  CropEdgeGeometryFilter(const CropEdgeGeometryFilter&) = delete;
  CropEdgeGeometryFilter(CropEdgeGeometryFilter&&) noexcept = delete;

  CropEdgeGeometryFilter& operator=(const CropEdgeGeometryFilter&) = delete;
  CropEdgeGeometryFilter& operator=(CropEdgeGeometryFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CropXDim_Key = "crop_x_dim";
  static inline constexpr StringLiteral k_CropYDim_Key = "crop_y_dim";
  static inline constexpr StringLiteral k_CropZDim_Key = "crop_z_dim";
  static inline constexpr StringLiteral k_MinCoord_Key = "min_coord";
  static inline constexpr StringLiteral k_MaxCoord_Key = "max_coord";
  static inline constexpr StringLiteral k_SelectedEdgeGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_CreatedEdgeGeometryPath_Key = "output_image_geometry_path";
  static inline constexpr StringLiteral k_RemoveOriginalGeometry_Key = "remove_original_geometry";
  static inline constexpr StringLiteral k_BoundaryIntersectionBehavior_Key = "boundary_intersection_behavior_index";

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
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, CropEdgeGeometryFilter, "e3e794c9-ef0c-4ed2-a25f-8d19b6b2ce4c");
