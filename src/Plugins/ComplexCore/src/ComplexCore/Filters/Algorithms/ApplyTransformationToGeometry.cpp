#include "ApplyTransformationToGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ApplyTransformationToGeometry::ApplyTransformationToGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             ApplyTransformationToGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ApplyTransformationToGeometry::~ApplyTransformationToGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ApplyTransformationToGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::applyImageGeometryTransformation()
{
  // Pure translation for Image Geom, just return
  if(m_InputValues->TransformationSelection == k_TranslationIdx)
  {
    return {};
  }

  // Pure Scale for image geom, just return
  if(m_InputValues->TransformationSelection == k_ScaleIdx)
  {
    return {};
  }

  DataPath destImagePath;
  if(m_InputValues->RemoveOriginalGeometry)
  {
    // Create an Image Geometry name with a "." as a prefix to the original Image Geometry Name
    std::vector<std::string> tempPathVector = m_InputValues->SelectedGeometryPath.getPathVector();
    tempPathVector.back() = "." + tempPathVector.back();
    destImagePath = DataPath({tempPathVector});
  }

  auto& srcImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedGeometryPath);
  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(destImagePath);

  const auto rotateArgs = ImageRotationUtilities::CreateRotationArgs(srcImageGeom, m_TransformationMatrix);

  auto selectedCellDataChildren = GetAllChildArrayDataPaths(m_DataStructure, srcImageGeom.getCellDataPath());
  auto selectedCellArrays = selectedCellDataChildren.has_value() ? selectedCellDataChildren.value() : std::vector<DataPath>{};

  ImageRotationUtilities::FilterProgressCallback filterProgressCallback(m_MessageHandler, m_ShouldCancel);

  // The actual rotating of the dataStructure arrays is done in parallel where parallel here
  // refers to the cropping of each DataArray being done on a separate thread.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);

  const DataPath srcCelLDataAMPath = srcImageGeom.getCellDataPath();
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();

  const DataPath destCellDataAMPath = destImageGeom.getCellDataPath();

  if(m_InputValues->TransformationSelection == k_PrecomputedTransformationMatrixIdx)
  {
    // Adjust the destination objects because we didn't have the transformation matrix values during preflight
    auto& destCellDataAM = destImageGeom.getCellDataRef();
    const std::vector<usize> dims = {static_cast<usize>(rotateArgs.xpNew), static_cast<usize>(rotateArgs.ypNew), static_cast<usize>(rotateArgs.zpNew)};
    const std::vector<float32> spacing = {rotateArgs.xResNew, rotateArgs.yResNew, rotateArgs.zResNew};
    auto origin = srcImageGeom.getOrigin().toContainer<std::vector<float32>>();
    origin[0] = rotateArgs.xMinNew;
    origin[1] = rotateArgs.yMinNew;
    origin[2] = rotateArgs.zMinNew;

    std::vector<usize> const dataArrayShape = {dims[2], dims[1], dims[0]}; // The DataArray shape goes slowest to fastest (ZYX), opposite of ImageGeometry dimensions
    destImageGeom.setDimensions(dims);
    destImageGeom.setOrigin(origin);
    destImageGeom.setSpacing(spacing);
    destCellDataAM.resizeTuples(dataArrayShape);
  }

  for(const auto& [dataId, srcDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto* srcDataArrayPtr = m_DataStructure.getDataAs<IDataArray>(srcCelLDataAMPath.createChildPath(srcDataObject->getName()));
    auto* destDataArrayPtr = m_DataStructure.getDataAs<IDataArray>(destCellDataAMPath.createChildPath(srcDataObject->getName()));
    m_MessageHandler(fmt::format("Applying Transform || Copying Data Array {}", srcDataObject->getName()));

    if(m_InputValues->InterpolationSelection == k_NearestNeighborInterpolationIdx)
    {
      ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithNearestNeighbor>(srcDataArrayPtr->getDataType(), taskRunner, srcDataArrayPtr, destDataArrayPtr, rotateArgs,
                                                                                              m_TransformationMatrix, false, &filterProgressCallback);
    }
    else if(m_InputValues->InterpolationSelection == k_LinearInterpolationIdx)
    {
      ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithTrilinearInterpolation, NoBooleanType>(srcDataArrayPtr->getDataType(), taskRunner, srcDataArrayPtr, destDataArrayPtr,
                                                                                                                    rotateArgs, m_TransformationMatrix, &filterProgressCallback);
    }

    if(getCancel())
    {
      break;
    }
  }

  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return {};
}

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::applyNodeGeometryTransformation()
{
  INodeGeometry0D& nodeGeometry0D = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->SelectedGeometryPath);

  IGeometry::SharedVertexList& vertexList = nodeGeometry0D.getVerticesRef();

  ImageRotationUtilities::FilterProgressCallback filterProgressCallback(m_MessageHandler, m_ShouldCancel);

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, vertexList.getNumberOfTuples());
  dataAlg.execute(ImageRotationUtilities::ApplyTransformationToNodeGeometry(vertexList, m_TransformationMatrix, &filterProgressCallback));

  return {};
}

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::operator()()
{
  if(!m_InputValues->RemoveOriginalGeometry)
  {
    return MakeErrorResult(-84500, fmt::format("Keeping the original geometry is not supported."));
  }

  auto* imageGeometryPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->SelectedGeometryPath);
  const bool isNodeBased = (imageGeometryPtr == nullptr);

  ImageRotationUtilities::Matrix4fR translationToGlobalOriginMat = ImageRotationUtilities::Matrix4fR::Identity();
  ImageRotationUtilities::Matrix4fR translationFromGlobalOriginMat = ImageRotationUtilities::Matrix4fR::Identity();
  if(isNodeBased)
  {
    Point3D<float32> minPoint = {0.0F, 0.0F, 0.0F};
    auto& nodeGeometry0D = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->SelectedGeometryPath);
    auto boundingBox = nodeGeometry0D.getBoundingBox();
    minPoint = boundingBox.getMinPoint();
    translationToGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({-minPoint[0], -minPoint[1], -minPoint[2]});
    translationFromGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({minPoint[0], minPoint[1], minPoint[2]});
  }

  switch(m_InputValues->TransformationSelection)
  {
  case k_NoTransformIdx: // No-Op
  {
    return {};
  }
  case k_PrecomputedTransformationMatrixIdx: // Transformation matrix from array
  {
    const Float32Array& precomputed = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->ComputedTransformationMatrix);
    m_TransformationMatrix = ImageRotationUtilities::CopyPrecomputedToTransformationMatrix(precomputed);
    break;
  }
  case k_ManualTransformationMatrixIdx: // Manual transformation matrix
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateManualTransformationMatrix(m_InputValues->ManualMatrixTableData);
    break;
  }
  case k_RotationIdx: // Rotation via axis-angle
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(m_InputValues->Rotation);
    if(isNodeBased)
    {
      // Translate the geometry to/from the global origin
      m_TransformationMatrix = translationFromGlobalOriginMat * m_TransformationMatrix * translationToGlobalOriginMat;
    }

    break;
  }
  case k_TranslationIdx: // Translation
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateTranslationTransformationMatrix(m_InputValues->Translation);
    break;
  }
  case k_ScaleIdx: // Scale
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateScaleTransformationMatrix(m_InputValues->Scale);
    if(isNodeBased)
    {
      // Translate the geometry to/from the global origin
      m_TransformationMatrix = translationFromGlobalOriginMat * m_TransformationMatrix * translationToGlobalOriginMat;
    }
    break;
  }
  }

  // Apply geometry transformation
  if(isNodeBased)
  {
    applyNodeGeometryTransformation();
  }
  else
  {
    applyImageGeometryTransformation();
  }

  return {};
}
