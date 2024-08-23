#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT PartitionGeometryInputValues
{
  ChoicesParameter::ValueType PartitioningMode;
  int32 StartingFeatureID;
  int32 OutOfBoundsFeatureID;
  VectorInt32Parameter::ValueType NumberOfCellsPerAxis;
  VectorFloat32Parameter::ValueType PartitionGridOrigin;
  VectorFloat32Parameter::ValueType CellLength;
  VectorFloat32Parameter::ValueType MinGridCoord;
  VectorFloat32Parameter::ValueType MaxGridCoord;
  DataPath InputGeomCellAMPath;
  DataPath PartitionGridGeomPath;
  std::string PartitionGridCellAMName;
  std::string PartitionGridFeatureIDsArrayName;
  DataPath InputGeometryToPartition;
  std::string PartitionIdsArrayName;
  DataPath ExistingPartitionGridPath;
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

class SIMPLNXCORE_EXPORT PartitionGeometry
{
public:
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

  PartitionGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, PartitionGeometryInputValues* inputValues);
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
   * @param partitionIdsStore The partition ids array that stores the results.
   * @param psImageGeom The partitioning scheme image geometry that is used
   * to partition the cell-based input geometry.
   * @param outOfBoundsValue Value that out-of-bounds cells will be
   * labeled with
   * @return The result of the partitioning algorithm.  Valid if successful, invalid
   * if there was an error.
   */
  Result<> partitionCellBasedGeometry(const IGridGeometry& inputGeometry, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue);

  /**
   * @brief Partitions a vertex list (typically from a node-based geometry) according to
   * the partitioning scheme geometry provided, and stores the partition ids in the
   * partitionIds array.
   *
   * If a given vertex is outside the partitioning scheme bounds and an out of bounds value
   * is provided, the vertex will be labeled with the out of bounds value.  Otherwise,
   * the function will return an invalid Result with an error message.
   *
   * @param vertexListStore The list of vertices from the node-based geometry
   * @param partitionIdsStore The partition ids array that stores the results.
   * @param psImageGeom The partitioning scheme image geometry that is used
   * to partition the vertex list.
   * @param outOfBoundsValue Value that out-of-bounds vertices will be
   * labeled with
   * @param maskArrayOpt Optional mask array
   * @return The result of the partitioning algorithm.  Valid if successful, invalid
   * if there was an error.
   */
  Result<> partitionNodeBasedGeometry(const VertexStore& vertexListStore, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue,
                                      const std::optional<const BoolArray>& maskArrayOpt);
};

} // namespace nx::core
