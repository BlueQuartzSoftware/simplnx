#include "ComputeCoordinateThreshold.hpp"

#include "ComputeCoordinateThresholdDirect.hpp"
#include "ComputeCoordinateThresholdScanline.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/IntersectionUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
/**
 * @concept GeometryType
 * @brief Restricts node-mask workers to geometry types.
 * @tparam T Specifies the candidate geometry type.
 */
template <typename T>
concept GeometryType = std::is_base_of_v<IGeometry, T>;

/**
 * @class ComputeMaskImpl
 * @brief Writes a coordinate-bound mask for one node geometry type.
 * @tparam GeomT Specifies the input geometry type.
 *
 * The caller disables parallel execution. Generic DataArray and DataStore access
 * has no concurrent-access guarantee.
 */
template <GeometryType GeomT>
class ComputeMaskImpl
{
public:
  /**
   * @brief Initializes a node-geometry mask worker.
   * @param geom Supplies geometry vertices and cells.
   * @param mask Receives one mask value per cell.
   * @param shouldInvert True to reverse mask values.
   * @param isInBoundsFunct Tests each geometry vertex.
   * @pre All arguments outlive the worker execution.
   */
  ComputeMaskImpl(const GeomT& geom, UInt8AbstractDataStore& mask, bool shouldInvert, const std::function<uint8(float32, float32, float32)>& isInBoundsFunct)
  : m_Geom(geom)
  , m_Mask(mask)
  , m_Invert(shouldInvert)
  , m_IsInBoundsFunct(isInBoundsFunct)
  {
  }
  /**
   * @brief Destroys the node-geometry mask worker.
   */
  ~ComputeMaskImpl() = default;

  /**
   * @brief Writes mask values for a cell interval.
   * @param start Identifies the first cell.
   * @param end Identifies one past the last cell.
   * @pre [start, end) is inside the geometry cell range.
   */
  void compute(usize start, usize end) const
  {
    static_assert(std::is_base_of_v<INodeGeometry0D, GeomT> || std::is_base_of_v<INodeGeometry1D, GeomT> || std::is_base_of_v<INodeGeometry2D, GeomT>);

    uint8 trueValue = (m_Invert) ? 0 : 1;
    uint8 falseValue = (m_Invert) ? 1 : 0;

    if constexpr(std::is_same_v<VertexGeom, GeomT>)
    {
      const IGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
      for(usize i = start; i < end; i++)
      {
        float32 xVal = verts[(i * 3) + 0];
        float32 yVal = verts[(i * 3) + 1];
        float32 zVal = verts[(i * 3) + 2];

        if(m_IsInBoundsFunct(xVal, yVal, zVal) == 1)
        {
          m_Mask.setValue(i, trueValue);
        }
        else
        {
          m_Mask.setValue(i, falseValue);
        }
      }
    }
    if constexpr(std::is_same_v<EdgeGeom, GeomT>)
    {
      const IGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
      const IGeometry::SharedEdgeList::store_type& edges = m_Geom.getEdgesRef().getDataStoreRef();
      usize numComp = edges.getNumberOfComponents();
      for(usize i = start; i < end; i++)
      {
        uint8 hits = 0;
        for(usize comp = 0; comp < numComp; comp++)
        {
          const IGeometry::SharedFaceList::value_type activeVertIndex = edges.getValue((i * numComp) + comp);
          float32 xVal = verts.getValue((activeVertIndex * 3) + 0);
          float32 yVal = verts.getValue((activeVertIndex * 3) + 1);
          float32 zVal = verts.getValue((activeVertIndex * 3) + 2);

          hits += m_IsInBoundsFunct(xVal, yVal, zVal);
        }
        if(hits == numComp)
        {
          m_Mask.setValue(i, trueValue);
        }
        else
        {
          m_Mask.setValue(i, falseValue);
        }
      }
    }
    if constexpr(std::is_same_v<TriangleGeom, GeomT> || std::is_same_v<QuadGeom, GeomT>)
    {
      const IGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
      const IGeometry::SharedFaceList::store_type& faces = m_Geom.getFacesRef().getDataStoreRef();
      usize numComp = faces.getNumberOfComponents();
      for(usize i = start; i < end; i++)
      {
        uint8 hits = 0;
        for(usize comp = 0; comp < numComp; comp++)
        {
          const IGeometry::SharedFaceList::value_type activeVertIndex = faces.getValue((i * numComp) + comp);
          float32 xVal = verts.getValue((activeVertIndex * 3) + 0);
          float32 yVal = verts.getValue((activeVertIndex * 3) + 1);
          float32 zVal = verts.getValue((activeVertIndex * 3) + 2);

          hits += m_IsInBoundsFunct(xVal, yVal, zVal);
        }
        if(hits == numComp)
        {
          m_Mask.setValue(i, trueValue);
        }
        else
        {
          m_Mask.setValue(i, falseValue);
        }
      }
    }
  }

