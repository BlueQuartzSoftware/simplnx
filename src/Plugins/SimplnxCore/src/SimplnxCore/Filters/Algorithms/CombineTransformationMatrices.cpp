#include "CombineTransformationMatrices.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <Eigen/Dense>

using namespace nx::core;

namespace
{
template <typename T>
Eigen::Matrix<T, 4, 4, Eigen::RowMajor> CreateEigenMatrix(const AbstractDataStore<T>& dataStore)
{
  Eigen::Matrix<T, 4, 4, Eigen::RowMajor> matrix;
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

    auto eigenMatrix1 = CreateEigenMatrix<ScalarType>(array1StoreRef);
    auto eigenMatrix2 = CreateEigenMatrix<ScalarType>(array2StoreRef);

    MatrixType output = eigenMatrix1 * eigenMatrix2;

    auto& dataStore = outputArray.getIDataStoreRefAs<StoreType>();

    dataStore[0] = output(0, 0);
    dataStore[1] = output(0, 1);
    dataStore[2] = output(0, 2);
    dataStore[3] = output(0, 3);
    dataStore[4] = output(1, 0);
    dataStore[5] = output(1, 1);
    dataStore[6] = output(1, 2);
    dataStore[7] = output(1, 3);
    dataStore[8] = output(2, 0);
    dataStore[9] = output(2, 1);
    dataStore[10] = output(2, 2);
    dataStore[11] = output(2, 3);
    dataStore[12] = output(3, 0);
    dataStore[13] = output(3, 1);
    dataStore[14] = output(3, 2);
    dataStore[15] = output(3, 3);

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
