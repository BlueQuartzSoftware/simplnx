#include "ImageContouring.hpp"

#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/FlyingEdges.hpp"

using namespace complex;

namespace
{
struct ExecuteFlyingEdgesFunctor
{
  template <typename T>
  void operator()(const ImageGeom& image, const IDataArray& iDataArray, float64 isoVal)
  {
    FlyingEdgesAlgorithm flyingEdges = FlyingEdgesAlgorithm<T>(image, iDataArray, static_cast<T>(isoVal));
    flyingEdges.pass1();
    flyingEdges.pass2();
    flyingEdges.pass3();
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
  const ImageGeom& image = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->imageGeomPath);
  float64 isoVal = m_InputValues->isoVal;
  const IDataArray& iDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->contouringArrayPath);

  ExecuteDataFunction(ExecuteFlyingEdgesFunctor{}, iDataArray.getDataType(), image, iDataArray, isoVal);

  return {};
}