  /**
   * @brief Writes mask values for an assigned cell range.
   * @param range Identifies the cell interval.
   */
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const GeomT& m_Geom;
  UInt8AbstractDataStore& m_Mask;
  bool m_Invert;
  const std::function<uint8(float32, float32, float32)>& m_IsInBoundsFunct;
};

/**
 * @brief Creates a coordinate-bound mask for a node geometry.
 * @param geom Supplies the node geometry.
 * @param mask Receives one value per geometry cell.
 * @param shouldInvert True to reverse mask values.
 * @param isInBoundsFunct Tests each geometry vertex.
 * @return Success, or an unsupported-geometry error.
 *
 * The execution remains serial because generic DataArray and DataStore access
 * has no concurrent-access guarantee.
 */
Result<> ExecuteNodeMask(const IGeometry& geom, UInt8AbstractDataStore& mask, bool shouldInvert, const std::function<uint8(float32, float32, float32)>& isInBoundsFunct)
{
  ParallelDataAlgorithm dataAlg;
  // Generic geometry and mask stores remain serial.
  dataAlg.setParallelizationEnabled(false);
  switch(geom.getGeomType())
  {
  case IGeometry::Type::Triangle: {
    dataAlg.setRange(0, geom.getNumberOfCells());
    dataAlg.execute(ComputeMaskImpl<TriangleGeom>(dynamic_cast<const TriangleGeom&>(geom), mask, shouldInvert, isInBoundsFunct));
    break;
  }
  case IGeometry::Type::Vertex: {
    dataAlg.setRange(0, geom.getNumberOfCells());
    dataAlg.execute(ComputeMaskImpl<VertexGeom>(dynamic_cast<const VertexGeom&>(geom), mask, shouldInvert, isInBoundsFunct));
    break;
  }
  case IGeometry::Type::Edge: {
    dataAlg.setRange(0, geom.getNumberOfCells());
    dataAlg.execute(ComputeMaskImpl<EdgeGeom>(dynamic_cast<const EdgeGeom&>(geom), mask, shouldInvert, isInBoundsFunct));
    break;
  }
  case IGeometry::Type::Quad: {
    dataAlg.setRange(0, geom.getNumberOfCells());
    dataAlg.execute(ComputeMaskImpl<QuadGeom>(dynamic_cast<const QuadGeom&>(geom), mask, shouldInvert, isInBoundsFunct));
    break;
  }
  default: {
    return MakeErrorResult(-89472, fmt::format("Input geometry must be of type(s) [ Image, Triangle, Vertex, Edge, Quad ]. Supplied geometry name {}", geom.getName()));
  }
  }
  return {};
}

/**
 * @brief Tests whether bounds can intersect a non-image geometry.
 * @param geom Supplies geometry bounds.
 * @param inputValues Supplies selected coordinate bounds.
 * @return True if geometry bounds can intersect selected bounds.
 * @pre inputValues is not null.
 *
 * A false result lets the caller fill the mask without visiting every cell.
 */
