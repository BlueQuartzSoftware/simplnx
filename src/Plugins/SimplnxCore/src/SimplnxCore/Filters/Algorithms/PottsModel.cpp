#include "PottsModel.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <vector>

using namespace nx::core;

namespace
{
constexpr float64 k_BoltzmannConstant = 1.38064852e-23;
constexpr int32 k_NoEligibleCellsError = -72005;
constexpr int32 k_InvalidDimensionalityError = -72006;

using Neighborhood = std::vector<std::array<int8, 3>>;

enum class Dimension : uint8
{
  Two,
  Three
};

/**
 * @class SpinLattice
 * @brief Manages neighborhood queries and spin-flip acceptance for one Potts-model lattice.
 */
class SpinLattice
{
public:
  /**
   * @brief Constructs a spin lattice that shares the execution random generator.
   * @param imageGeometry Image geometry that defines the lattice dimensions.
   * @param temperature Simulation temperature in kelvin.
   * @param periodicBoundaries Whether each boundary wraps to its opposite boundary.
   * @param featureIds Spin values that the lattice modifies in place.
   * @param mask Optional mask that restricts valid neighbors.
   * @param generator Random generator shared with site selection.
   */
  SpinLattice(const ImageGeom& imageGeometry, float64 temperature, bool periodicBoundaries, DataArray<int32>& featureIds, const MaskCompareUtilities::MaskCompare* mask, std::mt19937_64& generator)
  : m_Temperature(temperature)
  , m_PeriodicBoundaries(periodicBoundaries)
  , m_FeatureIds(featureIds)
  , m_Mask(mask)
  , m_Generator(generator)
  {
    determineDimensionality(imageGeometry);
    generateNeighborhood();
    m_KT = k_BoltzmannConstant * m_Temperature;
    m_Neighbors.reserve(m_Neighborhood.size());
  }

  /**
   * @brief Attempts one spin flip at the specified cell.
   * @param index Flat cell index.
   */
  void attemptFlip(usize index)
  {
    const int32 spin = m_FeatureIds[index];
    queryValidNeighbors(index);

    const auto sameSpinCount = std::count_if(m_Neighbors.cbegin(), m_Neighbors.cend(), [this, spin](usize neighborIndex) { return spin == m_FeatureIds[neighborIndex]; });
    if(sameSpinCount == m_Neighbors.size())
    {
      return;
    }

    std::uniform_int_distribution<usize> neighborDistribution(0, m_Neighbors.size() - 1);
    const usize randomNeighbor = m_Neighbors[neighborDistribution(m_Generator)];
    const int32 candidate = m_FeatureIds[randomNeighbor];
    if(candidate == 0 || spin == candidate)
    {
      return;
    }

    float64 deltaEnergy = 0.0;
    for(const usize neighborIndex : m_Neighbors)
    {
      const int32 neighborSpin = m_FeatureIds[neighborIndex];
      const float64 candidateMismatch = neighborSpin != candidate ? 1.0 : 0.0;
      const float64 currentMismatch = neighborSpin != spin ? 1.0 : 0.0;
      deltaEnergy += candidateMismatch - currentMismatch;
    }
    deltaEnergy *= 0.5;

    if(deltaEnergy <= 0.0 || m_AcceptanceDistribution(m_Generator) < std::exp(-deltaEnergy / m_KT))
    {
      m_FeatureIds[index] = candidate;
      m_TotalFlips++;
    }
  }

  usize totalFlips() const
  {
    return m_TotalFlips;
  }

private:
  void determineDimensionality(const ImageGeom& imageGeometry)
  {
    m_Dimensions = imageGeometry.getDimensions().toArray();
    m_DimensionType = Dimension::Three;
    if(std::find(m_Dimensions.cbegin(), m_Dimensions.cend(), 1) != m_Dimensions.cend())
    {
      m_DimensionType = Dimension::Two;
    }

    if(m_DimensionType == Dimension::Two)
    {
      // The remap preserves the flattened index and places the singleton axis last.
      if(m_Dimensions[0] == 1)
      {
        m_Dimensions[0] = m_Dimensions[1];
        m_Dimensions[1] = m_Dimensions[2];
        m_Dimensions[2] = 1;
      }
      else if(m_Dimensions[1] == 1)
      {
        m_Dimensions[1] = m_Dimensions[2];
        m_Dimensions[2] = 1;
      }
    }
  }

