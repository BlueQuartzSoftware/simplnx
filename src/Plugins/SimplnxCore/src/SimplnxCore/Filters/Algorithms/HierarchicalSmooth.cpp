// Hierarchical Smoothing Algorithm
// Original algorithm by Siddharth Maddali (2016-2018)
// Ported to simplnx filter framework

#include "HierarchicalSmooth.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/Sparse>

#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
// ============================================================================
// Type aliases matching the original HierarchicalSmooth types
// ============================================================================
using TrimeshType = Eigen::Array<int, Eigen::Dynamic, 3>;
using MeshnodeType = Eigen::Matrix<double, 3, Eigen::Dynamic>;
using FacelabelType = Eigen::Array<int, Eigen::Dynamic, 2>;
using NodetypeType = Eigen::Array<int, Eigen::Dynamic, 1>;
using IsSmoothedType = Eigen::Array<bool, Eigen::Dynamic, 1>;
using MatindexType = Eigen::Matrix<int, Eigen::Dynamic, 1>;
using EdgePairType = std::pair<int, int>;
using EdgeListType = std::vector<EdgePairType>;
using SpMatType = Eigen::SparseMatrix<double>;
using TripletType = Eigen::Triplet<double>;
using SmootherType = Eigen::ConjugateGradient<SpMatType, Eigen::Upper | Eigen::Lower>;

// Use std::map for the edge/boundary dictionaries to ensure deterministic iteration
// order across platforms and STL implementations (libc++ vs libstdc++). The algorithm
// is hierarchical and its boundary/free-boundary traversal order is result-affecting,
// so a hash-ordered container would produce non-reproducible cross-platform output.
template <typename T>
using OrderedEdgeDictType = std::map<EdgePairType, T>;

struct EdgeCount
{
  EdgePairType orig_pair;
  int count;
  EdgeCount(int x, int y)
  : orig_pair(std::make_pair(x, y))
  , count(1)
  {
  }
};

// ============================================================================
// Slice helpers
// ============================================================================

// Slice rows from a dense matrix: out = mat(rowIdx, :)
Eigen::MatrixXd SliceDenseRows(const Eigen::MatrixXd& mat, const MatindexType& rowIdx)
{
  int numRows = static_cast<int>(rowIdx.size());
  Eigen::MatrixXd result(numRows, mat.cols());
  for(int i = 0; i < numRows; i++)
  {
    result.row(i) = mat.row(rowIdx(i));
  }
  return result;
}

// Merge rows from source into target at specified row indices
void MergeDenseRows(const Eigen::MatrixXd& source, Eigen::MatrixXd& target, const MatindexType& locations)
{
  for(int i = 0; i < static_cast<int>(source.rows()); i++)
  {
    target.row(locations(i)) = source.row(i);
  }
}

// Slice rows and columns from a sparse matrix: out = mat(rowIdx, colIdx)
SpMatType SliceSparse(const SpMatType& mat, const MatindexType& rowIdx, const MatindexType& colIdx)
{
  int numRows = static_cast<int>(rowIdx.size());
  int numCols = static_cast<int>(colIdx.size());
  SpMatType result(numRows, numCols);

  std::unordered_map<int, int> rowMap;
  for(int i = 0; i < numRows; i++)
  {
    rowMap[rowIdx(i)] = i;
  }
  std::unordered_map<int, int> colMap;
  for(int i = 0; i < numCols; i++)
  {
    colMap[colIdx(i)] = i;
  }

  std::vector<TripletType> triplets;
  triplets.reserve(mat.nonZeros());
  for(int k = 0; k < mat.outerSize(); ++k)
  {
    auto colFound = colMap.find(k);
    if(colFound == colMap.end())
    {
      continue;
    }
    for(SpMatType::InnerIterator it(mat, k); it; ++it)
    {
      auto rowFound = rowMap.find(static_cast<int>(it.row()));
      if(rowFound != rowMap.end())
      {
        triplets.emplace_back(rowFound->second, colFound->second, it.value());
      }
    }
  }
  result.setFromTriplets(triplets.begin(), triplets.end());
  result.makeCompressed();
  return result;
}

