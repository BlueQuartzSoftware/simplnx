#include "CombineTransformationMatrices.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <Eigen/Dense>

using namespace nx::core;

namespace
{
template <typename T, typename U>
Eigen::Matrix<U, 4, 4, Eigen::RowMajor> CreateEigenMatrix(const AbstractDataStore<T>& dataStore)
{
  Eigen::Matrix<U, 4, 4, Eigen::RowMajor> matrix;
  matrix.fill(0);
  matrix << dataStore[0], dataStore[1], dataStore[2], dataStore[3], dataStore[4], dataStore[5], dataStore[6], dataStore[7], dataStore[8], dataStore[9], dataStore[10], dataStore[11], dataStore[12],
      dataStore[13], dataStore[14], dataStore[15];

  return matrix;
}

struct MatrixOperationFunctor
{
  template <typename ScalarType>
  Result<> operator()(const IDataArray& array1, const IDataArray& array2, IDataArray& outputArray)
  {
    using MatrixType = Eigen::Matrix<ScalarType, 4, 4, Eigen::RowMajor>;
    using StoreType = AbstractDataStore<ScalarType>;
    const auto& array1StoreRef = array1.getIDataStoreRefAs<StoreType>();
    const auto& array2StoreRef = array2.getIDataStoreRefAs<StoreType>();

    MatrixType output;
    if constexpr(std::is_same_v<ScalarType, float32>)
    {
      // This exists because x64 and ARM architectures round floating point values
      // differently during Eigen matrix multiplication, and this makes sure that we
      // get consistent values on both architectures by doing the multiplication
      // using float64 matrices and then casting back down to float32
      using Float64MatrixType = Eigen::Matrix<float64, 4, 4, Eigen::RowMajor>;
      auto eigenMatrix1 = CreateEigenMatrix<float32, float64>(array1StoreRef);
      auto eigenMatrix2 = CreateEigenMatrix<float32, float64>(array2StoreRef);
      Float64MatrixType float64Output = eigenMatrix1 * eigenMatrix2;
      output = float64Output.cast<float32>();
    }
    else
    {
      auto eigenMatrix1 = CreateEigenMatrix<ScalarType, ScalarType>(array1StoreRef);
      auto eigenMatrix2 = CreateEigenMatrix<ScalarType, ScalarType>(array2StoreRef);
      output = eigenMatrix1 * eigenMatrix2;
    }

    auto& dataStore = outputArray.getIDataStoreRefAs<StoreType>();
    std::copy(output.data(), output.data() + output.size(), dataStore.begin());
    return {};
  }
};

} // namespace

// -----------------------------------------------------------------------------
CombineTransformationMatrices::CombineTransformationMatrices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             CombineTransformationMatricesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CombineTransformationMatrices::~CombineTransformationMatrices() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CombineTransformationMatrices::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CombineTransformationMatrices::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("CombineTransformationMatrices: Combining transformation matrices...");

  auto& outputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->OutputPath);
  auto pathsIter = m_InputValues->SelectedPaths.begin();

  const auto& array1 = m_DataStructure.getDataRefAs<IDataArray>(*pathsIter++);
  const auto& array2 = m_DataStructure.getDataRefAs<IDataArray>(*pathsIter++);
  if(array1.getDataType() != array2.getDataType())
  {
    return MakeErrorResult(-89750, "DataType mismatch");
  }
  // Combine first two matrices: second * first
  ExecuteDataFunction(MatrixOperationFunctor{}, array1.getDataType(), array2, array1, outputArray);

  for(; pathsIter != m_InputValues->SelectedPaths.end(); ++pathsIter)
  {
    const auto& arrayRef = m_DataStructure.getDataRefAs<IDataArray>(*pathsIter);
    ExecuteDataFunction(MatrixOperationFunctor{}, outputArray.getDataType(), arrayRef, outputArray, outputArray);
  }

  return {};
}
