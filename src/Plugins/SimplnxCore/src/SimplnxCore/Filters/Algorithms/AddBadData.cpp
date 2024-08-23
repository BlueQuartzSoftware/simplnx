#include "AddBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <random>

using namespace nx::core;

namespace
{
struct InitializeTupleToZeroFunctor
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& arrayPath, usize index)
  {
    dataStructure.getDataAsUnsafe<DataArray<T>>(arrayPath)->initializeTuple(index, 0);
  }
};
} // namespace

// -----------------------------------------------------------------------------
AddBadData::AddBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AddBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AddBadData::~AddBadData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AddBadData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AddBadData::operator()()
{
  std::mt19937 generator(m_InputValues->SeedValue); // Standard mersenne_twister_engine seeded
  std::uniform_real_distribution<float32> distribution(0.0F, 1.0F);

  const auto& imgGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& GBEuclideanDistances = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->GBEuclideanDistancesArrayPath).getDataStoreRef();

  auto childArrayPaths = GetAllChildArrayDataPaths(m_DataStructure, imgGeom.getCellDataPath());
  auto voxelArrayPaths = childArrayPaths.has_value() ? childArrayPaths.value() : std::vector<DataPath>{};

  float32 random = 0.0f;
  const size_t totalPoints = GBEuclideanDistances.getSize();
  for(size_t i = 0; i < totalPoints; ++i)
  {
    if(m_InputValues->BoundaryNoise && GBEuclideanDistances[i] < 1)
    {
      random = distribution(generator);
      if(random < m_InputValues->BoundaryVolFraction)
      {
        for(const auto& voxelArrayPath : voxelArrayPaths)
        {
          ExecuteDataFunction(InitializeTupleToZeroFunctor{}, m_DataStructure.getDataAsUnsafe<IDataArray>(voxelArrayPath)->getDataType(), m_DataStructure, voxelArrayPath, i);
        }
      }
    }
    if(m_InputValues->PoissonNoise)
    {
      random = distribution(generator);
      if(random < m_InputValues->PoissonVolFraction)
      {
        for(const auto& voxelArrayPath : voxelArrayPaths)
        {
          ExecuteDataFunction(InitializeTupleToZeroFunctor{}, m_DataStructure.getDataAs<IDataArray>(voxelArrayPath)->getDataType(), m_DataStructure, voxelArrayPath, i);
        }
      }
    }
  }

  return {};
}