bool PrecheckRuntimeGeom(const IGeometry& geom, const ComputeCoordinateThresholdInputValues* inputValues)
{
  if(geom.getGeomType() == IGeometry::Type::Image)
  {
    return true;
  }

  const auto& iNodeGeom = dynamic_cast<const INodeGeometry0D&>(geom);

  BoundingBox3Df bounds = iNodeGeom.getBoundingBox();
  std::array<float32, 3> minPoint = bounds.getMinPoint().toArray();
  std::array<float32, 3> maxPoint = bounds.getMaxPoint().toArray();

  switch(static_cast<ComputeCoordinateThreshold::BoundsType>(inputValues->ShapeType))
  {
  case ComputeCoordinateThreshold::BoundsType::Rectangle: {
    VectorFloat32Parameter::ValueType minBound = inputValues->MinCoord;
    VectorFloat32Parameter::ValueType maxBound = inputValues->MaxCoord;

    if(maxPoint[0] < minBound[0] || maxPoint[1] < minBound[1] || maxPoint[2] < minBound[2])
    {
      return false;
    }

    if(minPoint[0] > maxBound[0] || minPoint[1] > maxBound[1] || minPoint[2] > maxBound[2])
    {
      return false;
    }

    return true;
  }
  case ComputeCoordinateThreshold::BoundsType::Sphere: {
    VectorFloat32Parameter::ValueType sphereInfo = inputValues->SphereInfo;
    return IntersectionUtilities::SphereIntersectsRectangularPrism({sphereInfo[0], sphereInfo[1], sphereInfo[2]}, sphereInfo[3], minPoint, maxPoint);
  }
  }

  return true;
}
} // namespace

ComputeCoordinateThreshold::ComputeCoordinateThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ComputeCoordinateThresholdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeCoordinateThreshold::~ComputeCoordinateThreshold() noexcept = default;

Result<> ComputeCoordinateThreshold::operator()()
{
  const auto& geom = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->GeometryPath);
  auto& maskArray = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->MaskArrayPath);
  auto& mask = maskArray.getDataStoreRef();

  if(!PrecheckRuntimeGeom(geom, m_InputValues))
  {
    if(m_InputValues->Invert)
    {
      mask.fill(1);
    }
    else
    {
      mask.fill(0);
    }

    return MakeWarningVoidResult(-24715, "The input geometry did not contain any points within the supplied coordinate bounds, all values in the mask are the same.");
  }

  if(geom.getGeomType() == IGeometry::Type::Image)
  {
    return DispatchAlgorithm<ComputeCoordinateThresholdDirect, ComputeCoordinateThresholdScanline>({&maskArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
  }

  std::function<uint8(float32, float32, float32)> isInBounds;
  switch(static_cast<BoundsType>(m_InputValues->ShapeType))
  {
  case BoundsType::Rectangle: {
    const VectorFloat32Parameter::ValueType minPoint = m_InputValues->MinCoord;
    const VectorFloat32Parameter::ValueType maxPoint = m_InputValues->MaxCoord;
    isInBounds = [minPoint, maxPoint](float32 x, float32 y, float32 z) -> uint8 {
      if(minPoint[0] > x || maxPoint[0] < x)
      {
        return 0;
      }

      if(minPoint[1] > y || maxPoint[1] < y)
      {
        return 0;
      }

      if(minPoint[2] > z || maxPoint[2] < z)
      {
        return 0;
      }

      return 1;
    };
    break;
  }
  case BoundsType::Sphere: {
    const VectorFloat32Parameter::ValueType sphereInfo = m_InputValues->SphereInfo;
    isInBounds = [sphereInfo](float32 x, float32 y, float32 z) -> uint8 {
      const float32 xDiff = x - sphereInfo[0];
      const float32 yDiff = y - sphereInfo[1];
      const float32 zDiff = z - sphereInfo[2];
      const float32 totalDiff = (xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff);
      return totalDiff > (sphereInfo[3] * sphereInfo[3]) ? 0 : 1;
    };
  }
  }

  return ExecuteNodeMask(geom, mask, m_InputValues->Invert, isInBounds);
}
