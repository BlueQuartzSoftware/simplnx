#include "ComputeCoordinatesImageGeom.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
class ComputeCoordsImpl
{
public:
  ComputeCoordsImpl(const ImageGeom& imageGeom, Float32AbstractDataStore& coords)
  : m_ImageGeom(imageGeom)
  , m_Coords(coords)
  {
  }
  ~ComputeCoordsImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    usize xPoints = m_ImageGeom.getNumXCells();
    usize yPoints = m_ImageGeom.getNumYCells();
    FloatVec3 spacing = m_ImageGeom.getSpacing();
    FloatVec3 origin = m_ImageGeom.getOrigin();

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
          usize tup = zStride + yStride + k;
          m_Coords[(tup * 3) + 0] = k * spacing[0] + origin[0] + (0.5f * spacing[0]);
          m_Coords[(tup * 3) + 1] = j * spacing[1] + origin[1] + (0.5f * spacing[1]);
          m_Coords[(tup * 3) + 2] = i * spacing[2] + origin[2] + (0.5f * spacing[2]);
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
  const ImageGeom& m_ImageGeom;
  Float32AbstractDataStore& m_Coords;
};

class ComputeIndicesImpl
{
public:
  ComputeIndicesImpl(const ImageGeom& imageGeom, Int32AbstractDataStore& indices)
  : m_ImageGeom(imageGeom)
  , m_Indices(indices)
  {
  }
  ~ComputeIndicesImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    usize xPoints = m_ImageGeom.getNumXCells();
    usize yPoints = m_ImageGeom.getNumYCells();

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
          usize tup = zStride + yStride + k;
          m_Indices[(tup * 3) + 0] = k;
          m_Indices[(tup * 3) + 1] = j;
          m_Indices[(tup * 3) + 2] = i;
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
  const ImageGeom& m_ImageGeom;
  Int32AbstractDataStore& m_Indices;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeCoordinatesImageGeom::ComputeCoordinatesImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         ComputeCoordinatesImageGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeCoordinatesImageGeom::~ComputeCoordinatesImageGeom() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeCoordinatesImageGeom::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Computing Coordinates for Image Geometry...");

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);

  usize zPoints = imageGeom.getNumZCells();

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, zPoints);

  if(m_InputValues->CoordinateOption != to_underlying(ComputeCoordinatesImageGeom::OutputType::Index))
  {
    auto& coords = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CoordArrayPath).getDataStoreRef();
    dataAlg.execute(::ComputeCoordsImpl(imageGeom, coords));
  }
  if(m_InputValues->CoordinateOption != to_underlying(ComputeCoordinatesImageGeom::OutputType::Physical))
  {
    auto& indices = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->IndexArrayPath).getDataStoreRef();
    dataAlg.execute(::ComputeIndicesImpl(imageGeom, indices));
  }

  return {};
}
