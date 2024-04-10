#pragma once

#include "Algorithms/PartitionGeometry.hpp"

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class PartitionGeometryFilter
 * @brief This filter splits a given geometry into partitions using an image geometry as the partitioning scheme.
 *
 * There are three available schemes to choose from:
 * 1. Basic - Just provide the starting ID and the number of partitions per axis, and the filter will automatically
 * create a partitioning scheme that fits your geometry data.
 *
 * 2. Advanced - Provide the starting ID, out-of-bounds ID, number of partitions per axis, partitioning scheme origin,
 * and length per partition, and the filter will create a partitioning scheme using these inputs.  It IS possible,
 * using this mode, to create a partitioning scheme that does not completely fit the input geometry.  In this case,
 * the out-of-bounds value is used to label those particular cells/vertices.
 *
 * 3. Bounding Box - Provide the starting ID, number of partitions per axis, lower left coordinate (ll), and upper
 * right coordinate (ur), and the filter will create a partitioning scheme that has bounds [ll, ur).  It IS
 * possible, using this mode, to create a partitioning scheme that does not completely fit the input geometry.
 * In this case, the out-of-bounds value is used to label those particular cells/vertices.
 */
class SIMPLNXCORE_EXPORT PartitionGeometryFilter : public IFilter
{
public:
  PartitionGeometryFilter() = default;
  ~PartitionGeometryFilter() noexcept override = default;

  PartitionGeometryFilter(const PartitionGeometryFilter&) = delete;
  PartitionGeometryFilter(PartitionGeometryFilter&&) noexcept = delete;

  PartitionGeometryFilter& operator=(const PartitionGeometryFilter&) = delete;
  PartitionGeometryFilter& operator=(PartitionGeometryFilter&&) noexcept = delete;

  enum class PartitioningMode
  {
    Basic = 0,
    Advanced = 1,
    BoundingBox = 2,
    ExistingPartitionGrid = 3
  };

  // Parameter Keys
  static inline constexpr StringLiteral k_PartitioningMode_Key = "partitioning_mode";
  static inline constexpr StringLiteral k_StartingFeatureID_Key = "starting_partition_id";
  static inline constexpr StringLiteral k_OutOfBoundsFeatureID_Key = "out_of_bounds_value";
  static inline constexpr StringLiteral k_NumberOfCellsPerAxis_Key = "number_of_partitions_per_axis";
  static inline constexpr StringLiteral k_PartitionGridOrigin_Key = "partitioning_scheme_origin";
  static inline constexpr StringLiteral k_CellLength_Key = "length_per_partition";
  static inline constexpr StringLiteral k_MinGridCoord_Key = "lower_left_coord";
  static inline constexpr StringLiteral k_MaxGridCoord_Key = "upper_right_coord";
  static inline constexpr StringLiteral k_InputGeometryCellAttributeMatrixPath_Key = "selected_geometry_attribute_matrix_path";
  static inline constexpr StringLiteral k_PartitionGridGeometry_Key = "created_image_geometry_path";
  static inline constexpr StringLiteral k_PartitionGridCellAMName_Key = "created_attribute_matrix_name";
  static inline constexpr StringLiteral k_PartitionGridFeatureIDsName_Key = "created_feature_ids_name";
  static inline constexpr StringLiteral k_InputGeometryToPartition_Key = "selected_geometry_path";
  static inline constexpr StringLiteral k_ExistingPartitionGridPath_Key = "existing_partitioning_scheme_path";
  static inline constexpr StringLiteral k_UseVertexMask_Key = "use_vertex_mask";
  static inline constexpr StringLiteral k_VertexMaskPath_Key = "vertex_mask_path";
  static inline constexpr StringLiteral k_FeatureAttrMatrixName_Key = "feature_attr_matrix_name";
  static inline constexpr StringLiteral k_PartitionIdsArrayName_Key = "partition_ids_array_name";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;

  template <typename GeomType>
  Result<> dataCheckPartitioningMode(const DataStructure& dataStructure, const Arguments& filterArgs, const GeomType& geometryToPartition) const;

  template <typename GeomType>
  Result<> dataCheckBasicMode(const SizeVec3& numOfPartitionsPerAxis, const GeomType& geometryToPartition, const AttributeMatrix& attrMatrix) const;

  template <typename GeomType>
  Result<> dataCheckAdvancedMode(const SizeVec3& numOfPartitionsPerAxis, const FloatVec3& lengthPerPartition, const GeomType& geometryToPartition, const AttributeMatrix& attrMatrix) const;

  template <typename GeomType>
  Result<> dataCheckBoundingBoxMode(const SizeVec3& numOfPartitionsPerAxis, const FloatVec3& llCoord, const FloatVec3& urCoord, const GeomType& geometryToPartition,
                                    const AttributeMatrix& attrMatrix) const;

  static Result<> DataCheckExistingGeometryMode();

  template <typename GeomType>
  Result<> dataCheckPartitioningScheme(const GeomType& geometryToPartition, const AttributeMatrix& attrMatrix) const;

  /**
   * @brief Helper method that data checks the Number Of Partitions Per Axis variable.
   */
  static Result<> DataCheckNumberOfPartitions(const SizeVec3& numberOfPartitionsPerAxis);

  static Result<> DataCheckDimensionality(const INodeGeometry0D& geometry);

  Result<PartitionGeometry::PSGeomInfo> generateNodeBasedPSInfo(const DataStructure& dataStructure, const Arguments& filterArgs, const DataPath& geometryToPartitionPath,
                                                                const DataPath& attrMatrixPath) const;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, PartitionGeometryFilter, "aad29ebc-bf1c-5dd3-ad25-0f7f8907a02d");
/* LEGACY UUID FOR THIS FILTER {aad29ebc-bf1c-5dd3-ad25-0f7f8907a02d} */
