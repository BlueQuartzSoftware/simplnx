#include "ResampleRectGridToImageGeom.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

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

  imageGeom.setSpacing(imageGeomSpacing);
  imageGeom.setOrigin(origin);

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

    CopyFromArray::RunParallelMapRectToImage(destDataArray, taskRunner, srcArray, origin, imageGeomDims, imageGeomSpacing, rectGridDims, xGridValues, yGridValues, zGridValues);
  }

  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return {};
}
