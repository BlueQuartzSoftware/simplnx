#include "RotateSampleRefFrame.hpp"

#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <Eigen/Dense>

using namespace nx::core;

namespace
{
using RotationRepresentationType = RotateSampleRefFrame::RotationRepresentation;

constexpr float32 k_Threshold = 0.01f;

const Eigen::Vector3f k_XAxis = Eigen::Vector3f::UnitX();
const Eigen::Vector3f k_YAxis = Eigen::Vector3f::UnitY();
const Eigen::Vector3f k_ZAxis = Eigen::Vector3f::UnitZ();

} // namespace

// -----------------------------------------------------------------------------
RotateSampleRefFrame::RotateSampleRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateSampleRefFrameInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RotateSampleRefFrame::~RotateSampleRefFrame() noexcept = default;

// -----------------------------------------------------------------------------
void RotateSampleRefFrame::updateProgress(const std::string& message)
{
  m_MessageHandler.sendInfoMessage(message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& RotateSampleRefFrame::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> RotateSampleRefFrame::operator()()
{
  auto& srcImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SourceGeometryPath);
  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DestGeometryPath);

  ImageRotationUtilities::Matrix4fR rotationMatrix;

  switch(static_cast<RotationRepresentationType>(m_InputValues->RotationRepresentationIndex))
  {
  case RotationRepresentationType::AxisAngle: {
    rotationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(m_InputValues->RotationAxisAngle);
    break;
  }
  case RotationRepresentationType::RotationMatrix: {
    rotationMatrix = ImageRotationUtilities::GenerateManualTransformationMatrix(m_InputValues->RotationMatrixTable);
    break;
  }
  }

  ImageRotationUtilities::RotateArgs rotateArgs = ImageRotationUtilities::CreateRotationArgs(srcImageGeom, rotationMatrix);

  ImageRotationUtilities::FilterProgressCallback filterProgressCallback(m_MessageHandler, m_ShouldCancel);

  // The actual rotating of the dataStructure arrays is done in parallel where parallel here
  // refers to the cropping of each DataArray being done on a separate thread.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);
  const DataPath srcCellDataAMPath = srcImageGeom.getCellDataPath();
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();

  const DataPath destCellDataAMPath = destImageGeom.getCellDataPath();

  for(const auto& [dataId, srcDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto* srcDataArray = m_DataStructure.getDataAs<IDataArray>(srcCellDataAMPath.createChildPath(srcDataObject->getName()));
    auto* destDataArray = m_DataStructure.getDataAs<IDataArray>(destCellDataAMPath.createChildPath(srcDataObject->getName()));
    m_MessageHandler.sendInfoMessage(fmt::format("Rotating Volume || Copying Data Array {}", srcDataObject->getName()));

    ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithNearestNeighbor>(srcDataArray->getDataType(), taskRunner, srcDataArray, destDataArray, rotateArgs, rotationMatrix,
                                                                                            m_InputValues->SliceBySlice, &filterProgressCallback);
  }

  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  if(m_InputValues->KeepInputGeometryOrigin)
  {
    destImageGeom.setOrigin(srcImageGeom.getOrigin());
  }

  // Surface any error/warning a parallel resample task reported through the shared callback.
  return filterProgressCallback.takeResult();
}