// Slice columns from meshnode (3xN matrix): out = mat(:, colIdx)
MeshnodeType SliceMeshnodeCols(const MeshnodeType& mat, const MatindexType& colIdx)
{
  int numCols = static_cast<int>(colIdx.size());
  MeshnodeType result(3, numCols);
  for(int i = 0; i < numCols; i++)
  {
    result.col(i) = mat.col(colIdx(i));
  }
  return result;
}

// Slice rows from is_smoothed array: out = arr(rowIdx)
IsSmoothedType SliceIsSmoothed(const IsSmoothedType& arr, const MatindexType& idx)
{
  int n = static_cast<int>(idx.size());
  IsSmoothedType result(n);
  for(int i = 0; i < n; i++)
  {
    result(i) = arr(idx(i));
  }
  return result;
}

// ============================================================================
// Base utility functions (from HSmoothBase)
// ============================================================================

TrimeshType Ismember(const TrimeshType& array1, const std::vector<int>& array2)
{
  std::unordered_map<int, int> dict;
  for(int i = 0; i < static_cast<int>(array2.size()); i++)
  {
    dict.insert({array2[i], i});
  }
  TrimeshType newTri = TrimeshType::Zero(array1.rows(), array1.cols());
  for(int col = 0; col < array1.cols(); col++)
  {
    for(int row = 0; row < array1.rows(); row++)
    {
      auto got = dict.find(array1(row, col));
      newTri(row, col) = got->second;
    }
  }
  return newTri;
}

MatindexType Getindex(const std::vector<int>& fromThis)
{
  MatindexType idx(static_cast<int>(fromThis.size()));
  for(int i = 0; i < static_cast<int>(fromThis.size()); i++)
  {
    idx(i) = fromThis[i];
  }
  return idx;
}

MatindexType Getindex(const std::vector<int>& fromThis, const MatindexType& inThis)
{
  std::unordered_map<int, int> dict;
  for(int i = 0; i < inThis.rows(); i++)
  {
    dict.insert({inThis(i), i});
  }
  std::vector<int> vtemp;
  for(int fromThi : fromThis)
  {
    auto got = dict.find(fromThi);
    vtemp.push_back(got->second);
  }
  return Getindex(vtemp);
}

MatindexType Getcomplement(const MatindexType& nSet, int n)
{
  MatindexType nAll = -1 * MatindexType::Ones(n, 1);
  for(int i = 0; i < nSet.size(); i++)
  {
    nAll(nSet(i)) = nSet(i);
  }
  std::vector<int> nComplement;
  for(int i = 0; i < nAll.size(); i++)
  {
    if(nAll(i) < 0)
    {
      nComplement.push_back(i);
    }
  }
  return Getindex(nComplement);
}

MatindexType Matunion(const MatindexType& mat1, const MatindexType& mat2)
{
  std::vector<int> v;
  v.reserve(mat1.size());
  for(int i = 0; i < mat1.size(); i++)
  {
    v.push_back(mat1(i));
  }
  for(int i = 0; i < mat2.size(); i++)
  {
    v.push_back(mat2(i));
  }
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
  return Getindex(v);
}

void Merge(const MeshnodeType& source, MeshnodeType& target, const MatindexType& locations)
{
  for(int i = 0; i < source.cols(); i++)
  {
    target.col(locations(i)) = source.col(i);
  }
}

// ============================================================================
// Triangulation class (from HSmoothTri)
// ============================================================================

class Triangulation
{
public:
  Triangulation() = default;

  explicit Triangulation(TrimeshType& inTri)
  {
    auto [edges, freeBnd] = getEdges(inTri);
    m_FreeBoundary = freeBnd;
    differentiateFaces();
  }

