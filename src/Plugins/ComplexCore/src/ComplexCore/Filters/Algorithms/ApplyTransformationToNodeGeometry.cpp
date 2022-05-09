
#include "ApplyTransformationToNodeGeometry.hpp"

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

class ApplyTransformationToNodeGeometryImpl
{

public:
  ApplyTransformationToNodeGeometryImpl(ApplyTransformationToNodeGeometry& filter, const std::vector<float>& transformationMatrix, AbstractGeometry::SharedVertexList* verticesPtr,
                                        const std::atomic_bool& shouldCancel, size_t progIncrement)
  : m_Filter(filter)
  , m_TransformationMatrix(transformationMatrix)
  , m_Vertices(verticesPtr)
  , m_ShouldCancel(shouldCancel)
  , m_ProgIncrement(progIncrement)
  {
  }
  ~ApplyTransformationToNodeGeometryImpl() = default;

  ApplyTransformationToNodeGeometryImpl(const ApplyTransformationToNodeGeometryImpl&) = default;           // Copy Constructor defaulted
  ApplyTransformationToNodeGeometryImpl(ApplyTransformationToNodeGeometryImpl&&) = delete;                 // Move Constructor Not Implemented
  ApplyTransformationToNodeGeometryImpl& operator=(const ApplyTransformationToNodeGeometryImpl&) = delete; // Copy Assignment Not Implemented
  ApplyTransformationToNodeGeometryImpl& operator=(ApplyTransformationToNodeGeometryImpl&&) = delete;      // Move Assignment Not Implemented

  void convert(size_t start, size_t end) const
  {
    using ProjectiveMatrix = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;
    Eigen::Map<const ProjectiveMatrix> transformation(m_TransformationMatrix.data());

    size_t progCounter = 0;
    size_t totalElements = static_cast<int64_t>(end - start);

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

      if(progCounter > m_ProgIncrement)
      {
        m_Filter.sendThreadSafeProgressMessage(progCounter);
        progCounter = 0;
      }
      progCounter++;
    }
    m_Filter.sendThreadSafeProgressMessage(progCounter);
  }

  void operator()(const ComplexRange& range) const
  {
    convert(range.min(), range.max());
  }

private:
  ApplyTransformationToNodeGeometry& m_Filter;
  const std::vector<float>& m_TransformationMatrix;
  AbstractGeometry::SharedVertexList* m_Vertices;
  const std::atomic_bool& m_ShouldCancel;
  size_t m_ProgIncrement = 0;
};

// -----------------------------------------------------------------------------
ApplyTransformationToNodeGeometry::ApplyTransformationToNodeGeometry(DataStructure& dataStructure, ApplyTransformationToNodeGeometryInputValues* inputValues, const std::atomic_bool& shouldCancel,
                                                                     const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ApplyTransformationToNodeGeometry::~ApplyTransformationToNodeGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToNodeGeometry::operator()()
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
  // Needed for Threaded Progress Messages
  m_ProgressCounter = 0;
  m_LastProgressInt = 0;
  size_t progIncrement = m_TotalElements / 100;

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, m_TotalElements);
  dataAlg.execute(ApplyTransformationToNodeGeometryImpl(*this, m_InputValues->transformationMatrix, vertexList, m_ShouldCancel, progIncrement));
  return {};
}

// -----------------------------------------------------------------------------
void ApplyTransformationToNodeGeometry::sendThreadSafeProgressMessage(size_t counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;
  size_t progressInt = static_cast<size_t>((static_cast<float>(m_ProgressCounter) / m_TotalElements) * 100.0f);

  size_t progIncrement = m_TotalElements / 100;

  if(m_ProgressCounter > 1 && m_LastProgressInt != progressInt)
  {
    std::string progressMessage = "Transforming...";
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
  }

  m_LastProgressInt = progressInt;
}
