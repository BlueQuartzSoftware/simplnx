#include "ApplyTransformationToGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

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
  if(m_InputValues->TransformationSelection == detail::k_TranslationIdx)
  {
    return {};
  }

  // Pure Scale for image geom, just return
  if(m_InputValues->TransformationSelection == detail::k_ScaleIdx)
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

  if(m_InputValues->TransformationSelection == detail::k_PrecomputedTransformationMatrixIdx)
  {
    // Adjust the destination objects because we didn't have the transformation matrix values during preflight
    auto& destCellDataAM = destImageGeom.getCellDataRef();
    const std::vector<usize> dims = {static_cast<usize>(rotateArgs.outputDims[0]), static_cast<usize>(rotateArgs.outputDims[1]), static_cast<usize>(rotateArgs.outputDims[2])};
    const std::vector<float32> spacing = {rotateArgs.outputSpacing[0], rotateArgs.outputSpacing[1], rotateArgs.outputSpacing[2]};
    auto origin = srcImageGeom.getOrigin().toContainer<std::vector<float32>>();
    origin[0] = rotateArgs.outputXMin;
    origin[1] = rotateArgs.outputYMin;
    origin[2] = rotateArgs.outputZMin;

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

    if(m_InputValues->InterpolationSelection == detail::k_NearestNeighborInterpolationIdx)
    {
      m_MessageHandler.sendInfoMessage(fmt::format("Applying Transform || Nearest Neighbor Interpolation {}", srcDataObject->getName()));

      ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithNearestNeighbor>(srcDataArrayPtr->getDataType(), taskRunner, srcDataArrayPtr, destDataArrayPtr, rotateArgs,
                                                                                              m_TransformationMatrix, false, &filterProgressCallback);
    }
    else if(m_InputValues->InterpolationSelection == detail::k_LinearInterpolationIdx)
    {
      m_MessageHandler.sendInfoMessage(fmt::format("Applying Transform || Trilinear Interpolation {}", srcDataObject->getName()));

      ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithTrilinearInterpolation, NoBooleanType>(srcDataArrayPtr->getDataType(), taskRunner, srcDataArrayPtr, destDataArrayPtr,
                                                                                                                    rotateArgs, m_TransformationMatrix, &filterProgressCallback);
    }

    if(getCancel())
    {
      break;
    }
  }

  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  // Surface any error/warning a parallel resample task reported through the shared callback.
  return filterProgressCallback.takeResult();
}

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::applyNodeGeometryTransformation()
{
  auto& nodeGeometry0D = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->SelectedGeometryPath);

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

  switch(m_InputValues->TransformationSelection)
  {
  case detail::k_NoTransformIdx: // No-Op
  {
    return {};
  }
  case detail::k_PrecomputedTransformationMatrixIdx: // Transformation matrix from array
  {
    const auto& precomputed = m_DataStructure.getDataAsUnsafe<Float32Array>(m_InputValues->ComputedTransformationMatrix)->getDataStoreRef();
    m_TransformationMatrix = ImageRotationUtilities::CopyPrecomputedToTransformationMatrix(precomputed);
    break;
  }
  case detail::k_ManualTransformationMatrixIdx: // Manual transformation matrix
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateManualTransformationMatrix(m_InputValues->ManualMatrixTableData);
    break;
  }
  case detail::k_RotationIdx: // Rotation via axis-angle
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(m_InputValues->Rotation);
    break;
  }
  case detail::k_TranslationIdx: // Translation
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateTranslationTransformationMatrix(m_InputValues->Translation);
    break;
  }
  case detail::k_ScaleIdx: // Scale
  {
    m_TransformationMatrix = ImageRotationUtilities::GenerateScaleTransformationMatrix(m_InputValues->Scale);
    break;
  }
  }

  // Apply geometry transformation
  auto* imageGeometryPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->SelectedGeometryPath);
  auto* nodeGeometry0D = m_DataStructure.getDataAs<INodeGeometry0D>(m_InputValues->SelectedGeometryPath);
  if(m_InputValues->TranslateGeometryToGlobalOrigin)
  {
    auto boundingBox = (imageGeometryPtr != nullptr) ? imageGeometryPtr->getBoundingBoxf() : nodeGeometry0D->getBoundingBox();
    Point3Df minPoint = boundingBox.getMinPoint();
    const ImageRotationUtilities::Matrix4fR translationToGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({-minPoint[0], -minPoint[1], -minPoint[2]});
    const ImageRotationUtilities::Matrix4fR translationFromGlobalOriginMat = ImageRotationUtilities::GenerateTranslationTransformationMatrix({minPoint[0], minPoint[1], minPoint[2]});
    m_TransformationMatrix = translationFromGlobalOriginMat * m_TransformationMatrix * translationToGlobalOriginMat;
  }

  // If asked to do so, save the transformation matrix as a flattened 1x16 array where we raster
  // along the columns the fastest then the the rows (Same as an image with its origin in the upper left
  if(m_InputValues->SaveTransformMatrix)
  {
    auto& transformMatrix = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->TransformMatrixPath);
    usize index = 0;
    for(usize row = 0; row < 4; row++)
    {
      for(usize col = 0; col < 4; col++)
      {
        transformMatrix[index++] = m_TransformationMatrix(row, col);
      }
    }
  }

  if(imageGeometryPtr == nullptr)
  {
    applyNodeGeometryTransformation();
  }
  else
  {
    applyImageGeometryTransformation();
  }

  return {};
}