  std::tuple<EdgeListType, EdgeListType> freeBoundary() const
  {
    return std::make_tuple(m_FreeBoundary, m_FreeBoundarySegments);
  }

  std::tuple<SpMatType, MatindexType> graphLaplacian() const
  {
    std::vector<TripletType> tripletList;
    tripletList.reserve(m_Unique.size() + 2 * m_Dict.size());
    for(const auto& it : m_Dict)
    {
      int l = it.first.first;
      int m = it.first.second;
      tripletList.emplace_back(l, m, -1.0);
      tripletList.emplace_back(m, l, -1.0);
    }
    for(int i = 0; i < static_cast<int>(m_DiagCount.size()); i++)
    {
      tripletList.emplace_back(i, i, m_DiagCount[i]);
    }

    SpMatType gl(static_cast<int>(m_Unique.size()), static_cast<int>(m_Unique.size()));
    gl.setFromTriplets(tripletList.begin(), tripletList.end());
    gl.makeCompressed();

    // Copy unique to matindex
    std::vector<int> uniqueCopy = m_Unique;
    return std::make_tuple(gl, Getindex(uniqueCopy));
  }

private:
  EdgeListType m_FreeBoundary;
  EdgeListType m_FreeBoundarySegments;
  std::vector<int> m_Unique;
  // Ordered (std::map) so freeBoundary iteration order is identical across platforms/STL
  // implementations. m_Dict order feeds the freeBoundary sequence -> chain-link/segment
  // assignment, so a hash-ordered map (libc++ vs libstdc++) caused cross-platform divergence.
  OrderedEdgeDictType<EdgeCount> m_Dict;
  std::vector<double> m_DiagCount;

  void differentiateFaces()
  {
    if(m_FreeBoundary.empty())
    {
      return;
    }
    int start = m_FreeBoundary[0].first;
    std::vector<int> thisSec{0};
    int n = 1;
    while(n < static_cast<int>(m_FreeBoundary.size()))
    {
      if(m_FreeBoundary[n].second == start)
      {
        thisSec.push_back(n);
        m_FreeBoundarySegments.emplace_back(thisSec[0], thisSec[1]);
        thisSec.clear();
      }
      else if(thisSec.empty())
      {
        start = m_FreeBoundary[n].first;
        thisSec.push_back(n);
      }
      n++;
    }
  }

  std::tuple<EdgeListType, EdgeListType> getEdges(const TrimeshType& inTri)
  {
    for(int i = 0; i < inTri.rows(); i++)
    {
      for(int j = 0; j < inTri.cols(); j++)
      {
        m_Unique.push_back(inTri(i, j));
      }
    }
    std::sort(m_Unique.begin(), m_Unique.end());
    m_Unique.erase(std::unique(m_Unique.begin(), m_Unique.end()), m_Unique.end());

    m_DiagCount = std::vector<double>(m_Unique.size(), 0.0);
    TrimeshType subTri = Ismember(inTri, m_Unique);

    EdgeListType edgeList;
    EdgeListType freeBoundary;

    for(int i = 0; i < subTri.rows(); i++)
    {
      for(int j = 0; j < 3; j++)
      {
        int l = (j + 3) % 3;
        int m = (j + 4) % 3;
        int thisRow = subTri(i, l);
        int thisCol = subTri(i, m);
        EdgePairType ep = std::make_pair(std::min(thisRow, thisCol), std::max(thisRow, thisCol));
        auto got = m_Dict.find(ep);
        if(got == m_Dict.end())
        {
          EdgeCount ec(m_Unique[thisRow], m_Unique[thisCol]);
          m_Dict.insert({ep, ec});
          m_DiagCount[thisRow] += 1.0;
          m_DiagCount[thisCol] += 1.0;
        }
        else
        {
          got->second.count++;
        }
      }
    }

    for(auto& it : m_Dict)
    {
      edgeList.push_back(it.second.orig_pair);
      if(it.second.count == 1)
      {
        freeBoundary.push_back(it.second.orig_pair);
      }
    }

    return std::make_tuple(edgeList, FastChainLinkSort(freeBoundary));
  }

