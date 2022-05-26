
#include "ApplyTransformationToGeometry.hpp"

#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <Eigen/Dense>

#include <fmt/format.h>

#include <cstdint>
#include <map>

using namespace complex;

class ApplyTransformationToGeometryImpl
{

public:
  ApplyTransformationToGeometryImpl(ApplyTransformationToGeometry& filter, const std::vector<float>& transformationMatrix, IGeometry::SharedVertexList* verticesPtr,
                                    const std::atomic_bool& shouldCancel, size_t progIncrement)
  : m_Filter(filter)
  , m_TransformationMatrix(transformationMatrix)
  , m_Vertices(verticesPtr)
  , m_ShouldCancel(shouldCancel)
  , m_ProgIncrement(progIncrement)
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

    size_t progCounter = 0;
    size_t totalElements = static_cast<int64_t>(end - start);

    IGeometry::SharedVertexList& vertices = *(m_Vertices);
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
  ApplyTransformationToGeometry& m_Filter;
  const std::vector<float>& m_TransformationMatrix;
  IGeometry::SharedVertexList* m_Vertices;
  const std::atomic_bool& m_ShouldCancel;
  size_t m_ProgIncrement = 0;
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
  auto& geom = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->pGeometryToTransform);

  IGeometry::SharedVertexList* vertexList = geom.getVertices();

  m_TotalElements = vertexList->getNumberOfTuples();
  // Needed for Threaded Progress Messages
  m_ProgressCounter = 0;
  m_LastProgressInt = 0;
  size_t progIncrement = m_TotalElements / 100;

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, m_TotalElements);
  dataAlg.execute(ApplyTransformationToGeometryImpl(*this, m_InputValues->transformationMatrix, vertexList, m_ShouldCancel, progIncrement));
  return {};
}

// -----------------------------------------------------------------------------
void ApplyTransformationToGeometry::sendThreadSafeProgressMessage(size_t counter)
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
