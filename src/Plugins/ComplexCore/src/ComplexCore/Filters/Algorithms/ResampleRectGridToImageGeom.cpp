#include "ResampleRectGridToImageGeom.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ResampleRectGridToImageGeom::ResampleRectGridToImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         ResampleRectGridToImageGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ResampleRectGridToImageGeom::~ResampleRectGridToImageGeom() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ResampleRectGridToImageGeom::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ResampleRectGridToImageGeom::operator()()
{
  const auto& rectGridGeom = m_DataStructure.getDataRefAs<RectGridGeom>(m_InputValues->RectilinearGridPath);
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  imageGeom.setUnits(rectGridGeom.getUnits());

  auto originResults = rectGridGeom.getOrigin();
  if(originResults.invalid())
  {
    return MakeErrorResult(-7367, fmt::format("Could not retrieve origin value from input rectilinear grid geometry at path {}", m_InputValues->RectilinearGridPath.toString()));
  }
  const Float32Array* xGridValues = rectGridGeom.getXBounds();
  const Float32Array* yGridValues = rectGridGeom.getYBounds();
  const Float32Array* zGridValues = rectGridGeom.getZBounds();
  FloatVec3 origin = rectGridGeom.getOrigin().value();
  const SizeVec3 rectGridDims = rectGridGeom.getDimensions();
  const SizeVec3 imageGeomDims = imageGeom.getDimensions();
  const std::vector<float32> max = {xGridValues->back(), yGridValues->back(), zGridValues->back()};
  const std::vector<float32> imageGeomSpacing = {(max[0] - origin[0]) / imageGeomDims[0], (max[1] - origin[1]) / imageGeomDims[1], (max[2] - origin[2]) / imageGeomDims[2]};
  const FloatVec3 halfSpacing = {imageGeomSpacing[0] * 0.5f, imageGeomSpacing[1] * 0.5f, imageGeomSpacing[2] * 0.5f};

  imageGeom.setSpacing(imageGeomSpacing);
  imageGeom.setOrigin(origin);

  usize rgIdxStart = 1;
  std::vector<usize> xIdx(imageGeomDims[0], 0);
  for(usize x = 0; x < imageGeomDims[0]; x++)
  {
    float32 coord = origin[0] + (x * imageGeomSpacing[0]) + halfSpacing[0];
    for(usize rgIdx = rgIdxStart; rgIdx < xGridValues->size(); rgIdx++)
    {
      if(coord > xGridValues->at(rgIdx - 1) && coord <= xGridValues->at(rgIdx))
      {
        xIdx[x] = rgIdx - 1;
        rgIdxStart = rgIdx;
        break;
      }
    }
  }

  rgIdxStart = 1;
  std::vector<usize> yIdx(imageGeomDims[1], 0);
  for(usize y = 0; y < imageGeomDims[1]; y++)
  {
    float32 coord = origin[1] + (y * imageGeomSpacing[1]) + halfSpacing[1];
    for(usize rgIdx = rgIdxStart; rgIdx < yGridValues->size(); rgIdx++)
    {
      if(coord > yGridValues->at(rgIdx - 1) && coord <= yGridValues->at(rgIdx))
      {
        yIdx[y] = rgIdx - 1;
        rgIdxStart = rgIdx;
        break;
      }
    }
  }

  rgIdxStart = 1;
  std::vector<usize> zIdx(imageGeomDims[2], 0);
  for(usize z = 0; z < imageGeomDims[2]; z++)
  {
    float32 coord = origin[2] + (z * imageGeomSpacing[2]) + halfSpacing[2];
    for(usize rgIdx = rgIdxStart; rgIdx < zGridValues->size(); rgIdx++)
    {
      if(coord > zGridValues->at(rgIdx - 1) && coord <= zGridValues->at(rgIdx))
      {
        zIdx[z] = rgIdx - 1;
        rgIdxStart = rgIdx;
        break;
      }
    }
  }

  // Store the mapped XYZ index into the RectGrid data ararys
  std::vector<int64> newToOldIdxs(imageGeom.getNumberOfCells());
  usize currIdx = 0;
  for(const usize z : zIdx)
  {
    for(const usize y : yIdx)
    {
      for(const usize x : xIdx)
      {
        // Compute the index into the RectGrid Data Array
        const usize idx = (rectGridDims[0] * rectGridDims[1] * z) + (rectGridDims[0] * y) + x;
        // Compute the index into the new Idx Array
        newToOldIdxs[currIdx++] = idx;
      }
    }
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  ParallelTaskAlgorithm taskRunner;
  auto& destCellDataAM = imageGeom.getCellDataRef();

  // copy over/resample the cell data
  for(const auto& srcArrayPath : m_InputValues->SelectedDataArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto& srcArray = m_DataStructure.getDataRefAs<IArray>(srcArrayPath);
    const std::string srcName = srcArray.getName();
    auto& destDataArray = dynamic_cast<IArray&>(destCellDataAM.at(srcName));
    m_MessageHandler(fmt::format("Resample Rect Grid To Image Geom || Copying Data Array {}", srcName));

    CopyFromArray::RunParallelCopyUsingIndexList(destDataArray, taskRunner, srcArray, newToOldIdxs);
  }

  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return {};
}