  static EdgeListType FastChainLinkSort(const EdgeListType& inList)
  {
    // Use std::map for deterministic iteration order across platforms
    std::map<int, std::vector<int>> windingDict;
    for(const auto& i : inList)
    {
      int ltemp = i.first;
      int rtemp = i.second;
      auto got = windingDict.find(ltemp);
      if(got == windingDict.end())
      {
        windingDict.insert({ltemp, std::vector<int>{rtemp}});
      }
      else
      {
        got->second.push_back(rtemp);
      }
    }

    EdgeListType outList;
    auto it = windingDict.begin();
    while(!windingDict.empty())
    {
      int next = it->second.back();
      outList.emplace_back(it->first, next);
      it->second.pop_back();
      if(it->second.empty())
      {
        windingDict.erase(it);
      }
      it = windingDict.find(next);
      if(it == windingDict.end())
      {
        it = windingDict.begin();
      }
    }
    return outList;
  }
};

// ============================================================================
// Core smoothing functions (from HSmoothMain)
// ============================================================================

SpMatType Laplacian2D(int n, const std::string& type = "serial")
{
  std::vector<TripletType> tripletList;
  tripletList.reserve(3 * n);
  for(int i = 0; i < n; i++)
  {
    tripletList.emplace_back(i, i, -1.0);
    if(i != n - 1)
    {
      tripletList.emplace_back(i, i + 1, 1.0);
    }
  }
  SpMatType temp(n, n);
  temp.setFromTriplets(tripletList.begin(), tripletList.end());
  SpMatType lap = SpMatType(temp.transpose()) * temp;
  if(type == "serial")
  {
    lap.coeffRef(n - 1, n - 1) = 1.0;
  }
  else if(type == "cyclic")
  {
    lap.coeffRef(0, 0) = 2.0;
    lap.coeffRef(0, n - 1) = -1.0;
    lap.coeffRef(n - 1, 0) = -1.0;
  }
  lap.makeCompressed();
  return lap;
}

std::tuple<SpMatType, SpMatType> AnalyzeLaplacian(const SpMatType& gl)
{
  SpMatType d(gl.rows(), gl.cols());
  SpMatType a(gl.rows(), gl.cols());
  std::vector<TripletType> dt;
  std::vector<TripletType> at;

  for(int k = 0; k < gl.outerSize(); ++k)
  {
    for(SpMatType::InnerIterator it(gl, k); it; ++it)
    {
      if(it.value() < -0.5)
      {
        at.emplace_back(static_cast<int>(it.row()), static_cast<int>(it.col()), it.value());
      }
      else if(it.value() > 0.5)
      {
        dt.emplace_back(static_cast<int>(it.row()), static_cast<int>(it.col()), it.value());
      }
    }
  }
  d.setFromTriplets(dt.begin(), dt.end());
  a.setFromTriplets(at.begin(), at.end());
  d.makeCompressed();
  a.makeCompressed();
  return std::make_tuple(d, a);
}

std::tuple<SpMatType, Eigen::MatrixXd> GetDirichletBvp(const SpMatType& gl, const Eigen::MatrixXd& yIn, const MatindexType& nFixed, const MatindexType& nMobile)
{
  MatindexType nAll = Matunion(nFixed, nMobile);

  SpMatType glRed = SliceSparse(gl, nMobile, nMobile);
  SpMatType sm1 = SliceSparse(gl, nAll, nFixed);
  Eigen::MatrixXd sm2 = SliceDenseRows(yIn, nFixed);
  Eigen::MatrixXd sm3 = sm1 * sm2;
  Eigen::MatrixXd fConst = SliceDenseRows(sm3, nMobile);

  return std::make_tuple(glRed, fConst);
}

