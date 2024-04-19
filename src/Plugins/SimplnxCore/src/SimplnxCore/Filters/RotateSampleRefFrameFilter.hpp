#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class RotateSampleRefFrameFilter
 * @brief
 */
class SIMPLNXCORE_EXPORT RotateSampleRefFrameFilter : public IFilter
{
public:
  RotateSampleRefFrameFilter() = default;
  ~RotateSampleRefFrameFilter() noexcept override = default;

  RotateSampleRefFrameFilter(const RotateSampleRefFrameFilter&) = delete;
  RotateSampleRefFrameFilter(RotateSampleRefFrameFilter&&) noexcept = delete;

  RotateSampleRefFrameFilter& operator=(const RotateSampleRefFrameFilter&) = delete;
  RotateSampleRefFrameFilter& operator=(RotateSampleRefFrameFilter&&) noexcept = delete;

  enum class RotationRepresentation : uint64
  {
    AxisAngle = 0,
    RotationMatrix = 1
  };

  // Parameter Keys
  static inline constexpr StringLiteral k_RotationRepresentation_Key = "rotation_representation_index";
  static inline constexpr StringLiteral k_RotationAxisAngle_Key = "rotation_axis_angle";
  static inline constexpr StringLiteral k_RotationMatrix_Key = "rotation_matrix";
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_CreatedImageGeometryPath_Key = "output_image_geometry_path";
  static inline constexpr StringLiteral k_RotateSliceBySlice_Key = "rotate_slice_by_slice";
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
   * @return
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
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param dataStructure
   * @param filterArgs
   * @param pipelineNode
   * @param messageHandler
   * @param shouldCancel
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RotateSampleRefFrameFilter, "d2451dc1-a5a1-4ac2-a64d-7991669dcffc");
