#include "PhaseType.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
PhaseType::PhaseType() = default;

// -----------------------------------------------------------------------------
PhaseType::~PhaseType() = default;

// -----------------------------------------------------------------------------
std::vector<PhaseType::EnumType> PhaseType::ToVector(Types& types)
{
  std::vector<PhaseType::EnumType> vec(types.size());
  for(int i = 0; i < types.size(); i++)
  {
    vec[i] = static_cast<PhaseType::EnumType>(types[i]);
  }
  return vec;
}

// -----------------------------------------------------------------------------
PhaseType::Types PhaseType::FromVector(std::vector<PhaseType::EnumType>& vec)
{
  PhaseType::Types types(vec.size());
  for(int i = 0; i < vec.size(); i++)
  {
    types[i] = static_cast<PhaseType::Type>(vec[i]);
  }
  return types;
}

// -----------------------------------------------------------------------------
std::string PhaseType::PrimaryStr()
{
  return "Primary";
}

// -----------------------------------------------------------------------------
std::string PhaseType::PrecipitateStr()
{
  return "Precipitate";
}

// -----------------------------------------------------------------------------
std::string PhaseType::TransformationStr()
{
  return "Transformation";
}

// -----------------------------------------------------------------------------
std::string PhaseType::MatrixStr()
{
  return "Matrix";
}

// -----------------------------------------------------------------------------
std::string PhaseType::BoundaryStr()
{
  return "Boundary";
}

// -----------------------------------------------------------------------------
std::string PhaseType::UnknownStr()
{
  return "Unknown Phase Type";
}

// -----------------------------------------------------------------------------
std::string PhaseType::getPhaseTypeString(Type phaseType)
{
  switch(phaseType)
  {
  case Type::Primary:
    return PrimaryStr();
  case Type::Precipitate:
    return PrecipitateStr();
  case Type::Transformation:
    return TransformationStr();
  case Type::Matrix:
    return MatrixStr();
  case Type::Boundary:
    return BoundaryStr();
  case Type::Unknown:
    return UnknownStr();
  default:
    break;
  }
  return "Undefined Phase Type (Error)";
}

// -----------------------------------------------------------------------------
PhaseType::Type PhaseType::getPhaseType(const char* str)
{
  if(PrimaryStr() == str)
  {
    return Type::Primary;
  }
  if(PrecipitateStr() == str)
  {
    return Type::Precipitate;
  }
  if(TransformationStr() == str)
  {
    return Type::Transformation;
  }
  if(MatrixStr() == str)
  {
    return Type::Matrix;
  }
  if(BoundaryStr() == str)
  {
    return Type::Boundary;
  }
  return Type::Unknown;
}

// -----------------------------------------------------------------------------
void PhaseType::getPhaseTypeStrings(std::vector<std::string>& strings)
{
  strings.clear();
  strings.push_back(PrimaryStr());
  strings.push_back(PrecipitateStr());
  strings.push_back(TransformationStr());
  strings.push_back(MatrixStr());
  strings.push_back(BoundaryStr());
  strings.push_back(UnknownStr());
}

// -----------------------------------------------------------------------------
void PhaseType::getPhaseTypeEnums(std::vector<Type>& types)
{
  types.clear();
  types.push_back(Type::Primary);
  types.push_back(Type::Precipitate);
  types.push_back(Type::Transformation);
  types.push_back(Type::Matrix);
  types.push_back(Type::Boundary);
  types.push_back(Type::Unknown);
}

// -----------------------------------------------------------------------------
void PhaseType::getPhaseTypeMap(std::map<Type, std::string>& phaseMap)
{
  phaseMap.clear();
  phaseMap[Type::Primary] = PrimaryStr();
  phaseMap[Type::Precipitate] = PrecipitateStr();
  phaseMap[Type::Transformation] = TransformationStr();
  phaseMap[Type::Matrix] = MatrixStr();
  phaseMap[Type::Boundary] = BoundaryStr();
  phaseMap[Type::Unknown] = UnknownStr();
}
