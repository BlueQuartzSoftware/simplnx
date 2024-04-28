#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT FindNeighborsFilter : public IFilter
{
public:
  FindNeighborsFilter() = default;
  ~FindNeighborsFilter() noexcept override = default;

  FindNeighborsFilter(const FindNeighborsFilter&) = delete;
  FindNeighborsFilter(FindNeighborsFilter&&) noexcept = delete;

  FindNeighborsFilter& operator=(const FindNeighborsFilter&) = delete;
  FindNeighborsFilter& operator=(FindNeighborsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_StoreBoundary_Key = "store_boundary_cells";
  static inline constexpr StringLiteral k_StoreSurface_Key = "store_surface_features";
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_CellFeaturesPath_Key = "cell_feature_array_path";
  static inline constexpr StringLiteral k_BoundaryCellsName_Key = "boundary_cells_name";
  static inline constexpr StringLiteral k_NumNeighborsName_Key = "number_of_neighbors_name";
  static inline constexpr StringLiteral k_NeighborListName_Key = "neighbor_list_name";
  static inline constexpr StringLiteral k_SharedSurfaceAreaName_Key = "shared_surface_area_list_name";
  static inline constexpr StringLiteral k_SurfaceFeaturesName_Key = "surface_features_name";

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
   * @brief
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return IFilter::UniquePointer
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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FindNeighborsFilter, "7177e88c-c3ab-4169-abe9-1fdaff20e598");