double GetObjFn(SmootherType& smth, double feps, const SpMatType& fSmallEye, const SpMatType& ltl, const Eigen::MatrixXd& ltk, const MatindexType& nMobile, const Eigen::MatrixXd& yMobile,
                const SpMatType& d, const Eigen::MatrixXd& ayIn, Eigen::MatrixXd& yOut)
{
  SpMatType bigA = (1.0 - feps) * fSmallEye + feps * ltl;
  Eigen::MatrixXd b = (1.0 - feps) * yMobile - feps * ltk;

  smth.compute(bigA);
  Eigen::MatrixXd ySmooth = smth.solve(b);

  MergeDenseRows(ySmooth, yOut, nMobile);

  Eigen::ArrayXXd yDeltaD = (d * yOut + ayIn).array();
  return (yDeltaD * yDeltaD).sum();
}

MeshnodeType Smooth(const MeshnodeType& nodesIn, const MatindexType& nFixed, SpMatType& gl, double fThresh = 0.001, int nIter = 53)
{
  MatindexType nMobile = Getcomplement(nFixed, static_cast<int>(gl.cols()));
  if(nMobile.size() == 0)
  {
    return nodesIn;
  }

  Eigen::MatrixXd data = nodesIn.transpose(); // Dense Nx3

  auto [glRed, fConst] = GetDirichletBvp(gl, data, nFixed, nMobile);
  auto [d, a] = AnalyzeLaplacian(gl);

  Eigen::MatrixXd ayIn = a * data;
  SpMatType fSmallEye(nMobile.size(), nMobile.size());
  fSmallEye.setIdentity();

  Eigen::MatrixXd yMobile = SliceDenseRows(data, nMobile);

  SpMatType ltl = SpMatType(glRed.transpose() * glRed);
  Eigen::MatrixXd ltk = glRed.transpose() * fConst;
  Eigen::MatrixXd yOut = data;

  SmootherType smth;

  double fEps = 0.5;
  double fStep = fEps / 2.0;
  int nCount = 1;

  double fobj1 = GetObjFn(smth, fEps, fSmallEye, ltl, ltk, nMobile, yMobile, d, ayIn, yOut);
  double fobj2 = GetObjFn(smth, fEps + fThresh, fSmallEye, ltl, ltk, nMobile, yMobile, d, ayIn, yOut);
  double fslope = (fobj2 - fobj1) / fThresh;

  // Adaptive bisection over the smoothing parameter fEps, matching the reference
  // MATLAB/Python implementation (Smooth.m / HierarchicalSmooth.py):
  //   - iterate while the objective slope is still steep (|fslope| > fThresh)
  //   - step fEps in the direction given by the sign of the slope
  //   - halve the step each iteration (true bisection)
  while(std::fabs(fslope) > fThresh && nCount < nIter)
  {
    if(fslope > 0.0)
    {
      fEps -= fStep;
    }
    else
    {
      fEps += fStep;
    }

    fStep /= 2.0;
    fobj1 = GetObjFn(smth, fEps, fSmallEye, ltl, ltk, nMobile, yMobile, d, ayIn, yOut);
    fobj2 = GetObjFn(smth, fEps + fThresh, fSmallEye, ltl, ltk, nMobile, yMobile, d, ayIn, yOut);
    fslope = (fobj2 - fobj1) / fThresh;
    nCount++;
  }

  return yOut.transpose(); // Nx3 -> 3xN
}

MeshnodeType SmoothWithType(const MeshnodeType& nodesIn, const std::string& type = "serial", double fThresh = 0.001, int nIter = 53)
{
  SpMatType lap = Laplacian2D(static_cast<int>(nodesIn.cols()), type);
  std::vector<int> vidx;
  if(type == "serial")
  {
    vidx = std::vector<int>{0, static_cast<int>(lap.cols() - 1)};
  }
  MatindexType nFixed = Getindex(vidx);
  return Smooth(nodesIn, nFixed, lap, fThresh, nIter);
}

// ============================================================================
// Volume Solver - the main algorithm orchestrator
// ============================================================================

