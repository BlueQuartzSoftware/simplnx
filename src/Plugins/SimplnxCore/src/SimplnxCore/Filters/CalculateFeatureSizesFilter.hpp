#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class CalculateFeatureSizesFilter
 * @brief
 */
class SIMPLNXCORE_EXPORT CalculateFeatureSizesFilter : public IFilter
{
public:
  CalculateFeatureSizesFilter() = default;
  ~CalculateFeatureSizesFilter() noexcept override = default;

  CalculateFeatureSizesFilter(const CalculateFeatureSizesFilter&) = delete;
  CalculateFeatureSizesFilter(CalculateFeatureSizesFilter&&) noexcept = delete;

  CalculateFeatureSizesFilter& operator=(const CalculateFeatureSizesFilter&) = delete;
  CalculateFeatureSizesFilter& operator=(CalculateFeatureSizesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SaveElementSizes_Key = "save_element_sizes";
  static inline constexpr StringLiteral k_GeometryPath_Key = "input_geometry_path";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixPath_Key = "feature_attribute_matrix_path";
  static inline constexpr StringLiteral k_VolumesName_Key = "volumes_name";
  static inline constexpr StringLiteral k_EquivalentDiametersName_Key = "equivalent_diameters_name";
  static inline constexpr StringLiteral k_NumElementsName_Key = "num_elements_name";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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
   * @brief Returns the filter's uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the filter's human name.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the filter's Parameters.
   * @return Parameter
   */
  Parameters parameters() const override;

  /**
   * @brief Creates and returns a copy of the filter.
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, CalculateFeatureSizesFilter, "c666ee17-ca58-4969-80d0-819986c72485");