  void generateNeighborhood()
  {
    if(m_DimensionType == Dimension::Two)
    {
      m_Neighborhood = {{{1, 0, 0}}, {{-1, 0, 0}}, {{0, 1, 0}}, {{0, -1, 0}}, {{1, 1, 0}}, {{-1, 1, 0}}, {{1, -1, 0}}, {{-1, -1, 0}}};
      return;
    }

    m_Neighborhood = {{{1, 0, 0}},   {{-1, 0, 0}}, {{0, 1, 0}},  {{0, -1, 0}},  {{0, 0, 1}},   {{0, 0, -1}},  {{1, 1, 0}},   {{-1, 1, 0}},  {{1, -1, 0}},
                      {{-1, -1, 0}}, {{1, 0, 1}},  {{1, 0, -1}}, {{-1, 0, 1}},  {{-1, 0, -1}}, {{0, 1, 1}},   {{0, 1, -1}},  {{0, -1, 1}},  {{0, -1, -1}},
                      {{1, 1, 1}},   {{1, 1, -1}}, {{1, -1, 1}}, {{1, -1, -1}}, {{-1, 1, 1}},  {{-1, 1, -1}}, {{-1, -1, 1}}, {{-1, -1, -1}}};
  }

  void queryValidNeighbors(usize index)
  {
    m_Neighbors.clear();

    const usize x = index % m_Dimensions[0];
    const usize y = (index / m_Dimensions[0]) % m_Dimensions[1];
    const usize z = index / (m_Dimensions[0] * m_Dimensions[1]);

    for(const auto& neighbor : m_Neighborhood)
    {
      if(m_PeriodicBoundaries)
      {
        const usize modifiedX = applyModularOperation(x, neighbor[0], m_Dimensions[0]);
        const usize modifiedY = applyModularOperation(y, neighbor[1], m_Dimensions[1]);
        const usize modifiedZ = applyModularOperation(z, neighbor[2], m_Dimensions[2]);
        appendNeighborIfSelected(neighborIndex(modifiedX, modifiedY, modifiedZ));
        continue;
      }

      const int64 modifiedX = static_cast<int64>(x) + neighbor[0];
      const int64 modifiedY = static_cast<int64>(y) + neighbor[1];
      const int64 modifiedZ = static_cast<int64>(z) + neighbor[2];
      if(modifiedX >= 0 && modifiedX < static_cast<int64>(m_Dimensions[0]) && modifiedY >= 0 && modifiedY < static_cast<int64>(m_Dimensions[1]) && modifiedZ >= 0 &&
         modifiedZ < static_cast<int64>(m_Dimensions[2]))
      {
        appendNeighborIfSelected(neighborIndex(static_cast<usize>(modifiedX), static_cast<usize>(modifiedY), static_cast<usize>(modifiedZ)));
      }
    }
  }

  void appendNeighborIfSelected(usize neighborIndex)
  {
    if(m_Mask == nullptr || m_Mask->isTrue(neighborIndex))
    {
      m_Neighbors.push_back(neighborIndex);
    }
  }

  static usize modularSubtraction(usize value, usize offset, usize modulus)
  {
    if(value >= offset)
    {
      return value - offset;
    }
    return modulus - offset + value;
  }

  static usize modularAddition(usize value, usize offset, usize modulus)
  {
    if(offset == 0)
    {
      return value;
    }
    offset = modulus - offset;
    if(value >= offset)
    {
      return value - offset;
    }
    return modulus - offset + value;
  }

  static usize applyModularOperation(usize value, int8 offset, usize modulus)
  {
    if(offset < 0)
    {
      return modularSubtraction(value, static_cast<usize>(-offset), modulus);
    }
    return modularAddition(value, static_cast<usize>(offset), modulus);
  }

  usize neighborIndex(usize x, usize y, usize z) const
  {
    if(m_DimensionType == Dimension::Two)
    {
      return y * m_Dimensions[0] + x;
    }
    return z * m_Dimensions[1] * m_Dimensions[0] + y * m_Dimensions[0] + x;
  }

