#include "ComputeBiasedFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeBiasedFeatures::ComputeBiasedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBiasedFeaturesInputValues* inputValues)
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
void ComputeBiasedFeatures::findBoundingBoxFeatures()
{
  const ImageGeom imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  const auto& surfaceFeatures = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->SurfaceFeaturesArrayPath);
  Int32Array* phases = nullptr;
  if(m_InputValues->CalcByPhase)
  {
    phases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->PhasesArrayPath);
  }
  auto& biasedFeatures = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->BiasedFeaturesArrayName);
  biasedFeatures.fill(false);

  const usize size = centroids.getNumberOfTuples();
  float boundBox[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  float32 coords[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  float32 x = 0.0f;
  float32 y = 0.0f;
  float32 z = 0.0f;
  float32 dist[7] = {
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  };
  float32 minDist = std::numeric_limits<float32>::max();
  int32 sideToMove = 0;
  int32 move = 0;

  // loop first to determine number of phases if calcByPhase is being used
  int32 numPhases = 1;
  if(m_InputValues->CalcByPhase)
  {
    for(usize i = 1; i < size; i++)
    {
      if((*phases)[i] > numPhases)
      {
        numPhases = (*phases)[i];
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
    const BoundingBox3D<float32> bb = imageGeometry.getBoundingBoxf();

    auto min = bb.getMinPoint();
    auto max = bb.getMaxPoint();
    boundBox[0] = min.getX();
    boundBox[1] = max.getX();
    boundBox[2] = min.getY();
    boundBox[3] = max.getY();
    boundBox[4] = min.getZ();
    boundBox[5] = max.getZ();

    for(usize i = 1; i < size; i++)
    {
      if(surfaceFeatures[i] && (!m_InputValues->CalcByPhase || (*phases)[i] == iter))
      {
        sideToMove = 0;
        move = 1;
        minDist = std::numeric_limits<float32>::max();
        x = centroids[3 * i];
        y = centroids[3 * i + 1];
        z = centroids[3 * i + 2];
        coords[0] = x;
        coords[1] = x;
        coords[2] = y;
        coords[3] = y;
        coords[4] = z;
        coords[5] = z;
        for(int32 j = 1; j < 7; j++)
        {
          dist[j] = std::numeric_limits<float32>::max();
          if(j % 2 == 1)
          {
            if(coords[j - 1] > boundBox[j - 1])
            {
              dist[j] = (coords[j - 1] - boundBox[j - 1]);
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
              dist[j] = (boundBox[j - 1] - coords[j - 1]);
            }
            if(coords[j - 1] >= boundBox[j - 1])
            {
              move = 0;
            }
          }
          if(dist[j] < minDist)
          {
            minDist = dist[j];
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
      if(!m_InputValues->CalcByPhase || (*phases)[j] == iter)
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
      return;
    }
  }
}

// -----------------------------------------------------------------------------
void ComputeBiasedFeatures::findBoundingBoxFeatures2D()
{
  const ImageGeom imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 imageDimensions = imageGeometry.getDimensions();
  const FloatVec3 imageOrigin = imageGeometry.getOrigin();
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  const auto& surfaceFeatures = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->SurfaceFeaturesArrayPath);
  auto& biasedFeatures = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->BiasedFeaturesArrayName);
  biasedFeatures.fill(false);

  const usize size = centroids.getNumberOfTuples();

  float32 coords[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float32 x = 0.0f;
  float32 y = 0.0f;
  float32 dist[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  float32 minDist = std::numeric_limits<float32>::max();
  int32 sideToMove = 0;
  int32 move = 0;

  float32 xOrigin = 0.0f;
  float32 yOrigin = 0.0f;
  int32 xPoints = 0, yPoints = 0;
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

  float32 boundBox[6] = {xOrigin, xOrigin + xPoints * spacing[0], yOrigin, yOrigin + yPoints * spacing[1], 0.0f, 0.0f};

  for(usize i = 1; i < size; i++)
  {
    if(surfaceFeatures[i])
    {
      sideToMove = 0;
      move = 1;
      minDist = std::numeric_limits<float32>::max();
      x = centroids[3 * i + centroidShift0];
      y = centroids[3 * i + centroidShift1];
      coords[0] = x;
      coords[1] = x;
      coords[2] = y;
      coords[3] = y;
      for(int32 j = 1; j < 5; j++)
      {
        dist[j] = std::numeric_limits<float32>::max();
        if(j % 2 == 1)
        {
          if(coords[j - 1] > boundBox[j - 1])
          {
            dist[j] = (coords[j - 1] - boundBox[j - 1]);
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
            dist[j] = (boundBox[j - 1] - coords[j - 1]);
          }
          if(coords[j - 1] >= boundBox[j - 1])
          {
            move = 0;
          }
        }
        if(dist[j] < minDist)
        {
          minDist = dist[j];
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
      return;
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
      return;
    }
  }
}
