#include "FindFeatureClustering.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"

#include <random>

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
std::vector<float32> GenerateRandomDistribution(float32 minDistance, float32 maxDistance, int32 numBins, const std::array<float32, 3>& boxDims, const std::array<float32, 3>& boxRes,
                                                uint64 userSeedValue)
{
  std::vector<float32> freq(numBins, 0);
  std::vector<float32> randomCentroids;
  std::vector<std::vector<float32>> distanceList;
  constexpr usize largeNumber = 1000;
  constexpr usize numDistances = largeNumber * (largeNumber - 1);

  // boxDims are the dimensions of the box in microns
  // boxRes is the resolution of the box in microns
  const auto xPoints = static_cast<usize>(boxDims[0] / boxRes[0]);
  const auto yPoints = static_cast<usize>(boxDims[1] / boxRes[1]);
  const auto zPoints = static_cast<usize>(boxDims[2] / boxRes[2]);

  const usize totalPoints = xPoints * yPoints * zPoints;

  const float32 stepSize = (maxDistance - minDistance) / numBins;
  const float32 maxBoxDistance = sqrtf((boxDims[0] * boxDims[0]) + (boxDims[1] * boxDims[1]) + (boxDims[2] * boxDims[2]));
  const auto currentNumBins = static_cast<usize>(ceil((maxBoxDistance - minDistance) / stepSize));

  freq.resize(static_cast<size_t>(currentNumBins + 1));

  std::random_device randomDevice;           // Will be used to obtain a seed for the random number engine
  std::mt19937_64 generator(randomDevice()); // Standard mersenne_twister_engine seeded with rd()
  generator.seed(userSeedValue);
  std::uniform_real_distribution<double> distribution(0.0, 1.0);

  randomCentroids.resize(largeNumber * 3);

  // Generating all of the random points and storing their coordinates in randomCentroids
  for(usize i = 0; i < largeNumber; i++)
  {
    const auto featureOwnerIdx = static_cast<usize>(distribution(generator) * totalPoints);

    const usize column = featureOwnerIdx % xPoints;
    const usize row = (featureOwnerIdx / xPoints) % yPoints;
    const usize plane = featureOwnerIdx / (xPoints * yPoints);

    const auto xc = static_cast<float>(column * boxRes[0]);
    const auto yc = static_cast<float>(row * boxRes[1]);
    const auto zc = static_cast<float>(plane * boxRes[2]);

    randomCentroids[3 * i] = xc;
    randomCentroids[3 * i + 1] = yc;
    randomCentroids[3 * i + 2] = zc;
  }

  distanceList.resize(largeNumber);

  // Calculating all of the distances and storing them in the distance list
  for(size_t i = 1; i < largeNumber; i++)
  {
    const float32 x = randomCentroids[3 * i];
    const float32 y = randomCentroids[3 * i + 1];
    const float32 z = randomCentroids[3 * i + 2];

    for(size_t j = i + 1; j < largeNumber; j++)
    {

      const float32 xn = randomCentroids[3 * j];
      const float32 yn = randomCentroids[3 * j + 1];
      const float32 zn = randomCentroids[3 * j + 2];

      float32 r = sqrtf((x - xn) * (x - xn) + (y - yn) * (y - yn) + (z - zn) * (z - zn));

      distanceList[i].push_back(r);
      distanceList[j].push_back(r);
    }
  }

  // bin up the distance list
  for(usize i = 0; i < largeNumber; i++)
  {
    for(const auto distance : distanceList[i])
    {
      if(distance < minDistance)
      {
        freq[0]++;
      }
      else
      {
        const auto bin = static_cast<usize>((distance - minDistance) / stepSize);
        freq[bin + 1]++;
      }
    }
  }

  // Normalize the frequencies
  for(size_t i = 0; i < currentNumBins + 1; i++)
  {
    freq[i] /= numDistances;
  }

  return freq;
}
} // namespace

