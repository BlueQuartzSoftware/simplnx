
#include "ApplyTransformationToGeometry.hpp"

#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <Eigen/Dense>

#include <fmt/format.h>

#include <cstdint>
#include <map>

using namespace complex;

namespace ApplyTransformationProgress
{
static size_t s_InstanceIndex = 0;
static std::map<size_t, int64_t> s_ProgressValues;
static std::map<size_t, int64_t> s_LastProgressInt;
} // namespace ApplyTransformationProgress

class ApplyTransformationToGeometryImpl
{

public:
  ApplyTransformationToGeometryImpl(ApplyTransformationToGeometry& filter, const std::vector<float>& transformationMatrix, AbstractGeometry::SharedVertexList* verticesPtr,
                                    const std::atomic_bool& shouldCancel)
  : m_Filter(filter)
  , m_TransformationMatrix(transformationMatrix)
  , m_Vertices(verticesPtr)
  , m_ShouldCancel(shouldCancel)
  {
  }
  ~ApplyTransformationToGeometryImpl() = default;

  ApplyTransformationToGeometryImpl(const ApplyTransformationToGeometryImpl&) = default;           // Copy Constructor defaulted
  ApplyTransformationToGeometryImpl(ApplyTransformationToGeometryImpl&&) = delete;                 // Move Constructor Not Implemented
  ApplyTransformationToGeometryImpl& operator=(const ApplyTransformationToGeometryImpl&) = delete; // Copy Assignment Not Implemented
  ApplyTransformationToGeometryImpl& operator=(ApplyTransformationToGeometryImpl&&) = delete;      // Move Assignment Not Implemented

  void convert(size_t start, size_t end) const
  {
    using ProjectiveMatrix = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;
    Eigen::Map<const ProjectiveMatrix> transformation(m_TransformationMatrix.data());

    int64_t progCounter = 0;
    int64_t totalElements = (end - start);
    int64_t progIncrement = static_cast<int64_t>(totalElements / 100);

    AbstractGeometry::SharedVertexList& vertices = *(m_Vertices);
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      Eigen::Vector4f position(vertices[3 * i + 0], vertices[3 * i + 1], vertices[3 * i + 2], 1);
      Eigen::Vector4f transformedPosition = transformation * position;
      vertices[i * 3 + 0] = transformedPosition[0];
      vertices[i * 3 + 1] = transformedPosition[1];
      vertices[i * 3 + 2] = transformedPosition[2];

      if(progCounter > progIncrement)
      {
        m_Filter.sendThreadSafeProgressMessage(progCounter);
        progCounter = 0;
      }
      progCounter++;
    }
  }

  void operator()(const ComplexRange& range) const
  {
    convert(range.min(), range.max());
  }

private:
  ApplyTransformationToGeometry& m_Filter;
  const std::vector<float>& m_TransformationMatrix;
  AbstractGeometry::SharedVertexList* m_Vertices;
  const std::atomic_bool& m_ShouldCancel;
};

// -----------------------------------------------------------------------------
ApplyTransformationToGeometry::ApplyTransformationToGeometry(DataStructure& dataStructure, ApplyTransformationToGeometryInputValues* inputValues, const std::atomic_bool& shouldCancel,
                                                             const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ApplyTransformationToGeometry::~ApplyTransformationToGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::operator()()
{
  // Needed for Threaded Progress Messages
  m_InstanceIndex = ++ApplyTransformationProgress::s_InstanceIndex;
  ApplyTransformationProgress::s_ProgressValues[m_InstanceIndex] = 0;
  ApplyTransformationProgress::s_LastProgressInt[m_InstanceIndex] = 0;

  return applyTransformation();
}

// -----------------------------------------------------------------------------
void ApplyTransformationToGeometry::sendThreadSafeProgressMessage(int64_t counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  int64_t& progCounter = ApplyTransformationProgress::s_ProgressValues[m_InstanceIndex];
  progCounter += counter;
  int64_t progressInt = static_cast<int64_t>((static_cast<float>(progCounter) / m_TotalElements) * 100.0f);

  int64_t progIncrement = m_TotalElements / 100;
  int64_t prog = 1;

  int64_t& lastProgressInt = ApplyTransformationProgress::s_LastProgressInt[m_InstanceIndex];

  if(progCounter > prog && lastProgressInt != progressInt)
  {
    std::string progressMessage = fmt::format("Transforming || {}% Completed", progressInt);
    m_MessageHandler({IFilter::Message::Type::Progress, progressMessage});
    prog += progIncrement;
  }

  lastProgressInt = progressInt;
}
// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::applyTransformation()
{

  DataObject* dataObject = m_DataStructure.getData(m_InputValues->pGeometryToTransform);

  AbstractGeometry::SharedVertexList* vertexList = nullptr;

  if(dataObject->getDataObjectType() == DataObject::Type::VertexGeom)
  {
    VertexGeom& geom = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->pGeometryToTransform);
    vertexList = geom.getVertices();
  }
  else if(dataObject->getDataObjectType() == DataObject::Type::EdgeGeom)
  {
    EdgeGeom& geom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->pGeometryToTransform);
    vertexList = geom.getVertices();
  }
  else if(dataObject->getDataObjectType() == DataObject::Type::TriangleGeom)
  {
    TriangleGeom& geom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->pGeometryToTransform);
    vertexList = geom.getVertices();
  }
  else if(dataObject->getDataObjectType() == DataObject::Type::QuadGeom)
  {
    QuadGeom& geom = m_DataStructure.getDataRefAs<QuadGeom>(m_InputValues->pGeometryToTransform);
    vertexList = geom.getVertices();
  }
  else if(dataObject->getDataObjectType() == DataObject::Type::TetrahedralGeom)
  {
    TetrahedralGeom& geom = m_DataStructure.getDataRefAs<TetrahedralGeom>(m_InputValues->pGeometryToTransform);
    vertexList = geom.getVertices();
  }
  else if(dataObject->getDataObjectType() == DataObject::Type::HexahedralGeom)
  {
    HexahedralGeom& geom = m_DataStructure.getDataRefAs<HexahedralGeom>(m_InputValues->pGeometryToTransform);
    vertexList = geom.getVertices();
  }
  else
  {
    return {MakeErrorResult(-7010, fmt::format("Geometry is not of the proper Type of Vertex, Edge, Triangle, Quad, Tetrahedral, hexahedral. Type is: '{}", dataObject->getDataObjectType()))};
  }

  m_TotalElements = vertexList->getNumberOfTuples();
  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, m_TotalElements);
  dataAlg.execute(ApplyTransformationToGeometryImpl(*this, m_InputValues->transformationMatrix, vertexList, m_ShouldCancel));
  return {};
}
