#include "ComputeBiasedFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeBiasedFeatures::ComputeBiasedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ComputeBiasedFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBiasedFeatures::~ComputeBiasedFeatures() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeBiasedFeatures::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeBiasedFeatures::operator()()
{
  const ImageGeom imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 imageDimensions = imageGeometry.getDimensions();

  if(imageDimensions[0] > 1 && imageDimensions[1] > 1 && imageDimensions[2] > 1)
  {
    findBoundingBoxFeatures();
  }
  else if(imageDimensions[0] == 1 || imageDimensions[1] == 1 || imageDimensions[2] == 1)
  {
    findBoundingBoxFeatures2D();
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> ComputeBiasedFeatures::findBoundingBoxFeatures()
{
  const ImageGeom imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  Int32Array* phasesPtr = nullptr;
  if(m_InputValues->CalcByPhase)
  {
    phasesPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->PhasesArrayPath);
  }
  auto& biasedFeatures = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->BiasedFeaturesArrayName);
  biasedFeatures.fill(false);

  std::unique_ptr<MaskCompare> surfaceFeatures = nullptr;
  try
  {
    surfaceFeatures = InstantiateMaskCompare(m_DataStructure, m_InputValues->SurfaceFeaturesArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Surface Features Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->SurfaceFeaturesArrayPath.toString());
    return MakeErrorResult(-54900, message);
  }

  const usize size = centroids.getNumberOfTuples();
  std::vector<float32> boundBox = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  std::vector<float32> coords = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

  // loop first to determine number of phases if calcByPhase is being used
  int32 numPhases = 1;
  if(m_InputValues->CalcByPhase)
  {
    for(usize i = 1; i < size; i++)
    {
      if((*phasesPtr)[i] > numPhases)
      {
        numPhases = (*phasesPtr)[i];
      }
    }
  }
  for(int32 iter = 1; iter <= numPhases; iter++)
  {
    if(m_InputValues->CalcByPhase)
    {
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Working on Phase {} of {}", iter, numPhases));
    }
    // reset boundBox for each phase
    const BoundingBox3D<float32> boundingBox = imageGeometry.getBoundingBoxf();

    const auto& min = boundingBox.getMinPoint();
    const auto& max = boundingBox.getMaxPoint();
    boundBox[0] = min.getX();
    boundBox[1] = max.getX();
    boundBox[2] = min.getY();
    boundBox[3] = max.getY();
    boundBox[4] = min.getZ();
    boundBox[5] = max.getZ();

    for(usize i = 1; i < size; i++)
    {
      if(surfaceFeatures->isTrue(i) && (!m_InputValues->CalcByPhase || (*phasesPtr)[i] == iter))
      {
        int32 sideToMove = 0;
        int32 move = 1;
        float32 minDist = std::numeric_limits<float32>::max();

        coords[0] = centroids[3 * i];
        coords[1] = centroids[3 * i];
        coords[2] = centroids[3 * i + 1];
        coords[3] = centroids[3 * i + 1];
        coords[4] = centroids[3 * i + 2];
        coords[5] = centroids[3 * i + 2];
        for(int32 j = 1; j < 7; j++)
        {
          float32 dist = std::numeric_limits<float32>::max();
          if(j % 2 == 1)
          {
            if(coords[j - 1] > boundBox[j - 1])
            {
              dist = (coords[j - 1] - boundBox[j - 1]);
            }
            if(coords[j - 1] <= boundBox[j - 1])
            {
              move = 0;
            }
          }
          if(j % 2 == 0)
          {
            if(coords[j - 1] < boundBox[j - 1])
            {
              dist = (boundBox[j - 1] - coords[j - 1]);
            }
            if(coords[j - 1] >= boundBox[j - 1])
            {
              move = 0;
            }
          }
          if(dist < minDist)
          {
            minDist = dist;
            sideToMove = j - 1;
          }
        }
        if(move == 1)
        {
          boundBox[sideToMove] = coords[sideToMove];
        }
      }
    }
    for(usize j = 1; j < size; j++)
    {
      if(!m_InputValues->CalcByPhase || (*phasesPtr)[j] == iter)
      {
        if(centroids[3 * j] <= boundBox[0])
        {
          biasedFeatures[j] = true;
        }
        if(centroids[3 * j] >= boundBox[1])
        {
          biasedFeatures[j] = true;
        }
        if(centroids[3 * j + 1] <= boundBox[2])
        {
          biasedFeatures[j] = true;
        }
        if(centroids[3 * j + 1] >= boundBox[3])
        {
          biasedFeatures[j] = true;
        }
        if(centroids[3 * j + 2] <= boundBox[4])
        {
          biasedFeatures[j] = true;
        }
        if(centroids[3 * j + 2] >= boundBox[5])
        {
          biasedFeatures[j] = true;
        }
      }
    }

    if(getCancel())
    {
      return {};
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> ComputeBiasedFeatures::findBoundingBoxFeatures2D()
{
  const ImageGeom imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 imageDimensions = imageGeometry.getDimensions();
  const FloatVec3 imageOrigin = imageGeometry.getOrigin();
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  auto& biasedFeatures = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->BiasedFeaturesArrayName);
  biasedFeatures.fill(false);

  std::unique_ptr<MaskCompare> maskCompare = nullptr;
  try
  {
    maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->SurfaceFeaturesArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Surface Features Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->SurfaceFeaturesArrayPath.toString());
    return MakeErrorResult(-54900, message);
  }

  const usize size = centroids.getNumberOfTuples();

  std::vector<float32> coords = {0.0f, 0.0f, 0.0f, 0.0f};

  float32 xOrigin = 0.0f;
  float32 yOrigin = 0.0f;
  int32 xPoints = 0;
  int32 yPoints = 0;
  FloatVec3 spacing;

  usize centroidShift0 = 0;
  usize centroidShift1 = 1;

  if(imageDimensions[0] == 1)
  {
    xPoints = static_cast<int32>(imageDimensions[1]);
    yPoints = static_cast<int32>(imageDimensions[2]);
    xOrigin = imageOrigin[1];
    yOrigin = imageOrigin[2];
    spacing = imageGeometry.getSpacing();
    centroidShift0 = 1;
    centroidShift1 = 2;
  }
  if(imageDimensions[1] == 1)
  {
    xPoints = static_cast<int32>(imageDimensions[0]);
    yPoints = static_cast<int32>(imageDimensions[2]);
    xOrigin = imageOrigin[0];
    yOrigin = imageOrigin[2];
    spacing = imageGeometry.getSpacing();
    centroidShift0 = 0;
    centroidShift1 = 2;
  }
  if(imageDimensions[2] == 1)
  {
    xPoints = static_cast<int32>(imageDimensions[0]);
    yPoints = static_cast<int32>(imageDimensions[1]);
    xOrigin = imageOrigin[0];
    yOrigin = imageOrigin[1];
    spacing = imageGeometry.getSpacing();
    centroidShift0 = 0;
    centroidShift1 = 1;
  }

  std::vector<float32> boundBox = {xOrigin, xOrigin + static_cast<float32>(xPoints) * spacing[0], yOrigin, yOrigin + static_cast<float32>(yPoints) * spacing[1], 0.0f, 0.0f};

  for(usize i = 1; i < size; i++)
  {
    if(maskCompare->isTrue(i))
    {
      int32 sideToMove = 0;
      int32 move = 1;
      float32 minDist = std::numeric_limits<float32>::max();
      coords[0] = centroids[3 * i + centroidShift0];
      coords[1] = centroids[3 * i + centroidShift0];
      coords[2] = centroids[3 * i + centroidShift1];
      coords[3] = centroids[3 * i + centroidShift1];
      for(int32 j = 1; j < 5; j++)
      {
        float32 dist = std::numeric_limits<float32>::max();
        if(j % 2 == 1)
        {
          if(coords[j - 1] > boundBox[j - 1])
          {
            dist = (coords[j - 1] - boundBox[j - 1]);
          }
          if(coords[j - 1] <= boundBox[j - 1])
          {
            move = 0;
          }
        }
        if(j % 2 == 0)
        {
          if(coords[j - 1] < boundBox[j - 1])
          {
            dist = (boundBox[j - 1] - coords[j - 1]);
          }
          if(coords[j - 1] >= boundBox[j - 1])
          {
            move = 0;
          }
        }
        if(dist < minDist)
        {
          minDist = dist;
          sideToMove = j - 1;
        }
      }
      if(move == 1)
      {
        boundBox[sideToMove] = coords[sideToMove];
      }
    }

    if(getCancel())
    {
      return {};
    }
  }
  for(usize j = 1; j < size; j++)
  {
    if(centroids[3 * j] <= boundBox[0])
    {
      biasedFeatures[j] = true;
    }
    if(centroids[3 * j] >= boundBox[1])
    {
      biasedFeatures[j] = true;
    }
    if(centroids[3 * j + 1] <= boundBox[2])
    {
      biasedFeatures[j] = true;
    }
    if(centroids[3 * j + 1] >= boundBox[3])
    {
      biasedFeatures[j] = true;
    }

    if(getCancel())
    {
      return {};
    }
  }
  return {};
}
