#include "RotateSampleRefFrame.hpp"

#include "simplnx/Utilities/AlgorithmDispatch.hpp"
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

RotateSampleRefFrame::RotateSampleRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateSampleRefFrameInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

const std::atomic_bool& RotateSampleRefFrame::getCancel()
{
  return m_ShouldCancel;
}

Result<> RotateSampleRefFrame::operator()()
{
  // Preflight actions have already cleared or copied the Feature Attribute Matrix.
  auto& srcImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->source_geometry_path);
  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->dest_geometry_path);

  image_rotation_utilities::Matrix4fRType rotationMatrix;

  switch(static_cast<RotationRepresentationType>(m_InputValues->rotation_representation_index))
  {
  case RotationRepresentationType::AxisAngle: {
    rotationMatrix = image_rotation_utilities::GenerateRotationTransformationMatrix(m_InputValues->rotation_axis_angle);
    break;
  }
  case RotationRepresentationType::RotationMatrix: {
    rotationMatrix = image_rotation_utilities::GenerateManualTransformationMatrix(m_InputValues->rotation_matrix_table);
    break;
  }
  }

  image_rotation_utilities::RotateArgs rotateArgs = image_rotation_utilities::CreateRotationArgs(srcImageGeom, rotationMatrix);

  image_rotation_utilities::FilterProgressCallback filterProgressCallback(m_MessageHandler, m_ShouldCancel);

  // Resident cell arrays rotate as independent tasks.
  ParallelTaskAlgorithm taskRunner;
  const DataPath srcCellDataAMPath = srcImageGeom.getCellDataPath();
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();
  filterProgressCallback.resetProgress(srcCellDataAM.getSize() * static_cast<usize>(rotateArgs.output_dims[2]), "Interpolating Cell Arrays");

  const DataPath destCellDataAMPath = destImageGeom.getCellDataPath();

  // Serialize all array tasks when one store needs bounded pages. This prevents
  // independent page windows from competing for RAM and the disk chunk cache.
  bool usesOutOfCoreStore = false;
  for(const auto& [dataId, srcDataObject] : srcCellDataAM)
  {
    const auto* srcDataArrayPtr = m_DataStructure.getDataAs<IDataArray>(srcCellDataAMPath.createChildPath(srcDataObject->getName()));
    const auto* destDataArrayPtr = m_DataStructure.getDataAs<IDataArray>(destCellDataAMPath.createChildPath(srcDataObject->getName()));
    usesOutOfCoreStore = usesOutOfCoreStore || IsOutOfCore(*srcDataArrayPtr) || IsOutOfCore(*destDataArrayPtr);
  }
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  taskRunner.setParallelizationEnabled(!useOutOfCoreAlgorithm);

  for(const auto& [dataId, srcDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto* srcDataArrayPtr = m_DataStructure.getDataAs<IDataArray>(srcCellDataAMPath.createChildPath(srcDataObject->getName()));
    auto* destDataArrayPtr = m_DataStructure.getDataAs<IDataArray>(destCellDataAMPath.createChildPath(srcDataObject->getName()));
    filterProgressCallback.sendThreadSafeStatusMessage(fmt::format("Rotating Volume || Copying Data Array {}", srcDataObject->getName()));

    ExecuteParallelFunction<image_rotation_utilities::RotateImageGeometryWithNearestNeighbor>(srcDataArrayPtr->getDataType(), taskRunner, srcDataArrayPtr, destDataArrayPtr, rotateArgs, rotationMatrix,
                                                                                            m_InputValues->slice_by_slice, &filterProgressCallback);
  }

  taskRunner.wait();

  if(m_InputValues->keep_input_geometry_origin)
  {
    if(auto result = destImageGeom.setOrigin(srcImageGeom.getOrigin()); result.invalid())
    {
      return result;
    }
  }

  // Publish errors and warnings after all array tasks join.
  return filterProgressCallback.takeResult();
}
