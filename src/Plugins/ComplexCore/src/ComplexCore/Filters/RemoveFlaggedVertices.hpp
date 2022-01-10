#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class RemoveFlaggedVertices
 * @brief This filter will remove specified vertices from the specified geometry.
 */
class COMPLEXCORE_EXPORT RemoveFlaggedVertices : public IFilter
{
public:
  RemoveFlaggedVertices() = default;
  ~RemoveFlaggedVertices() noexcept override = default;

  RemoveFlaggedVertices(const RemoveFlaggedVertices&) = delete;
  RemoveFlaggedVertices(RemoveFlaggedVertices&&) noexcept = delete;

  RemoveFlaggedVertices& operator=(const RemoveFlaggedVertices&) = delete;
  RemoveFlaggedVertices& operator=(RemoveFlaggedVertices&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_VertexGeomPath_Key = "vertex_geom";
  static inline constexpr StringLiteral k_MaskPath_Key = "mask";
  static inline constexpr StringLiteral k_ArraySelection_Key = "target_arrays";
  static inline constexpr StringLiteral k_ReducedVertexPath_Key = "reduced_vertex";

  /**
   * @brief Returns the name of the filter.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return std::vector<std::string>
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, RemoveFlaggedVertices, "379ccc67-16dd-530a-984f-177db2314bac");
