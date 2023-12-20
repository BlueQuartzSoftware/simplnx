#include "ImageContouring.hpp"

#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/FlyingEdges.hpp"

using namespace nx::core;

namespace
{
struct ExecuteFlyingEdgesFunctor
{
  template <typename T>
  void operator()(const ImageGeom& image, const IDataArray& iDataArray, float64 isoVal, TriangleGeom& triangleGeom, Float32Array& normals, AttributeMatrix& normAM)
  {
    FlyingEdgesAlgorithm flyingEdges = FlyingEdgesAlgorithm<T>(image, iDataArray, static_cast<T>(isoVal), triangleGeom, normals);
    flyingEdges.pass1();
    flyingEdges.pass2();
    flyingEdges.pass3();

    // pass 3 resized normals so be sure to resize parent AM
    normAM.resizeTuples(normals.getTupleShape());

    flyingEdges.pass4();
  }
};
} // namespace

// -----------------------------------------------------------------------------
ImageContouring::ImageContouring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImageContouringInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImageContouring::~ImageContouring() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImageContouring::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImageContouring::operator()()
{
  const auto& image = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->imageGeomPath);
  float64 isoVal = m_InputValues->isoVal;
  const auto& iDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->contouringArrayPath);
  auto triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->triangleGeomPath);
  auto normals = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->normalsArrayPath);

  // auto created so must have a parent
  DataPath normAMPath = m_InputValues->normalsArrayPath.getParent();

  auto& normAM = m_DataStructure.getDataRefAs<AttributeMatrix>(normAMPath);

  ExecuteNeighborFunction(ExecuteFlyingEdgesFunctor{}, iDataArray.getDataType(), image, iDataArray, isoVal, triangleGeom, normals, normAM);

  return {};
}