struct VolumeSolverData
{
  IsSmoothedType status;
  TrimeshType mesh;
  MeshnodeType node;
  MeshnodeType node_smooth;
  FacelabelType label;
  NodetypeType type;
  int max_iterations{};
  double error{};
  double error_threshold{};
  OrderedEdgeDictType<std::vector<int>> boundary_dict;
};

void InitVolumeSolver(VolumeSolverData& vs, TrimeshType&& volumeMesh, MeshnodeType&& surfaceNodes, FacelabelType&& fLabels, NodetypeType&& nodeType, int nIterations, double errorThreshold)
{
  vs.mesh = std::move(volumeMesh);
  vs.node = std::move(surfaceNodes);
  vs.label = std::move(fLabels);
  vs.type = std::move(nodeType);
  vs.max_iterations = nIterations;
  vs.error_threshold = errorThreshold;

  vs.status = IsSmoothedType(vs.type.size());
  for(int i = 0; i < vs.type.size(); i++)
  {
    vs.status(i) = (vs.type(i) % 10 == 4); // quad junction points considered already smoothed
  }

  vs.node_smooth = vs.node;

  // Compute minimum edge length for error threshold
  vs.error = (vs.node.col(vs.mesh(0, 0)).array() - vs.node.col(vs.mesh(0, 1)).array()).matrix().norm();
  vs.error = std::min(vs.error, (vs.node.col(vs.mesh(0, 1)).array() - vs.node.col(vs.mesh(0, 2)).array()).matrix().norm());
  vs.error = std::sqrt(3.0 * vs.error * vs.error);

  // Build boundary dictionary and ensure consistent mesh handedness
  for(int i = 0; i < vs.label.rows(); i++)
  {
    int thisMin = std::min(vs.label(i, 0), vs.label(i, 1));
    int thisMax = std::max(vs.label(i, 0), vs.label(i, 1));
    if(vs.label(i, 0) == thisMin)
    {
      // Flip to ensure consistent handedness
      int temp = vs.mesh(i, 0);
      vs.mesh(i, 0) = vs.mesh(i, 1);
      vs.mesh(i, 1) = temp;
    }
    EdgePairType thisPair = std::make_pair(thisMin, thisMax);
    auto got = vs.boundary_dict.find(thisPair);
    if(got == vs.boundary_dict.end())
    {
      std::vector<int> v;
      v.push_back(i);
      vs.boundary_dict.insert({thisPair, v});
    }
    else
    {
      got->second.push_back(i);
    }
  }
}

TrimeshType SliceMesh(const VolumeSolverData& vs, const std::vector<int>& fromThesePatches)
{
  MatindexType patchIdx = Getindex(fromThesePatches);
  int numPatches = static_cast<int>(patchIdx.size());
  TrimeshType triSub(numPatches, 3);
  for(int i = 0; i < numPatches; i++)
  {
    triSub(i, 0) = vs.mesh(patchIdx(i), 0);
    triSub(i, 1) = vs.mesh(patchIdx(i), 1);
    triSub(i, 2) = vs.mesh(patchIdx(i), 2);
  }
  return triSub;
}

void MarkSectionAsComplete(VolumeSolverData& vs, const MatindexType& idx)
{
  for(int i = 0; i < idx.size(); i++)
  {
    vs.status(idx(i)) = true;
  }
}