// -----------------------------------------------------------------------------
FindFeatureClustering::FindFeatureClustering(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             FindFeatureClusteringInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindFeatureClustering::~FindFeatureClustering() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindFeatureClustering::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindFeatureClustering::operator()()
{
  const auto& imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& equivalentDiameters = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->EquivalentDiametersArrayPath);
  const auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);

  auto& clusteringList = m_DataStructure.getDataRefAs<NeighborList<float32>>(m_InputValues->ClusteringListArrayName);
  auto& rdf = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->RDFArrayName);
  auto& minMaxDistances = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MaxMinArrayName);
  BoolArray* biasedFeatures = nullptr;
  if(m_InputValues->RemoveBiasedFeatures)
  {
    biasedFeatures = m_DataStructure.getDataAs<BoolArray>(m_InputValues->BiasedFeaturesArrayPath);
  }

  float32 x = 0.0f, y = 0.0f, z = 0.0f;
  float32 xn = 0.0f, yn = 0.0f, zn = 0.0f;
  float32 r = 0.0f;

  int32 bin = 0;
  int32 ensemble = 0;
  int32 totalPptFeatures = 0;
  float32 min = std::numeric_limits<float32>::max();
  float32 max = 0.0f;
  float32 value = 0.0f;

  std::vector<std::vector<float32>> clusters;
  std::vector<float32> oldCount(m_InputValues->NumberOfBins);
  std::vector<float32> randomRDF;

  const usize totalFeatures = featurePhases.getNumberOfTuples();

  SizeVec3 dims = imageGeometry.getDimensions();
  FloatVec3 spacing = imageGeometry.getSpacing();

  const float32 sizeX = dims[0] * spacing[0];
  const float32 sizeY = dims[1] * spacing[1];
  const float32 sizeZ = dims[2] * spacing[2];

  // initialize boxDims and boxRes vectors
  const std::array<float32, 3> boxDims = {sizeX, sizeY, sizeZ};
  const std::array<float32, 3> boxRes = {spacing[0], spacing[1], spacing[2]};

  for(usize i = 1; i < totalFeatures; i++)
  {
    if(featurePhases[i] == m_InputValues->PhaseNumber)
    {
      totalPptFeatures++;
    }
  }

  clusters.resize(totalFeatures);

  for(usize i = 1; i < totalFeatures; i++)
  {
    if(featurePhases[i] == m_InputValues->PhaseNumber)
    {
      if(i % 1000 == 0)
      {
        m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Working on Feature {} of {}", i, totalPptFeatures));
      }

      x = centroids[3 * i];
      y = centroids[3 * i + 1];
      z = centroids[3 * i + 2];

      for(usize j = i + 1; j < totalFeatures; j++)
      {
        if(featurePhases[i] == featurePhases[j])
        {
          xn = centroids[3 * j];
          yn = centroids[3 * j + 1];
          zn = centroids[3 * j + 2];

          r = sqrtf((x - xn) * (x - xn) + (y - yn) * (y - yn) + (z - zn) * (z - zn));

          clusters[i].push_back(r);
          clusters[j].push_back(r);
        }
      }
    }
  }

  for(usize i = 1; i < totalFeatures; i++)
  {
    if(featurePhases[i] == m_InputValues->PhaseNumber)
    {
      for(usize j = 0; j < clusters[i].size(); j++)
      {
        value = clusters[i][j];
        if(value > max)
        {
          max = value;
        }
        if(value < min)
        {
          min = value;
        }
      }
    }
  }

  const float32 stepSize = (max - min) / m_InputValues->NumberOfBins;

  minMaxDistances[(m_InputValues->PhaseNumber * 2)] = max;
  minMaxDistances[(m_InputValues->PhaseNumber * 2) + 1] = min;

  for(usize i = 1; i < totalFeatures; i++)
  {
    if(featurePhases[i] == m_InputValues->PhaseNumber)
    {
      if(m_InputValues->RemoveBiasedFeatures && (*biasedFeatures)[i])
      {
        continue;
      }

      for(usize j = 0; j < clusters[i].size(); j++)
      {
        ensemble = featurePhases[i];
        bin = (clusters[i][j] - min) / stepSize;
        if(bin >= m_InputValues->NumberOfBins)
        {
          bin = m_InputValues->NumberOfBins - 1;
        }
        rdf[(m_InputValues->NumberOfBins * ensemble) + bin]++;
      }
    }
  }

  // Generate random distribution based on same box size and same stepSize
  const float32 maxBoxDistance = sqrtf((sizeX * sizeX) + (sizeY * sizeY) + (sizeZ * sizeZ));
  const int32 currentNumBins = ceilf((maxBoxDistance - min) / (stepSize));

  randomRDF.resize(currentNumBins + 1);
  // Call this function to generate the random distribution, which is normalized by the total number of distances
  randomRDF = GenerateRandomDistribution(min, max, m_InputValues->NumberOfBins, boxDims, boxRes, m_InputValues->SeedValue);

  // Scale the random distribution by the number of distances in this particular instance
  const float32 normFactor = totalPptFeatures * (totalPptFeatures - 1);
  for(usize i = 0; i < randomRDF.size(); i++)
  {
    randomRDF[i] *= normFactor;
  }

  for(usize i = 0; i < m_InputValues->NumberOfBins; i++)
  {
    oldCount[i] = rdf[(m_InputValues->NumberOfBins * m_InputValues->PhaseNumber) + i];
    rdf[(m_InputValues->NumberOfBins * m_InputValues->PhaseNumber) + i] = oldCount[i] / randomRDF[i + 1];
  }

  for(usize i = 1; i < totalFeatures; i++)
  {
    // Set the vector for each list into the Clustering Object
    NeighborList<float32>::SharedVectorType sharedClusterLst(new std::vector<float32>);
    sharedClusterLst->assign(clusters[i].begin(), clusters[i].end());
    clusteringList.setList(static_cast<int>(i), sharedClusterLst);
  }
  return {};
}
