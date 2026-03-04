#include "ComputeCoordinateThreshold.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/AbstractGeometry.hpp"
#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/IntersectionUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
template <typename T>
concept GeometryType = std::is_base_of_v<AbstractGeometry, T>;

template <GeometryType GeomT>
class ComputeMaskImpl
{
public:
  ComputeMaskImpl(const GeomT& geom, UInt8AbstractDataStore& mask, bool shouldInvert, const std::function<uint8(float32, float32, float32)>& isInBoundsFunct)
  : m_Geom(geom)
  , m_Mask(mask)
  , m_Invert(shouldInvert)
  , m_IsInBoundsFunct(isInBoundsFunct)
  {
  }
  ~ComputeMaskImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    static_assert(std::is_same_v<ImageGeom, GeomT> || std::is_base_of_v<AbstractNodeGeometry0D, GeomT> || std::is_base_of_v<AbstractNodeGeometry1D, GeomT> ||
                  std::is_base_of_v<AbstractNodeGeometry2D, GeomT>);

    uint8 trueValue = (m_Invert) ? 0 : 1;
    uint8 falseValue = (m_Invert) ? 1 : 0;

    if constexpr(std::is_same_v<ImageGeom, GeomT>)
    {
      usize xPoints = m_Geom.getNumXCells();
      usize yPoints = m_Geom.getNumYCells();
      FloatVec3 spacing = m_Geom.getSpacing();
      FloatVec3 origin = m_Geom.getOrigin();

      usize zStride = 0, yStride = 0;
      for(usize i = start; i < end; i++)
      {
        zStride = i * xPoints * yPoints;
        for(usize j = 0; j < yPoints; j++)
        {
          yStride = j * xPoints;
          for(usize k = 0; k < xPoints; k++)
          {
            // We are inlining the calculations here to leverage the speed of primitives (no Point object or vector from the API)
            float32 minXVal = k * spacing[0] + origin[0];
            float32 minYVal = j * spacing[1] + origin[1];
            float32 minZVal = i * spacing[2] + origin[2];

            float32 maxXVal = k * spacing[0] + origin[0] + spacing[0];
            float32 maxYVal = j * spacing[1] + origin[1] + spacing[1];
            float32 maxZVal = i * spacing[2] + origin[2] + spacing[2];

            // Check every vertex for spherical and other potential thresholds
            uint8 inBoundsVertexCount = 0;
            inBoundsVertexCount += m_IsInBoundsFunct(minXVal, minYVal, minZVal);
            inBoundsVertexCount += m_IsInBoundsFunct(maxXVal, minYVal, minZVal);
            inBoundsVertexCount += m_IsInBoundsFunct(minXVal, maxYVal, minZVal);
            inBoundsVertexCount += m_IsInBoundsFunct(minXVal, minYVal, maxZVal);
            inBoundsVertexCount += m_IsInBoundsFunct(maxXVal, maxYVal, maxZVal);
            inBoundsVertexCount += m_IsInBoundsFunct(minXVal, maxYVal, maxZVal);
            inBoundsVertexCount += m_IsInBoundsFunct(maxXVal, minYVal, maxZVal);
            inBoundsVertexCount += m_IsInBoundsFunct(maxXVal, maxYVal, minZVal);

            usize tup = zStride + yStride + k;
            if(inBoundsVertexCount == 8)
            {
              m_Mask.setValue(tup, trueValue);
            }
            else
            {
              m_Mask.setValue(tup, falseValue);
            }
          }
        }
      }
    }
    if constexpr(std::is_same_v<VertexGeom, GeomT>)
    {
      const AbstractGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
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
      const AbstractGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
      const AbstractGeometry::SharedEdgeList::store_type& edges = m_Geom.getEdgesRef().getDataStoreRef();
      usize numComp = edges.getNumberOfComponents();
      for(usize i = start; i < end; i++)
      {
        uint8 hits = 0;
        for(usize comp = 0; comp < numComp; comp++)
        {
          const AbstractGeometry::SharedFaceList::value_type activeVertIndex = edges.getValue((i * numComp) + comp);
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
      const AbstractGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
      const AbstractGeometry::SharedFaceList::store_type& faces = m_Geom.getFacesRef().getDataStoreRef();
      usize numComp = faces.getNumberOfComponents();
      for(usize i = start; i < end; i++)
      {
        uint8 hits = 0;
        for(usize comp = 0; comp < numComp; comp++)
        {
          const AbstractGeometry::SharedFaceList::value_type activeVertIndex = faces.getValue((i * numComp) + comp);
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

  // -----------------------------------------------------------------------------
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

Result<> ExecuteComputeMask(const AbstractGeometry& geom, UInt8AbstractDataStore& mask, bool shouldInvert, const std::function<uint8(float32, float32, float32)>& isInBoundsFunct)
{
  ParallelDataAlgorithm dataAlg;
  dataAlg.setParallelizationEnabled(false);
  switch(geom.getGeomType())
  {
  case IGeometry::Type::Image: {
    const auto& image = dynamic_cast<const ImageGeom&>(geom);
    dataAlg.setRange(0, image.getNumZCells());
    dataAlg.execute(ComputeMaskImpl<ImageGeom>(image, mask, shouldInvert, isInBoundsFunct));
    break;
  }
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

bool PrecheckRuntimeGeom(const AbstractGeometry& geom, const ComputeCoordinateThresholdInputValues* inputValues)
{
  if(geom.getGeomType() == IGeometry::Type::Image)
  {
    return true;
  }

  const auto& iNodeGeom = dynamic_cast<const AbstractNodeGeometry0D&>(geom);

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

// -----------------------------------------------------------------------------
ComputeCoordinateThreshold::ComputeCoordinateThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ComputeCoordinateThresholdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeCoordinateThreshold::~ComputeCoordinateThreshold() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeCoordinateThreshold::operator()()
{
  std::function<uint8(float32, float32, float32)> f_IsInBounds;
  switch(static_cast<BoundsType>(m_InputValues->ShapeType))
  {
  case BoundsType::Rectangle: {
    VectorFloat32Parameter::ValueType minPoint = m_InputValues->MinCoord;
    VectorFloat32Parameter::ValueType maxPoint = m_InputValues->MaxCoord;
    f_IsInBounds = [minPoint, maxPoint](float32 x, float32 y, float32 z) -> uint8 {
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
    VectorFloat32Parameter::ValueType sphereInfo = m_InputValues->SphereInfo;
    f_IsInBounds = [sphereInfo](float32 x, float32 y, float32 z) -> uint8 {
      float32 xDiff = x - sphereInfo[0];
      float32 yDiff = y - sphereInfo[1];
      float32 zDiff = z - sphereInfo[2];

      // Do not switch to pow() inlined is faster for square case for floating point num
      float32 tDiff = (xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff);

      if(tDiff > (sphereInfo[3] * sphereInfo[3]))
      {
        return 0;
      }

      return 1;
    };
  }
  }

  const auto& geom = m_DataStructure.getDataRefAs<AbstractGeometry>(m_InputValues->GeometryPath);
  auto& mask = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->MaskArrayPath).getDataStoreRef();

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

  ExecuteComputeMask(geom, mask, m_InputValues->Invert, f_IsInBounds);

  return {};
}
