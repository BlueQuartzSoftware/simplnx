#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "ComplexCore/utils/PSInfoGenerator.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT PartitionGeometryInputValues
{
  ChoicesParameter::ValueType PartitioningMode;
  int32 StartingPartitionID;
  int32 OutOfBoundsValue;
  VectorInt32Parameter::ValueType NumberOfPartitionsPerAxis;
  VectorFloat32Parameter::ValueType PartitioningSchemeOrigin;
  VectorFloat32Parameter::ValueType LengthPerPartition;
  VectorFloat32Parameter::ValueType LowerLeftCoord;
  VectorFloat32Parameter::ValueType UpperRightCoord;
  DataPath AttributeMatrixPath;
  DataPath PSGeometryPath;
  std::string PSGeometryAMName;
  std::string PSGeometryDataArrayName;
  DataPath GeometryToPartition;
  std::string PartitionIdsArrayName;
  DataPath ExistingPartitioningSchemePath;
  bool UseVertexMask;
  DataPath VertexMaskPath;
  std::string FeatureAttrMatrixName;
};

/**
 * @class PartitionGeometry
 * @brief This algorithm generates the image geometry information necessary to
 * split a given geometry into partitions, using one of the three partitioning modes.
 *
 * In the Basic partitioning mode, it calculates the dimensions, origin, and spacing
 * of the partitioning scheme using the input geometry and the number of partitions per axis.
 *
 * In the Advanced mode, it uses the number of partitions per axis, partitioning scheme
 * origin, and length per partition inputs as the dimensions, origin, and spacing, respectively.
 *
 * In the Bounding Box mode, it calculates the dimensions, origin, and spacing of the
 * partitioning scheme using the input geometry, the number of partitions per axis, and
 * a set of bounding box points.
 */

class COMPLEXCORE_EXPORT PartitionGeometry
{
public:
  PartitionGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PartitionGeometryInputValues* inputValues);
  ~PartitionGeometry() noexcept;

  PartitionGeometry(const PartitionGeometry&) = delete;
  PartitionGeometry(PartitionGeometry&&) noexcept = delete;
  PartitionGeometry& operator=(const PartitionGeometry&) = delete;
  PartitionGeometry& operator=(PartitionGeometry&&) noexcept = delete;

  struct PSGeomInfo
  {
    USizeVec3 geometryDims;
    std::optional<FloatVec3> geometryOrigin;
    std::optional<FloatVec3> geometrySpacing;
    IGeometry::LengthUnit geometryUnits;
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Generates the partitioning scheme information (dimensions, origin, spacing, units)
   * that the filter will use to create the partitioning scheme image geometry.
   * @param psInfoGen The partitioning scheme information generator class that is responsible
   * for generating the dimensions, origin, spacing, and units based on the input geometry type.
   * @param filterArgs The arguments from the filter, some filter variables are needed.
   * @return Returns a result that either contains a PSGeomInfo object with the data inside,
   * or an error describing what went wrong during the generation process.
   */
  static Result<PSGeomInfo> GeneratePartitioningSchemeInfo(const PSInfoGenerator& psInfoGen, const DataStructure& ds, const Arguments& filterArgs);

  /**
   * @brief Generates the display text that describes the input geometry,
   * shown as a preflight updated value in the user interface.
   * @param dims The dimensions of the input geometry
   * @param origin The origin of the input geometry
   * @param spacing The spacing of the input geometry
   * @param lengthUnits The length units of the input geometry
   * @return The text description of the current input geometry.
   */
  static std::string GenerateInputGeometryDisplayText(const SizeVec3& dims, const FloatVec3& origin, const FloatVec3& spacing, const IGeometry::LengthUnit& lengthUnits);

  /**
   * @brief Generates the display text that describes the partitioning scheme
   * geometry, shown as a preflight updated value in the user interface.
   * @param psDims The dimensions of the partitioning scheme geometry
   * @param psOrigin The origin of the partitioning scheme geometry
   * @param psSpacing The spacing of the partitioning scheme geometry
   * @param lengthUnits The length units of the partitioning scheme geometry
   * @param iGeom The input geometry, used to determine if the
   * partitioning scheme geometry fits the input geometry or not.
   * @return The text description of the partitioning scheme geometry.
   */
  static std::string GeneratePartitioningSchemeDisplayText(const SizeVec3& psDims, const FloatVec3& psOrigin, const FloatVec3& psSpacing, const IGeometry::LengthUnit& lengthUnits,
                                                           const IGeometry& iGeom);

private:
  DataStructure& m_DataStructure;
  const PartitionGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  /**
   * @brief Partitions a cell-based input geometry according to the partitioning
   * scheme geometry provided, and stores the partition ids in the partitionIds array.
   *
   * If a given cell is outside the partitioning scheme bounds and an out of bounds value
   * is provided, the cell will be labeled with the out of bounds value.  Otherwise,
   * the function will return an invalid Result with an error message.
   *
   * @param inputGeometry The cell-based input geometry that is being partitioned.
   * @param partitionIds The partition ids array that stores the results.
   * @param psImageGeom The partitioning scheme image geometry that is used
   * to partition the cell-based input geometry.
   * @param outOfBoundsValue Value that out-of-bounds cells will be
   * labeled with
   * @return The result of the partitioning algorithm.  Valid if successful, invalid
   * if there was an error.
   */
  Result<> partitionCellBasedGeometry(const IGridGeometry& inputGeometry, Int32Array& partitionIds, const ImageGeom& psImageGeom, int outOfBoundsValue);

  /**
   * @brief Partitions a vertex list (typically from a node-based geometry) according to
   * the partitioning scheme geometry provided, and stores the partition ids in the
   * partitionIds array.
   *
   * If a given vertex is outside the partitioning scheme bounds and an out of bounds value
   * is provided, the vertex will be labeled with the out of bounds value.  Otherwise,
   * the function will return an invalid Result with an error message.
   *
   * @param geomName The name of the node-based input geometry
   * @param vertexList The list of vertices from the node-based geometry
   * @param partitionIds The partition ids array that stores the results.
   * @param psImageGeom The partitioning scheme image geometry that is used
   * to partition the vertex list.
   * @param outOfBoundsValue Value that out-of-bounds vertices will be
   * labeled with
   * @param maskArrayOpt Optional mask array
   * @return The result of the partitioning algorithm.  Valid if successful, invalid
   * if there was an error.
   */
  Result<> partitionNodeBasedGeometry(const std::string& geomName, const IGeometry::SharedVertexList& vertexList, Int32Array& partitionIds, const ImageGeom& psImageGeom, int outOfBoundsValue,
                                      const std::optional<const BoolArray>& maskArrayOpt);
};

} // namespace complex
