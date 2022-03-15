#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT ExtractInternalSurfacesFromTriangleGeometry : public IFilter
{
public:
  ExtractInternalSurfacesFromTriangleGeometry() = default;
  ~ExtractInternalSurfacesFromTriangleGeometry() noexcept override = default;

  ExtractInternalSurfacesFromTriangleGeometry(const ExtractInternalSurfacesFromTriangleGeometry&) = delete;
  ExtractInternalSurfacesFromTriangleGeometry(ExtractInternalSurfacesFromTriangleGeometry&&) noexcept = delete;

  ExtractInternalSurfacesFromTriangleGeometry& operator=(const ExtractInternalSurfacesFromTriangleGeometry&) = delete;
  ExtractInternalSurfacesFromTriangleGeometry& operator=(ExtractInternalSurfacesFromTriangleGeometry&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_TriangleGeom_Key = "triangle_geom";
  static inline constexpr StringLiteral k_InternalTriangleGeom_Key = "internal_triangle_geom";
  static inline constexpr StringLiteral k_NodeTypesPath_Key = "node_types";
  static inline constexpr StringLiteral k_CopyVertexPaths_Key = "copy_vertex_array_paths";
  static inline constexpr StringLiteral k_CopyTrianglePaths_Key = "copy_triangle_array_paths";

  /**
   * @brief Returns the filter's name.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the filter's UUID.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the filter name as a human-readable string.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns a collection of parameters required to execute the filter.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Creates and returns a copy of the filter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param filterArgs
   * @param messageHandler
   * @param shouldCancel
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

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
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ExtractInternalSurfacesFromTriangleGeometry, "52a069b4-6a46-5810-b0ec-e0693c636034");