Result<> RunHierarchicalSmooth(VolumeSolverData& vs, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  int boundaryCount = 1;
  int totalBoundaries = static_cast<int>(vs.boundary_dict.size());

  ThrottledMessageHandler progressThrottle(messageHandler, std::chrono::milliseconds(1000));

  for(auto it = vs.boundary_dict.begin(); it != vs.boundary_dict.end(); ++it)
  {
    if(shouldCancel)
    {
      return {};
    }

    progressThrottle.updateCount("Processing boundary", boundaryCount, totalBoundaries);

    TrimeshType triSub = SliceMesh(vs, it->second);
    Triangulation tri(triSub);

    auto [gl, nUniq] = tri.graphLaplacian();
    auto [fb, fbSec] = tri.freeBoundary();

    // Smooth each free boundary segment first
    for(auto& i : fbSec)
    {
      if(shouldCancel)
      {
        return {};
      }

      int start = i.first;
      int stop = i.second;
      int count = 0;
      for(count = start; count <= stop; count++)
      {
        if(vs.type(fb[count].first) % 10 == 4)
        {
          break;
        }
      }

      std::vector<int> vtemp;
      if(count > stop)
      {
        // No quad junctions in this free boundary - smooth without constraints
        for(count = start; count <= stop; count++)
        {
          vtemp.push_back(fb[count].first);
        }
        MatindexType thisFreeBoundaryIdx = Getindex(vtemp);
        MeshnodeType thisFreeBoundary = SliceMeshnodeCols(vs.node_smooth, thisFreeBoundaryIdx);
        MeshnodeType thisFreeBoundarySmooth = SmoothWithType(thisFreeBoundary, "cyclic");
        Merge(thisFreeBoundarySmooth, vs.node_smooth, thisFreeBoundaryIdx);
        MarkSectionAsComplete(vs, thisFreeBoundaryIdx);
      }
      else
      {
        // Triple line sections found - smooth separately
        vtemp.push_back(fb[count].first);
        int thisSize = 1 + (stop - start);
        for(int j = count + 1; j < 1 + count + thisSize; j++)
        {
          int effectiveJ = j % thisSize;
          vtemp.push_back(fb[effectiveJ].first);
          if(vs.type(fb[effectiveJ].first) % 10 == 4)
          {
            // Reached terminal quad point
            MatindexType thisTripleLineIndex = Getindex(vtemp);
            IsSmoothedType thisStatus = SliceIsSmoothed(vs.status, thisTripleLineIndex);
            if(!thisStatus.all())
            {
              MeshnodeType thisTripleLine = SliceMeshnodeCols(vs.node_smooth, thisTripleLineIndex);
              MeshnodeType thisTripleLineSmoothed = SmoothWithType(thisTripleLine);
              Merge(thisTripleLineSmoothed, vs.node_smooth, thisTripleLineIndex);
              MarkSectionAsComplete(vs, thisTripleLineIndex);
            }
            vtemp.clear();
            vtemp.push_back(fb[effectiveJ].first);
          }
        }
      }
    }

    if(shouldCancel)
    {
      return {};
    }

    // Smooth entire boundary subject to fixed triple points
    MeshnodeType boundaryNode = SliceMeshnodeCols(vs.node_smooth, nUniq);
    std::vector<int> fixed;
    for(auto& i : fb)
    {
      fixed.push_back(i.first);
    }
    MatindexType nFixed = Getindex(fixed, nUniq);
    MeshnodeType boundaryNodeSmooth = Smooth(boundaryNode, nFixed, gl);
    Merge(boundaryNodeSmooth, vs.node_smooth, nUniq);
    MarkSectionAsComplete(vs, nUniq);
    boundaryCount++;
  }

  // Validate results - reject nodes with excessive displacement
  Eigen::ArrayXXd fTemp = vs.node_smooth.array() - vs.node.array();
  Eigen::ArrayXXd fNorm = (fTemp * fTemp).colwise().sum().sqrt() / vs.error;
  int rejectedCount = 0;
  if((fNorm > vs.error_threshold).any())
  {
    for(int i = 0; i < vs.status.rows(); i++)
    {
      if(fNorm(0, i) > vs.error_threshold)
      {
        vs.status(i, 0) = false;
        vs.node_smooth.col(i) = vs.node.col(i); // reset to old values
        rejectedCount++;
      }
    }
  }

  if(rejectedCount > 0)
  {
    messageHandler.sendWarningMessage(fmt::format("{} of {} nodes not smoothed due to excessive displacement", rejectedCount, vs.node_smooth.cols()));
  }
  else
  {
    messageHandler.sendInfoMessage("All nodes smoothed successfully.");
  }

  return {};
}

} // anonymous namespace

