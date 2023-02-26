#include "ApplyTransformationToGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
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

  DataPath destImagePath;
  if(m_InputValues->RemoveOriginalGeometry)
  {
    // Generate a new name for the current Image Geometry
    auto tempPathVector = m_InputValues->SelectedGeometryPath.getPathVector();
    std::string tempName = "." + tempPathVector.back();
    tempPathVector.back() = tempName;
    DataPath tempPath(tempPathVector);

    tempPathVector = m_InputValues->SelectedGeometryPath.getPathVector();
    tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  auto& srcImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedGeometryPath);
  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(destImagePath);

  auto rotateArgs = ImageRotationUtilities::CreateRotationArgs(srcImageGeom, m_TransformationMatrix);

  int64_t newNumCellTuples = rotateArgs.xpNew * rotateArgs.ypNew * rotateArgs.zpNew;
  int64_t m_TotalElements = newNumCellTuples;

  auto selectedCellDataChildren = GetAllChildArrayDataPaths(m_DataStructure, srcImageGeom.getCellDataPath());
  auto selectedCellArrays = selectedCellDataChildren.has_value() ? selectedCellDataChildren.value() : std::vector<DataPath>{};

  // The actual rotating of the dataStructure arrays is done in parallel where parallel here
  // refers to the cropping of each DataArray being done on a separate thread.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(false);
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();

  for(const auto& [dataId, oldDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
    const std::string srcName = oldDataArray.getName();

    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));
    m_MessageHandler(fmt::format("Rotating Volume || Copying Data Array {}", srcName));

    ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithNearestNeighbor>(oldDataArray.getDataType(), taskRunner, oldDataArray, newDataArray, rotateArgs, m_TransformationMatrix,
                                                                                            false);
  }

  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

#if 0
  for(const auto& selectedDataArrayPath :)
  {
    // Get the source array from our "cached" Cell AttributeMatrix
    IDataArray::Pointer sourceArray = m_SourceAttributeMatrix->getAttributeArray(attrArrayName);
    // Get the target attribute array from our destination Cell AttributeMatrix which has *NOT* been allocated yet.
    IDataArray::Pointer targetArray = m->getAttributeMatrix(attrMatName)->getAttributeArray(attrArrayName);


    // So this little work-around is because if we just try to resize the DataArray<T> will think the sizes are the same
    // and never actually allocate the data. So we just resize to 1 tuple, and then to the real size.
    targetArray->resizeTuples(1);                // Allocate the memory for this data array
    targetArray->resizeTuples(newNumCellTuples); // Allocate the memory for this data array
    if(m_InterpolationType == k_NearestNeighborInterpolation)
    {
    }
    else if(m_InterpolationType == k_LinearInterpolation)
    {
      ImageRotationUtilities::ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithTrilinearInterpolation>(sourceArray, taskRunner, this, this, sourceArray, targetArray, m_Params,
                                                                                                                             m_TransformationMatrix);
    }
    if(getCancel() || getErrorCode() < 0)
    {
      break;
    }

  }
  taskRunner.wait();
#endif
  return {};
}

// -----------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::applyNodeGeometryTransformation()
{
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
  case k_NoTransformIdx: // No-Op
  {
    return {};
  }
  case k_PrecomputedTransformationMatrixIdx: // Transformation matrix from array
  {
    Float32Array& precomputed = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->ComputedTransformationMatrix);
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
    break;
  }
  }

  auto* geometryPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->SelectedGeometryPath);

  if(geometryPtr != nullptr) // Function for applying Image Transformation
  {
    applyImageGeometryTransformation();
  }
  else
  {
    applyNodeGeometryTransformation();
  }

  return {};
}
