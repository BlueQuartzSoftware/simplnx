#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindSizes
 * @brief
 */
class COMPLEXCORE_EXPORT FindSizes : public IFilter
{
public:
  FindSizes() = default;
  ~FindSizes() noexcept override = default;

  FindSizes(const FindSizes&) = delete;
  FindSizes(FindSizes&&) noexcept = delete;

  FindSizes& operator=(const FindSizes&) = delete;
  FindSizes& operator=(FindSizes&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SaveElementSizes_Key = "save_element_sizes";
  static inline constexpr StringLiteral k_GeometryPath_Key = "geometry_path";
  static inline constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_VolumesPath_Key = "volumes_path";
  static inline constexpr StringLiteral k_EquivalentDiametersPath_Key = "equivalent_diameters_path";
  static inline constexpr StringLiteral k_NumElementsPath_Key = "num_elements_path";

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
   * @brief findSizes Determines the size of each Feature independent of geometry
   * @param dataStructure
   * @param args
   * @return Return<>
   */
  Result<> findSizes(DataStructure& data, const Arguments& args) const;

  /**
   * @brief findSizesImage Specialization for determining Feature sizes on ImageGeometries
   * @param dataStructure
   * @param args
   * @param image
   * @return Return<>
   */
  Result<> findSizesImage(DataStructure& data, const Arguments& args, ImageGeom* image) const;

  /**
   * @brief findSizesUnstructured Determines the size of each Feature for non-ImageGeometries
   * @param dataStructure
   * @param args
   * @param igeom
   * @return Return<>
   */
  Result<> findSizesUnstructured(DataStructure& data, const Arguments& args, AbstractGeometry* igeom) const;

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindSizes, "30e6f5e9-7949-4c14-aaef-0a68c93c1fd5");