// =============================================================================
// HierarchicalSmooth class implementation
// =============================================================================

HierarchicalSmooth::HierarchicalSmooth(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, HierarchicalSmoothInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

Result<> HierarchicalSmooth::operator()()
{
  // Get the triangle geometry
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->triangle_geometry_data_path);

  // Get data arrays
  const auto& nodeTypeArray = m_DataStructure.getDataRefAs<Int8ArrayType>(m_InputValues->node_type_array_path);
  const auto& faceLabelsArray = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->face_labels_array_path);

  // Get vertex and face data
  auto& verticesRef = triangleGeom.getVertices()->getDataStoreRef();
  const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();
  const auto& nodeTypeRef = nodeTypeArray.getDataStoreRef();
  const auto& faceLabelsRef = faceLabelsArray.getDataStoreRef();

  IGeometry::MeshIndexType numVertices = triangleGeom.getNumberOfVertices();
  IGeometry::MeshIndexType numFaces = triangleGeom.getNumberOfFaces();

  m_MessageHandler.sendInfoMessage(fmt::format("Hierarchical Smooth: {} vertices, {} faces", numVertices, numFaces));

  // Convert simplnx data to Eigen types
  // meshnode is 3 x N (each column is a vertex)
  MeshnodeType vertices(3, numVertices);
  for(IGeometry::MeshIndexType i = 0; i < numVertices; i++)
  {
    vertices(0, i) = static_cast<double>(verticesRef[3 * i + 0]);
    vertices(1, i) = static_cast<double>(verticesRef[3 * i + 1]);
    vertices(2, i) = static_cast<double>(verticesRef[3 * i + 2]);
  }

  // trimesh is N x 3 (each row is a triangle with 3 vertex indices)
  TrimeshType faces(numFaces, 3);
  for(IGeometry::MeshIndexType i = 0; i < numFaces; i++)
  {
    faces(i, 0) = static_cast<int>(facesRef[3 * i + 0]);
    faces(i, 1) = static_cast<int>(facesRef[3 * i + 1]);
    faces(i, 2) = static_cast<int>(facesRef[3 * i + 2]);
  }

  // facelabel is N x 2
  FacelabelType fLabels(numFaces, 2);
  for(IGeometry::MeshIndexType i = 0; i < numFaces; i++)
  {
    fLabels(i, 0) = static_cast<int>(faceLabelsRef[2 * i + 0]);
    fLabels(i, 1) = static_cast<int>(faceLabelsRef[2 * i + 1]);
  }

  // nodetype is N x 1
  NodetypeType nType(numVertices);
  for(IGeometry::MeshIndexType i = 0; i < numVertices; i++)
  {
    nType(i) = static_cast<int>(nodeTypeRef[i]);
  }

  // Initialize solver and run (move Eigen matrices to avoid copies)
  VolumeSolverData vs;
  InitVolumeSolver(vs, std::move(faces), std::move(vertices), std::move(fLabels), std::move(nType), m_InputValues->max_iterations, m_InputValues->error_threshold);

  Result<> result = RunHierarchicalSmooth(vs, m_ShouldCancel, m_MessageHandler);
  if(result.invalid())
  {
    return result;
  }

  // Write smoothed vertices back to the triangle geometry
  m_MessageHandler.sendInfoMessage("Writing smoothed vertices back to geometry...");
  for(IGeometry::MeshIndexType i = 0; i < numVertices; i++)
  {
    verticesRef[3 * i + 0] = static_cast<float32>(vs.node_smooth(0, i));
    verticesRef[3 * i + 1] = static_cast<float32>(vs.node_smooth(1, i));
    verticesRef[3 * i + 2] = static_cast<float32>(vs.node_smooth(2, i));
  }

  return {};
}