  float64 m_Temperature = 0.0;
  float64 m_KT = 0.0;
  bool m_PeriodicBoundaries = false;
  DataArray<int32>& m_FeatureIds;
  const MaskCompareUtilities::MaskCompare* m_Mask = nullptr;
  std::mt19937_64& m_Generator;
  std::uniform_real_distribution<float64> m_AcceptanceDistribution{0.0, 1.0};
  Neighborhood m_Neighborhood;
  std::vector<usize> m_Neighbors;
  std::array<usize, 3> m_Dimensions = {};
  Dimension m_DimensionType = Dimension::Three;
  usize m_TotalFlips = 0;
};
} // namespace

//------------------------------------------------------------------------------
PottsModel::PottsModel(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const PottsModelInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

//------------------------------------------------------------------------------
PottsModel::~PottsModel() noexcept = default;

//------------------------------------------------------------------------------
Result<> PottsModel::operator()()
{
  auto& featureIds = m_DataStructure.getDataRefAs<DataArray<int32>>(m_InputValues->FeatureIdsArrayPath);
  const usize totalCells = featureIds.getNumberOfTuples();

  std::unique_ptr<MaskCompareUtilities::MaskCompare> mask;
  if(m_InputValues->UseMask)
  {
    mask = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  }

  usize attemptsPerIteration = totalCells;
  usize eligibleCellCount = 0;
  if(mask != nullptr)
  {
    attemptsPerIteration = mask->countTrueValues();
    for(usize cellIndex = 0; cellIndex < totalCells; cellIndex++)
    {
      if(mask->isTrue(cellIndex) && featureIds[cellIndex] != 0)
      {
        eligibleCellCount++;
      }
    }
  }
  else
  {
    eligibleCellCount = std::count_if(featureIds.cbegin(), featureIds.cend(), [](int32 featureId) { return featureId != 0; });
  }

  if(eligibleCellCount == 0)
  {
    const std::string maskDescription = mask == nullptr ? "without a mask" : fmt::format("with the mask at path '{}'", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(k_NoEligibleCellsError, fmt::format("The 'Feature IDs' array at path '{}' has no nonzero cells that are eligible for selection {}.",
                                                               m_InputValues->FeatureIdsArrayPath.toString(), maskDescription));
  }

  const DataPath imageGeometryPath = m_InputValues->FeatureIdsArrayPath.getParent().getParent();
  const auto& imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  const auto imageDimensions = imageGeometry.getDimensions().toArray();
  if(std::count(imageDimensions.cbegin(), imageDimensions.cend(), 1) > 1)
  {
    return MakeErrorResult(k_InvalidDimensionalityError, fmt::format("The image geometry at path '{}' must be two-dimensional or three-dimensional. Its dimensions are [{}, {}, {}].",
                                                                     imageGeometryPath.toString(), imageDimensions[0], imageDimensions[1], imageDimensions[2]));
  }

  std::mt19937_64 generator(m_InputValues->SeedValue);
  std::uniform_int_distribution<usize> siteDistribution(0, totalCells - 1);
  SpinLattice lattice(imageGeometry, m_InputValues->Temperature, m_InputValues->PeriodicBoundaries, featureIds, mask.get(), generator);

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(attemptsPerIteration);

  for(int32 iteration = 0; iteration < m_InputValues->Iterations; iteration++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    progressHelper.resetProgress();
    auto progressMessenger = progressHelper.createProgressMessenger();
    for(usize attemptIndex = 0; attemptIndex < attemptsPerIteration; attemptIndex++)
    {
      usize currentCell = 0;
      do
      {
        currentCell = siteDistribution(generator);
      } while(featureIds[currentCell] == 0 || (mask != nullptr && !mask->isTrue(currentCell)));

      lattice.attemptFlip(currentCell);
      progressMessenger.sendProgressMessage(1, [&lattice, iteration, this](usize currentProgress, usize maxProgress) {
        const usize percentComplete = currentProgress * 100 / maxProgress;
        return fmt::format("Iteration {} of {} || {}% Completed || {} Total Flips", iteration + 1, m_InputValues->Iterations, percentComplete, lattice.totalFlips());
      });
    }
  }

  return {};
}
