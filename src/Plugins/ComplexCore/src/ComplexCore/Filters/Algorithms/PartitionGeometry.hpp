#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

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
