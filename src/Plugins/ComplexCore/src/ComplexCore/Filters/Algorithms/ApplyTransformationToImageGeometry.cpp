
#include "ApplyTransformationToImageGeometry.hpp"

#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <Eigen/Dense>

#include <fmt/format.h>

#include <cstdint>
#include <map>

using namespace complex;

class ApplyTransformationToImageGeometryImpl
{

public:
  ApplyTransformationToImageGeometryImpl(ApplyTransformationToImageGeometry& filter, const std::vector<float>& transformationMatrix,
                                    const std::atomic_bool& shouldCancel, size_t progIncrement)
  : m_Filter(filter)
  , m_TransformationMatrix(transformationMatrix)
  , m_ShouldCancel(shouldCancel)
  , m_ProgIncrement(progIncrement)
  {
  }
  ~ApplyTransformationToImageGeometryImpl() = default;

  ApplyTransformationToImageGeometryImpl(const ApplyTransformationToImageGeometryImpl&) = default;           // Copy Constructor defaulted
  ApplyTransformationToImageGeometryImpl(ApplyTransformationToImageGeometryImpl&&) = delete;                 // Move Constructor Not Implemented
  ApplyTransformationToImageGeometryImpl& operator=(const ApplyTransformationToImageGeometryImpl&) = delete; // Copy Assignment Not Implemented
  ApplyTransformationToImageGeometryImpl& operator=(ApplyTransformationToImageGeometryImpl&&) = delete;      // Move Assignment Not Implemented

  //void convert(size_t start, size_t end) const
  //{
  //  using ProjectiveMatrix = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;
  //  Eigen::Map<const ProjectiveMatrix> transformation(m_TransformationMatrix.data());

  //  size_t progCounter = 0;
  //  size_t totalElements = static_cast<int64_t>(end - start);

  //  AbstractGeometry::SharedVertexList& vertices = *(m_Vertices);
  //  for(size_t i = start; i < end; i++)
  //  {
  //    if(m_ShouldCancel)
  //    {
  //      return;
  //    }
  //    Eigen::Vector4f position(vertices[3 * i + 0], vertices[3 * i + 1], vertices[3 * i + 2], 1);
  //    Eigen::Vector4f transformedPosition = transformation * position;
  //    vertices[i * 3 + 0] = transformedPosition[0];
  //    vertices[i * 3 + 1] = transformedPosition[1];
  //    vertices[i * 3 + 2] = transformedPosition[2];

  //    if(progCounter > m_ProgIncrement)
  //    {
  //      m_Filter.sendThreadSafeProgressMessage(progCounter);
  //      progCounter = 0;
  //    }
  //    progCounter++;
  //  }
  //  m_Filter.sendThreadSafeProgressMessage(progCounter);
  //}

  //void operator()(const ComplexRange& range) const
  //{
  //  convert(range.min(), range.max());
  //}

private:
  ApplyTransformationToImageGeometry& m_Filter;
  const std::vector<float>& m_TransformationMatrix;
  const std::atomic_bool& m_ShouldCancel;
  size_t m_ProgIncrement = 0;
};

// -----------------------------------------------------------------------------
struct CopyDataFunctor
{
  template <class T>
  Result<> operator()(const IDataArray& oldCellArray, IDataArray& newCellArray, nonstd::span<const int64> newIndices) const
  {
    const auto& oldDataStore = oldCellArray.getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& newDataStore = newCellArray.getIDataStoreRefAs<AbstractDataStore<T>>();

    for(usize i = 0; i < newIndices.size(); i++)
    {
      int64 newIndicies_I = newIndices[i];
      if(newIndicies_I >= 0)
      {
        if(!newDataStore.copyFrom(i, oldDataStore, newIndicies_I, 1))
        {
          return MakeErrorResult(-45102, fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", oldCellArray.getName(),
                                                     newIndicies_I, newCellArray.getName(), i));
        }
      }
      else
      {
        newDataStore.fillTuple(i, 0);
      }
    }

    return {};
  }
};

// -----------------------------------------------------------------------------
ApplyTransformationToImageGeometry::ApplyTransformationToImageGeometry(DataStructure& dataStructure, ApplyTransformationToImageGeometryInputValues* inputValues, const std::atomic_bool& shouldCancel,
                                                             const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ApplyTransformationToImageGeometry::~ApplyTransformationToImageGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToImageGeometry::operator()()
{

  DataObject* dataObject = m_DataStructure.getData(m_InputValues->pGeometryToTransform);

  AbstractGeometry::SharedVertexList* vertexList = nullptr;

  if(dataObject->getDataObjectType() == DataObject::Type::ImageGeom)
  {
    ApplyImageTransformation();
    return;
  }
  else
  {
    return {MakeErrorResult(-7010, fmt::format("Geometry is not of the proper Type of Image. Type is: '{}", dataObject->getDataObjectType()))};
  }

  //m_TotalElements = vertexList->getNumberOfTuples();
  //// Needed for Threaded Progress Messages
  //m_ProgressCounter = 0;
  //m_LastProgressInt = 0;
  //size_t progIncrement = m_TotalElements / 100;

  //// Allow data-based parallelization
  //ParallelDataAlgorithm dataAlg;
  //dataAlg.setRange(0, m_TotalElements);
  //dataAlg.execute(ApplyTransformationToImageGeometryImpl(*this, m_InputValues->transformationMatrix, vertexList, m_ShouldCancel, progIncrement));
  //return {};
}

// -----------------------------------------------------------------------------
//void ApplyTransformationToImageGeometry::sendThreadSafeProgressMessage(size_t counter)
//{
//  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
//
//  m_ProgressCounter += counter;
//  size_t progressInt = static_cast<size_t>((static_cast<float>(m_ProgressCounter) / m_TotalElements) * 100.0f);
//
//  size_t progIncrement = m_TotalElements / 100;
//
//  if(m_ProgressCounter > 1 && m_LastProgressInt != progressInt)
//  {
//    std::string progressMessage = "Transforming...";
//    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
//  }
//
//  m_LastProgressInt = progressInt;
//}
