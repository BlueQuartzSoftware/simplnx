#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT FindNeighbors : public IFilter
{
public:
  FindNeighbors() = default;
  ~FindNeighbors() noexcept override = default;

  FindNeighbors(const FindNeighbors&) = delete;
  FindNeighbors(FindNeighbors&&) noexcept = delete;

  FindNeighbors& operator=(const FindNeighbors&) = delete;
  FindNeighbors& operator=(FindNeighbors&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_StoreBoundary_Key = "store_boundary_cells";
  static inline constexpr StringLiteral k_StoreSurface_Key = "store_surface_features";
  static inline constexpr StringLiteral k_ImageGeom_Key = "image_geometry";
  static inline constexpr StringLiteral k_FeatureIds_Key = "feature_ids";
  static inline constexpr StringLiteral k_CellFeatures_Key = "cell_feature_arrays";
  static inline constexpr StringLiteral k_BoundaryCells_Key = "boundary_cells";
  static inline constexpr StringLiteral k_NumNeighbors_Key = "number_of_neighbors";
  static inline constexpr StringLiteral k_NeighborList_Key = "neighbor_list";
  static inline constexpr StringLiteral k_SharedSurfaceArea_Key = "shared_surface_area_list";
  static inline constexpr StringLiteral k_SurfaceFeatures_Key = "surface_features";

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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindNeighbors, "97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac");
