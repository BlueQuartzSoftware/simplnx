#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class RotateSampleRefFrame
 * @brief
 */
class COMPLEXCORE_EXPORT RotateSampleRefFrame : public IFilter
{
public:
  RotateSampleRefFrame() = default;
  ~RotateSampleRefFrame() noexcept override = default;

  RotateSampleRefFrame(const RotateSampleRefFrame&) = delete;
  RotateSampleRefFrame(RotateSampleRefFrame&&) noexcept = delete;

  RotateSampleRefFrame& operator=(const RotateSampleRefFrame&) = delete;
  RotateSampleRefFrame& operator=(RotateSampleRefFrame&&) noexcept = delete;

  enum class RotationRepresentation : uint64
  {
    AxisAngle = 0,
    RotationMatrix = 1
  };

  // Parameter Keys
  static inline constexpr StringLiteral k_RotationRepresentation_Key = "rotation_representation";
  static inline constexpr StringLiteral k_RotationAngle_Key = "rotation_angle";
  static inline constexpr StringLiteral k_RotationAxis_Key = "rotation_axis";
  static inline constexpr StringLiteral k_RotationMatrix_Key = "rotation_matrix";
  static inline constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";
  static inline constexpr StringLiteral k_SelectedCellArrays_Key = "selected_cell_arrays";
  static inline constexpr StringLiteral k_CreatedImageGeometry_Key = "created_image_geometry";
  static inline constexpr StringLiteral k_RotateSliceBySlice_Key = "rotate_slice_by_slice";

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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, RotateSampleRefFrame, "5efdf395-33fb-4dc0-986e-0dc0ae990f6a");
