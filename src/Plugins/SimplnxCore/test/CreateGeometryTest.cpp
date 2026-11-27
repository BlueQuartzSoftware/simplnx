#include "SimplnxCore/Filters/CreateGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateRectGridGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <limits>

using namespace nx::core;
namespace fs = std::filesystem;

static void CreateVerticesArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  Float32Array* verticesArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, name, {144}, {3}, parentId);
  (*verticesArrayPtr)[0] = -0.707107;
  (*verticesArrayPtr)[1] = -0.707107;
  (*verticesArrayPtr)[2] = 0;
  (*verticesArrayPtr)[3] = -0.599278;
  (*verticesArrayPtr)[4] = -0.800541;
  (*verticesArrayPtr)[5] = 0;
  (*verticesArrayPtr)[6] = -0.479249;
  (*verticesArrayPtr)[7] = -0.877679;
  (*verticesArrayPtr)[8] = 0;
  (*verticesArrayPtr)[9] = -0.349464;
  (*verticesArrayPtr)[10] = -0.93695;
  (*verticesArrayPtr)[11] = 0;
  (*verticesArrayPtr)[12] = -0.212565;
  (*verticesArrayPtr)[13] = -0.977147;
  (*verticesArrayPtr)[14] = 0;
  (*verticesArrayPtr)[15] = -0.0713392;
  (*verticesArrayPtr)[16] = -0.997452;
  (*verticesArrayPtr)[17] = 0;
  (*verticesArrayPtr)[18] = 0.0713392;
  (*verticesArrayPtr)[19] = -0.997452;
  (*verticesArrayPtr)[20] = 0;
  (*verticesArrayPtr)[21] = 0.212565;
  (*verticesArrayPtr)[22] = -0.977147;
  (*verticesArrayPtr)[23] = 0;
  (*verticesArrayPtr)[24] = 0.349464;
  (*verticesArrayPtr)[25] = -0.93695;
  (*verticesArrayPtr)[26] = 0;
  (*verticesArrayPtr)[27] = 0.479249;
  (*verticesArrayPtr)[28] = -0.877679;
  (*verticesArrayPtr)[29] = 0;
  (*verticesArrayPtr)[30] = 0.599277;
  (*verticesArrayPtr)[31] = -0.800541;
  (*verticesArrayPtr)[32] = 0;
  (*verticesArrayPtr)[33] = 0.707107;
  (*verticesArrayPtr)[34] = -0.707107;
  (*verticesArrayPtr)[35] = 0;
  (*verticesArrayPtr)[36] = -0.800541;
  (*verticesArrayPtr)[37] = -0.599278;
  (*verticesArrayPtr)[38] = 0;
  (*verticesArrayPtr)[39] = -0.667352;
  (*verticesArrayPtr)[40] = -0.667352;
  (*verticesArrayPtr)[41] = 0.330579;
  (*verticesArrayPtr)[42] = -0.541329;
  (*verticesArrayPtr)[43] = -0.773098;
  (*verticesArrayPtr)[44] = 0.330579;
  (*verticesArrayPtr)[45] = -0.398858;
  (*verticesArrayPtr)[46] = -0.855354;
  (*verticesArrayPtr)[47] = 0.330579;
  (*verticesArrayPtr)[48] = -0.244268;
  (*verticesArrayPtr)[49] = -0.91162;
  (*verticesArrayPtr)[50] = 0.330579;
  (*verticesArrayPtr)[51] = -0.0822558;
  (*verticesArrayPtr)[52] = -0.940187;
  (*verticesArrayPtr)[53] = 0.330579;
  (*verticesArrayPtr)[54] = 0.0822557;
  (*verticesArrayPtr)[55] = -0.940187;
  (*verticesArrayPtr)[56] = 0.330579;
  (*verticesArrayPtr)[57] = 0.244268;
  (*verticesArrayPtr)[58] = -0.91162;
  (*verticesArrayPtr)[59] = 0.330579;
  (*verticesArrayPtr)[60] = 0.398858;
  (*verticesArrayPtr)[61] = -0.855354;
  (*verticesArrayPtr)[62] = 0.330579;
  (*verticesArrayPtr)[63] = 0.541329;
  (*verticesArrayPtr)[64] = -0.773098;
  (*verticesArrayPtr)[65] = 0.330579;
  (*verticesArrayPtr)[66] = 0.667352;
  (*verticesArrayPtr)[67] = -0.667352;
  (*verticesArrayPtr)[68] = 0.330579;
  (*verticesArrayPtr)[69] = 0.800541;
  (*verticesArrayPtr)[70] = -0.599278;
  (*verticesArrayPtr)[71] = 0;
  (*verticesArrayPtr)[72] = -0.877679;
  (*verticesArrayPtr)[73] = -0.479249;
  (*verticesArrayPtr)[74] = 0;
  (*verticesArrayPtr)[75] = -0.773098;
  (*verticesArrayPtr)[76] = -0.541329;
  (*verticesArrayPtr)[77] = 0.330579;
  (*verticesArrayPtr)[78] = -0.568298;
  (*verticesArrayPtr)[79] = -0.568298;
  (*verticesArrayPtr)[80] = 0.595041;
  (*verticesArrayPtr)[81] = -0.427592;
  (*verticesArrayPtr)[82] = -0.680508;
  (*verticesArrayPtr)[83] = 0.595041;
  (*verticesArrayPtr)[84] = -0.265444;
  (*verticesArrayPtr)[85] = -0.758594;
  (*verticesArrayPtr)[86] = 0.595041;
  (*verticesArrayPtr)[87] = -0.0899854;
  (*verticesArrayPtr)[88] = -0.798642;
  (*verticesArrayPtr)[89] = 0.595041;
  (*verticesArrayPtr)[90] = 0.0899853;
  (*verticesArrayPtr)[91] = -0.798642;
  (*verticesArrayPtr)[92] = 0.595041;
  (*verticesArrayPtr)[93] = 0.265444;
  (*verticesArrayPtr)[94] = -0.758594;
  (*verticesArrayPtr)[95] = 0.595041;
  (*verticesArrayPtr)[96] = 0.427592;
  (*verticesArrayPtr)[97] = -0.680508;
  (*verticesArrayPtr)[98] = 0.595041;
  (*verticesArrayPtr)[99] = 0.568298;
  (*verticesArrayPtr)[100] = -0.568298;
  (*verticesArrayPtr)[101] = 0.595041;
  (*verticesArrayPtr)[102] = 0.773098;
  (*verticesArrayPtr)[103] = -0.541329;
  (*verticesArrayPtr)[104] = 0.330579;
  (*verticesArrayPtr)[105] = 0.877679;
  (*verticesArrayPtr)[106] = -0.479249;
  (*verticesArrayPtr)[107] = 0;
  (*verticesArrayPtr)[108] = -0.93695;
  (*verticesArrayPtr)[109] = -0.349464;
  (*verticesArrayPtr)[110] = 0;
  (*verticesArrayPtr)[111] = -0.855354;
  (*verticesArrayPtr)[112] = -0.398858;
  (*verticesArrayPtr)[113] = 0.330579;
  (*verticesArrayPtr)[114] = -0.680508;
  (*verticesArrayPtr)[115] = -0.427592;
  (*verticesArrayPtr)[116] = 0.595041;
  (*verticesArrayPtr)[117] = -0.430427;
  (*verticesArrayPtr)[118] = -0.430427;
  (*verticesArrayPtr)[119] = 0.793388;
  (*verticesArrayPtr)[120] = -0.276351;
  (*verticesArrayPtr)[121] = -0.54237;
  (*verticesArrayPtr)[122] = 0.793388;
  (*verticesArrayPtr)[123] = -0.0952242;
  (*verticesArrayPtr)[124] = -0.601221;
  (*verticesArrayPtr)[125] = 0.793388;
  (*verticesArrayPtr)[126] = 0.0952241;
  (*verticesArrayPtr)[127] = -0.601221;
  (*verticesArrayPtr)[128] = 0.793388;
  (*verticesArrayPtr)[129] = 0.276351;
  (*verticesArrayPtr)[130] = -0.54237;
  (*verticesArrayPtr)[131] = 0.793388;
  (*verticesArrayPtr)[132] = 0.430427;
  (*verticesArrayPtr)[133] = -0.430427;
  (*verticesArrayPtr)[134] = 0.793388;
  (*verticesArrayPtr)[135] = 0.680508;
  (*verticesArrayPtr)[136] = -0.427592;
  (*verticesArrayPtr)[137] = 0.595041;
  (*verticesArrayPtr)[138] = 0.855354;
  (*verticesArrayPtr)[139] = -0.398858;
  (*verticesArrayPtr)[140] = 0.330579;
  (*verticesArrayPtr)[141] = 0.93695;
  (*verticesArrayPtr)[142] = -0.349464;
  (*verticesArrayPtr)[143] = 0;
  (*verticesArrayPtr)[144] = -0.977147;
  (*verticesArrayPtr)[145] = -0.212565;
  (*verticesArrayPtr)[146] = 0;
  (*verticesArrayPtr)[147] = -0.91162;
  (*verticesArrayPtr)[148] = -0.244268;
  (*verticesArrayPtr)[149] = 0.330579;
  (*verticesArrayPtr)[150] = -0.758594;
  (*verticesArrayPtr)[151] = -0.265444;
  (*verticesArrayPtr)[152] = 0.595041;
  (*verticesArrayPtr)[153] = -0.54237;
  (*verticesArrayPtr)[154] = -0.276351;
  (*verticesArrayPtr)[155] = 0.793388;
  (*verticesArrayPtr)[156] = -0.267608;
  (*verticesArrayPtr)[157] = -0.267608;
  (*verticesArrayPtr)[158] = 0.92562;
  (*verticesArrayPtr)[159] = -0.0979513;
  (*verticesArrayPtr)[160] = -0.365559;
  (*verticesArrayPtr)[161] = 0.92562;
  (*verticesArrayPtr)[162] = 0.0979512;
  (*verticesArrayPtr)[163] = -0.365559;
  (*verticesArrayPtr)[164] = 0.92562;
  (*verticesArrayPtr)[165] = 0.267608;
  (*verticesArrayPtr)[166] = -0.267608;
  (*verticesArrayPtr)[167] = 0.92562;
  (*verticesArrayPtr)[168] = 0.54237;
  (*verticesArrayPtr)[169] = -0.276351;
  (*verticesArrayPtr)[170] = 0.793388;
  (*verticesArrayPtr)[171] = 0.758594;
  (*verticesArrayPtr)[172] = -0.265444;
  (*verticesArrayPtr)[173] = 0.595041;
  (*verticesArrayPtr)[174] = 0.91162;
  (*verticesArrayPtr)[175] = -0.244268;
  (*verticesArrayPtr)[176] = 0.330579;
  (*verticesArrayPtr)[177] = 0.977147;
  (*verticesArrayPtr)[178] = -0.212565;
  (*verticesArrayPtr)[179] = 0;
  (*verticesArrayPtr)[180] = -0.997452;
  (*verticesArrayPtr)[181] = -0.0713392;
  (*verticesArrayPtr)[182] = 0;
  (*verticesArrayPtr)[183] = -0.940187;
  (*verticesArrayPtr)[184] = -0.0822558;
  (*verticesArrayPtr)[185] = 0.330579;
  (*verticesArrayPtr)[186] = -0.798642;
  (*verticesArrayPtr)[187] = -0.0899854;
  (*verticesArrayPtr)[188] = 0.595041;
  (*verticesArrayPtr)[189] = -0.601221;
  (*verticesArrayPtr)[190] = -0.0952242;
  (*verticesArrayPtr)[191] = 0.793388;
  (*verticesArrayPtr)[192] = -0.365559;
  (*verticesArrayPtr)[193] = -0.0979513;
  (*verticesArrayPtr)[194] = 0.92562;
  (*verticesArrayPtr)[195] = -0.0907211;
  (*verticesArrayPtr)[196] = -0.0907211;
  (*verticesArrayPtr)[197] = 0.991736;
  (*verticesArrayPtr)[198] = 0.0907211;
  (*verticesArrayPtr)[199] = -0.0907212;
  (*verticesArrayPtr)[200] = 0.991736;
  (*verticesArrayPtr)[201] = 0.365559;
  (*verticesArrayPtr)[202] = -0.0979513;
  (*verticesArrayPtr)[203] = 0.92562;
  (*verticesArrayPtr)[204] = 0.601221;
  (*verticesArrayPtr)[205] = -0.0952242;
  (*verticesArrayPtr)[206] = 0.793388;
  (*verticesArrayPtr)[207] = 0.798642;
  (*verticesArrayPtr)[208] = -0.0899854;
  (*verticesArrayPtr)[209] = 0.595041;
  (*verticesArrayPtr)[210] = 0.940187;
  (*verticesArrayPtr)[211] = -0.0822558;
  (*verticesArrayPtr)[212] = 0.330579;
  (*verticesArrayPtr)[213] = 0.997452;
  (*verticesArrayPtr)[214] = -0.0713392;
  (*verticesArrayPtr)[215] = 0;
  (*verticesArrayPtr)[216] = -0.997452;
  (*verticesArrayPtr)[217] = 0.0713392;
  (*verticesArrayPtr)[218] = 0;
  (*verticesArrayPtr)[219] = -0.940187;
  (*verticesArrayPtr)[220] = 0.0822557;
  (*verticesArrayPtr)[221] = 0.330579;
  (*verticesArrayPtr)[222] = -0.798642;
  (*verticesArrayPtr)[223] = 0.0899853;
  (*verticesArrayPtr)[224] = 0.595041;
  (*verticesArrayPtr)[225] = -0.601221;
  (*verticesArrayPtr)[226] = 0.0952241;
  (*verticesArrayPtr)[227] = 0.793388;
  (*verticesArrayPtr)[228] = -0.365559;
  (*verticesArrayPtr)[229] = 0.0979512;
  (*verticesArrayPtr)[230] = 0.92562;
  (*verticesArrayPtr)[231] = -0.0907212;
  (*verticesArrayPtr)[232] = 0.0907211;
  (*verticesArrayPtr)[233] = 0.991736;
  (*verticesArrayPtr)[234] = 0.090721;
  (*verticesArrayPtr)[235] = 0.090721;
  (*verticesArrayPtr)[236] = 0.991736;
  (*verticesArrayPtr)[237] = 0.365559;
  (*verticesArrayPtr)[238] = 0.0979512;
  (*verticesArrayPtr)[239] = 0.92562;
  (*verticesArrayPtr)[240] = 0.601221;
  (*verticesArrayPtr)[241] = 0.0952241;
  (*verticesArrayPtr)[242] = 0.793388;
  (*verticesArrayPtr)[243] = 0.798642;
  (*verticesArrayPtr)[244] = 0.0899853;
  (*verticesArrayPtr)[245] = 0.595041;
  (*verticesArrayPtr)[246] = 0.940187;
  (*verticesArrayPtr)[247] = 0.0822557;
  (*verticesArrayPtr)[248] = 0.330579;
  (*verticesArrayPtr)[249] = 0.997452;
  (*verticesArrayPtr)[250] = 0.0713392;
  (*verticesArrayPtr)[251] = 0;
  (*verticesArrayPtr)[252] = -0.977147;
  (*verticesArrayPtr)[253] = 0.212565;
  (*verticesArrayPtr)[254] = 0;
  (*verticesArrayPtr)[255] = -0.91162;
  (*verticesArrayPtr)[256] = 0.244268;
  (*verticesArrayPtr)[257] = 0.330579;
  (*verticesArrayPtr)[258] = -0.758594;
  (*verticesArrayPtr)[259] = 0.265444;
  (*verticesArrayPtr)[260] = 0.595041;
  (*verticesArrayPtr)[261] = -0.54237;
  (*verticesArrayPtr)[262] = 0.276351;
  (*verticesArrayPtr)[263] = 0.793388;
  (*verticesArrayPtr)[264] = -0.267608;
  (*verticesArrayPtr)[265] = 0.267608;
  (*verticesArrayPtr)[266] = 0.92562;
  (*verticesArrayPtr)[267] = -0.0979513;
  (*verticesArrayPtr)[268] = 0.365559;
  (*verticesArrayPtr)[269] = 0.92562;
  (*verticesArrayPtr)[270] = 0.0979512;
  (*verticesArrayPtr)[271] = 0.365559;
  (*verticesArrayPtr)[272] = 0.92562;
  (*verticesArrayPtr)[273] = 0.267608;
  (*verticesArrayPtr)[274] = 0.267608;
  (*verticesArrayPtr)[275] = 0.92562;
  (*verticesArrayPtr)[276] = 0.54237;
  (*verticesArrayPtr)[277] = 0.276351;
  (*verticesArrayPtr)[278] = 0.793388;
  (*verticesArrayPtr)[279] = 0.758594;
  (*verticesArrayPtr)[280] = 0.265444;
  (*verticesArrayPtr)[281] = 0.595041;
  (*verticesArrayPtr)[282] = 0.91162;
  (*verticesArrayPtr)[283] = 0.244268;
  (*verticesArrayPtr)[284] = 0.330579;
  (*verticesArrayPtr)[285] = 0.977147;
  (*verticesArrayPtr)[286] = 0.212565;
  (*verticesArrayPtr)[287] = 0;
  (*verticesArrayPtr)[288] = -0.93695;
  (*verticesArrayPtr)[289] = 0.349464;
  (*verticesArrayPtr)[290] = 0;
  (*verticesArrayPtr)[291] = -0.855354;
  (*verticesArrayPtr)[292] = 0.398858;
  (*verticesArrayPtr)[293] = 0.330579;
  (*verticesArrayPtr)[294] = -0.680508;
  (*verticesArrayPtr)[295] = 0.427592;
  (*verticesArrayPtr)[296] = 0.595041;
  (*verticesArrayPtr)[297] = -0.430427;
  (*verticesArrayPtr)[298] = 0.430427;
  (*verticesArrayPtr)[299] = 0.793388;
  (*verticesArrayPtr)[300] = -0.276351;
  (*verticesArrayPtr)[301] = 0.54237;
  (*verticesArrayPtr)[302] = 0.793388;
  (*verticesArrayPtr)[303] = -0.0952242;
  (*verticesArrayPtr)[304] = 0.601221;
  (*verticesArrayPtr)[305] = 0.793388;
  (*verticesArrayPtr)[306] = 0.0952241;
  (*verticesArrayPtr)[307] = 0.601221;
  (*verticesArrayPtr)[308] = 0.793388;
  (*verticesArrayPtr)[309] = 0.276351;
  (*verticesArrayPtr)[310] = 0.54237;
  (*verticesArrayPtr)[311] = 0.793388;
  (*verticesArrayPtr)[312] = 0.430427;
  (*verticesArrayPtr)[313] = 0.430427;
  (*verticesArrayPtr)[314] = 0.793388;
  (*verticesArrayPtr)[315] = 0.680508;
  (*verticesArrayPtr)[316] = 0.427592;
  (*verticesArrayPtr)[317] = 0.595041;
  (*verticesArrayPtr)[318] = 0.855354;
  (*verticesArrayPtr)[319] = 0.398858;
  (*verticesArrayPtr)[320] = 0.330579;
  (*verticesArrayPtr)[321] = 0.93695;
  (*verticesArrayPtr)[322] = 0.349464;
  (*verticesArrayPtr)[323] = 0;
  (*verticesArrayPtr)[324] = -0.877679;
  (*verticesArrayPtr)[325] = 0.479249;
  (*verticesArrayPtr)[326] = 0;
  (*verticesArrayPtr)[327] = -0.773098;
  (*verticesArrayPtr)[328] = 0.541329;
  (*verticesArrayPtr)[329] = 0.330579;
  (*verticesArrayPtr)[330] = -0.568298;
  (*verticesArrayPtr)[331] = 0.568298;
  (*verticesArrayPtr)[332] = 0.595041;
  (*verticesArrayPtr)[333] = -0.427592;
  (*verticesArrayPtr)[334] = 0.680508;
  (*verticesArrayPtr)[335] = 0.595041;
  (*verticesArrayPtr)[336] = -0.265444;
  (*verticesArrayPtr)[337] = 0.758594;
  (*verticesArrayPtr)[338] = 0.595041;
  (*verticesArrayPtr)[339] = -0.0899854;
  (*verticesArrayPtr)[340] = 0.798642;
  (*verticesArrayPtr)[341] = 0.595041;
  (*verticesArrayPtr)[342] = 0.0899853;
  (*verticesArrayPtr)[343] = 0.798642;
  (*verticesArrayPtr)[344] = 0.595041;
  (*verticesArrayPtr)[345] = 0.265444;
  (*verticesArrayPtr)[346] = 0.758594;
  (*verticesArrayPtr)[347] = 0.595041;
  (*verticesArrayPtr)[348] = 0.427592;
  (*verticesArrayPtr)[349] = 0.680508;
  (*verticesArrayPtr)[350] = 0.595041;
  (*verticesArrayPtr)[351] = 0.568298;
  (*verticesArrayPtr)[352] = 0.568298;
  (*verticesArrayPtr)[353] = 0.595041;
  (*verticesArrayPtr)[354] = 0.773098;
  (*verticesArrayPtr)[355] = 0.541329;
  (*verticesArrayPtr)[356] = 0.330579;
  (*verticesArrayPtr)[357] = 0.877679;
  (*verticesArrayPtr)[358] = 0.479249;
  (*verticesArrayPtr)[359] = 0;
  (*verticesArrayPtr)[360] = -0.800541;
  (*verticesArrayPtr)[361] = 0.599277;
  (*verticesArrayPtr)[362] = 0;
  (*verticesArrayPtr)[363] = -0.667352;
  (*verticesArrayPtr)[364] = 0.667352;
  (*verticesArrayPtr)[365] = 0.330579;
  (*verticesArrayPtr)[366] = -0.541329;
  (*verticesArrayPtr)[367] = 0.773098;
  (*verticesArrayPtr)[368] = 0.330579;
  (*verticesArrayPtr)[369] = -0.398858;
  (*verticesArrayPtr)[370] = 0.855354;
  (*verticesArrayPtr)[371] = 0.330579;
  (*verticesArrayPtr)[372] = -0.244268;
  (*verticesArrayPtr)[373] = 0.91162;
  (*verticesArrayPtr)[374] = 0.330579;
  (*verticesArrayPtr)[375] = -0.0822558;
  (*verticesArrayPtr)[376] = 0.940187;
  (*verticesArrayPtr)[377] = 0.330579;
  (*verticesArrayPtr)[378] = 0.0822557;
  (*verticesArrayPtr)[379] = 0.940187;
  (*verticesArrayPtr)[380] = 0.330579;
  (*verticesArrayPtr)[381] = 0.244268;
  (*verticesArrayPtr)[382] = 0.91162;
  (*verticesArrayPtr)[383] = 0.330579;
  (*verticesArrayPtr)[384] = 0.398858;
  (*verticesArrayPtr)[385] = 0.855354;
  (*verticesArrayPtr)[386] = 0.330579;
  (*verticesArrayPtr)[387] = 0.541329;
  (*verticesArrayPtr)[388] = 0.773098;
  (*verticesArrayPtr)[389] = 0.330579;
  (*verticesArrayPtr)[390] = 0.667352;
  (*verticesArrayPtr)[391] = 0.667352;
  (*verticesArrayPtr)[392] = 0.330579;
  (*verticesArrayPtr)[393] = 0.800541;
  (*verticesArrayPtr)[394] = 0.599277;
  (*verticesArrayPtr)[395] = 0;
  (*verticesArrayPtr)[396] = -0.707107;
  (*verticesArrayPtr)[397] = 0.707107;
  (*verticesArrayPtr)[398] = 0;
  (*verticesArrayPtr)[399] = -0.599278;
  (*verticesArrayPtr)[400] = 0.800541;
  (*verticesArrayPtr)[401] = 0;
  (*verticesArrayPtr)[402] = -0.479249;
  (*verticesArrayPtr)[403] = 0.877679;
  (*verticesArrayPtr)[404] = 0;
  (*verticesArrayPtr)[405] = -0.349464;
  (*verticesArrayPtr)[406] = 0.93695;
  (*verticesArrayPtr)[407] = 0;
  (*verticesArrayPtr)[408] = -0.212565;
  (*verticesArrayPtr)[409] = 0.977147;
  (*verticesArrayPtr)[410] = 0;
  (*verticesArrayPtr)[411] = -0.0713392;
  (*verticesArrayPtr)[412] = 0.997452;
  (*verticesArrayPtr)[413] = 0;
  (*verticesArrayPtr)[414] = 0.0713392;
  (*verticesArrayPtr)[415] = 0.997452;
  (*verticesArrayPtr)[416] = 0;
  (*verticesArrayPtr)[417] = 0.212565;
  (*verticesArrayPtr)[418] = 0.977147;
  (*verticesArrayPtr)[419] = 0;
  (*verticesArrayPtr)[420] = 0.349464;
  (*verticesArrayPtr)[421] = 0.93695;
  (*verticesArrayPtr)[422] = 0;
  (*verticesArrayPtr)[423] = 0.479249;
  (*verticesArrayPtr)[424] = 0.877679;
  (*verticesArrayPtr)[425] = 0;
  (*verticesArrayPtr)[426] = 0.599277;
  (*verticesArrayPtr)[427] = 0.800541;
  (*verticesArrayPtr)[428] = 0;
  (*verticesArrayPtr)[429] = 0.707107;
  (*verticesArrayPtr)[430] = 0.707107;
  (*verticesArrayPtr)[431] = 0;
}

static void CreateEdgesArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  IGeometry::MeshIndexArrayType* edgesListArrayPtr = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, name, {264}, {2}, parentId);
  (*edgesListArrayPtr)[0] = 12;
  (*edgesListArrayPtr)[1] = 0;
  (*edgesListArrayPtr)[2] = 0;
  (*edgesListArrayPtr)[3] = 1;
  (*edgesListArrayPtr)[4] = 13;
  (*edgesListArrayPtr)[5] = 1;
  (*edgesListArrayPtr)[6] = 1;
  (*edgesListArrayPtr)[7] = 2;
  (*edgesListArrayPtr)[8] = 14;
  (*edgesListArrayPtr)[9] = 2;
  (*edgesListArrayPtr)[10] = 2;
  (*edgesListArrayPtr)[11] = 3;
  (*edgesListArrayPtr)[12] = 15;
  (*edgesListArrayPtr)[13] = 3;
  (*edgesListArrayPtr)[14] = 3;
  (*edgesListArrayPtr)[15] = 4;
  (*edgesListArrayPtr)[16] = 16;
  (*edgesListArrayPtr)[17] = 4;
  (*edgesListArrayPtr)[18] = 4;
  (*edgesListArrayPtr)[19] = 5;
  (*edgesListArrayPtr)[20] = 17;
  (*edgesListArrayPtr)[21] = 5;
  (*edgesListArrayPtr)[22] = 5;
  (*edgesListArrayPtr)[23] = 6;
  (*edgesListArrayPtr)[24] = 18;
  (*edgesListArrayPtr)[25] = 6;
  (*edgesListArrayPtr)[26] = 6;
  (*edgesListArrayPtr)[27] = 7;
  (*edgesListArrayPtr)[28] = 19;
  (*edgesListArrayPtr)[29] = 7;
  (*edgesListArrayPtr)[30] = 7;
  (*edgesListArrayPtr)[31] = 8;
  (*edgesListArrayPtr)[32] = 20;
  (*edgesListArrayPtr)[33] = 8;
  (*edgesListArrayPtr)[34] = 8;
  (*edgesListArrayPtr)[35] = 9;
  (*edgesListArrayPtr)[36] = 21;
  (*edgesListArrayPtr)[37] = 9;
  (*edgesListArrayPtr)[38] = 9;
  (*edgesListArrayPtr)[39] = 10;
  (*edgesListArrayPtr)[40] = 22;
  (*edgesListArrayPtr)[41] = 10;
  (*edgesListArrayPtr)[42] = 10;
  (*edgesListArrayPtr)[43] = 11;
  (*edgesListArrayPtr)[44] = 11;
  (*edgesListArrayPtr)[45] = 23;
  (*edgesListArrayPtr)[46] = 24;
  (*edgesListArrayPtr)[47] = 12;
  (*edgesListArrayPtr)[48] = 12;
  (*edgesListArrayPtr)[49] = 13;
  (*edgesListArrayPtr)[50] = 25;
  (*edgesListArrayPtr)[51] = 13;
  (*edgesListArrayPtr)[52] = 13;
  (*edgesListArrayPtr)[53] = 14;
  (*edgesListArrayPtr)[54] = 26;
  (*edgesListArrayPtr)[55] = 14;
  (*edgesListArrayPtr)[56] = 14;
  (*edgesListArrayPtr)[57] = 15;
  (*edgesListArrayPtr)[58] = 27;
  (*edgesListArrayPtr)[59] = 15;
  (*edgesListArrayPtr)[60] = 15;
  (*edgesListArrayPtr)[61] = 16;
  (*edgesListArrayPtr)[62] = 28;
  (*edgesListArrayPtr)[63] = 16;
  (*edgesListArrayPtr)[64] = 16;
  (*edgesListArrayPtr)[65] = 17;
  (*edgesListArrayPtr)[66] = 29;
  (*edgesListArrayPtr)[67] = 17;
  (*edgesListArrayPtr)[68] = 17;
  (*edgesListArrayPtr)[69] = 18;
  (*edgesListArrayPtr)[70] = 30;
  (*edgesListArrayPtr)[71] = 18;
  (*edgesListArrayPtr)[72] = 18;
  (*edgesListArrayPtr)[73] = 19;
  (*edgesListArrayPtr)[74] = 31;
  (*edgesListArrayPtr)[75] = 19;
  (*edgesListArrayPtr)[76] = 19;
  (*edgesListArrayPtr)[77] = 20;
  (*edgesListArrayPtr)[78] = 32;
  (*edgesListArrayPtr)[79] = 20;
  (*edgesListArrayPtr)[80] = 20;
  (*edgesListArrayPtr)[81] = 21;
  (*edgesListArrayPtr)[82] = 33;
  (*edgesListArrayPtr)[83] = 21;
  (*edgesListArrayPtr)[84] = 21;
  (*edgesListArrayPtr)[85] = 22;
  (*edgesListArrayPtr)[86] = 34;
  (*edgesListArrayPtr)[87] = 22;
  (*edgesListArrayPtr)[88] = 22;
  (*edgesListArrayPtr)[89] = 23;
  (*edgesListArrayPtr)[90] = 23;
  (*edgesListArrayPtr)[91] = 35;
  (*edgesListArrayPtr)[92] = 36;
  (*edgesListArrayPtr)[93] = 24;
  (*edgesListArrayPtr)[94] = 24;
  (*edgesListArrayPtr)[95] = 25;
  (*edgesListArrayPtr)[96] = 37;
  (*edgesListArrayPtr)[97] = 25;
  (*edgesListArrayPtr)[98] = 25;
  (*edgesListArrayPtr)[99] = 26;
  (*edgesListArrayPtr)[100] = 38;
  (*edgesListArrayPtr)[101] = 26;
  (*edgesListArrayPtr)[102] = 26;
  (*edgesListArrayPtr)[103] = 27;
  (*edgesListArrayPtr)[104] = 39;
  (*edgesListArrayPtr)[105] = 27;
  (*edgesListArrayPtr)[106] = 27;
  (*edgesListArrayPtr)[107] = 28;
  (*edgesListArrayPtr)[108] = 40;
  (*edgesListArrayPtr)[109] = 28;
  (*edgesListArrayPtr)[110] = 28;
  (*edgesListArrayPtr)[111] = 29;
  (*edgesListArrayPtr)[112] = 41;
  (*edgesListArrayPtr)[113] = 29;
  (*edgesListArrayPtr)[114] = 29;
  (*edgesListArrayPtr)[115] = 30;
  (*edgesListArrayPtr)[116] = 42;
  (*edgesListArrayPtr)[117] = 30;
  (*edgesListArrayPtr)[118] = 30;
  (*edgesListArrayPtr)[119] = 31;
  (*edgesListArrayPtr)[120] = 43;
  (*edgesListArrayPtr)[121] = 31;
  (*edgesListArrayPtr)[122] = 31;
  (*edgesListArrayPtr)[123] = 32;
  (*edgesListArrayPtr)[124] = 44;
  (*edgesListArrayPtr)[125] = 32;
  (*edgesListArrayPtr)[126] = 32;
  (*edgesListArrayPtr)[127] = 33;
  (*edgesListArrayPtr)[128] = 45;
  (*edgesListArrayPtr)[129] = 33;
  (*edgesListArrayPtr)[130] = 33;
  (*edgesListArrayPtr)[131] = 34;
  (*edgesListArrayPtr)[132] = 46;
  (*edgesListArrayPtr)[133] = 34;
  (*edgesListArrayPtr)[134] = 34;
  (*edgesListArrayPtr)[135] = 35;
  (*edgesListArrayPtr)[136] = 35;
  (*edgesListArrayPtr)[137] = 47;
  (*edgesListArrayPtr)[138] = 48;
  (*edgesListArrayPtr)[139] = 36;
  (*edgesListArrayPtr)[140] = 36;
  (*edgesListArrayPtr)[141] = 37;
  (*edgesListArrayPtr)[142] = 49;
  (*edgesListArrayPtr)[143] = 37;
  (*edgesListArrayPtr)[144] = 37;
  (*edgesListArrayPtr)[145] = 38;
  (*edgesListArrayPtr)[146] = 50;
  (*edgesListArrayPtr)[147] = 38;
  (*edgesListArrayPtr)[148] = 38;
  (*edgesListArrayPtr)[149] = 39;
  (*edgesListArrayPtr)[150] = 51;
  (*edgesListArrayPtr)[151] = 39;
  (*edgesListArrayPtr)[152] = 39;
  (*edgesListArrayPtr)[153] = 40;
  (*edgesListArrayPtr)[154] = 52;
  (*edgesListArrayPtr)[155] = 40;
  (*edgesListArrayPtr)[156] = 40;
  (*edgesListArrayPtr)[157] = 41;
  (*edgesListArrayPtr)[158] = 53;
  (*edgesListArrayPtr)[159] = 41;
  (*edgesListArrayPtr)[160] = 41;
  (*edgesListArrayPtr)[161] = 42;
  (*edgesListArrayPtr)[162] = 54;
  (*edgesListArrayPtr)[163] = 42;
  (*edgesListArrayPtr)[164] = 42;
  (*edgesListArrayPtr)[165] = 43;
  (*edgesListArrayPtr)[166] = 55;
  (*edgesListArrayPtr)[167] = 43;
  (*edgesListArrayPtr)[168] = 43;
  (*edgesListArrayPtr)[169] = 44;
  (*edgesListArrayPtr)[170] = 56;
  (*edgesListArrayPtr)[171] = 44;
  (*edgesListArrayPtr)[172] = 44;
  (*edgesListArrayPtr)[173] = 45;
  (*edgesListArrayPtr)[174] = 57;
  (*edgesListArrayPtr)[175] = 45;
  (*edgesListArrayPtr)[176] = 45;
  (*edgesListArrayPtr)[177] = 46;
  (*edgesListArrayPtr)[178] = 58;
  (*edgesListArrayPtr)[179] = 46;
  (*edgesListArrayPtr)[180] = 46;
  (*edgesListArrayPtr)[181] = 47;
  (*edgesListArrayPtr)[182] = 47;
  (*edgesListArrayPtr)[183] = 59;
  (*edgesListArrayPtr)[184] = 60;
  (*edgesListArrayPtr)[185] = 48;
  (*edgesListArrayPtr)[186] = 48;
  (*edgesListArrayPtr)[187] = 49;
  (*edgesListArrayPtr)[188] = 61;
  (*edgesListArrayPtr)[189] = 49;
  (*edgesListArrayPtr)[190] = 49;
  (*edgesListArrayPtr)[191] = 50;
  (*edgesListArrayPtr)[192] = 62;
  (*edgesListArrayPtr)[193] = 50;
  (*edgesListArrayPtr)[194] = 50;
  (*edgesListArrayPtr)[195] = 51;
  (*edgesListArrayPtr)[196] = 63;
  (*edgesListArrayPtr)[197] = 51;
  (*edgesListArrayPtr)[198] = 51;
  (*edgesListArrayPtr)[199] = 52;
  (*edgesListArrayPtr)[200] = 64;
  (*edgesListArrayPtr)[201] = 52;
  (*edgesListArrayPtr)[202] = 52;
  (*edgesListArrayPtr)[203] = 53;
  (*edgesListArrayPtr)[204] = 65;
  (*edgesListArrayPtr)[205] = 53;
  (*edgesListArrayPtr)[206] = 53;
  (*edgesListArrayPtr)[207] = 54;
  (*edgesListArrayPtr)[208] = 66;
  (*edgesListArrayPtr)[209] = 54;
  (*edgesListArrayPtr)[210] = 54;
  (*edgesListArrayPtr)[211] = 55;
  (*edgesListArrayPtr)[212] = 67;
  (*edgesListArrayPtr)[213] = 55;
  (*edgesListArrayPtr)[214] = 55;
  (*edgesListArrayPtr)[215] = 56;
  (*edgesListArrayPtr)[216] = 68;
  (*edgesListArrayPtr)[217] = 56;
  (*edgesListArrayPtr)[218] = 56;
  (*edgesListArrayPtr)[219] = 57;
  (*edgesListArrayPtr)[220] = 69;
  (*edgesListArrayPtr)[221] = 57;
  (*edgesListArrayPtr)[222] = 57;
  (*edgesListArrayPtr)[223] = 58;
  (*edgesListArrayPtr)[224] = 70;
  (*edgesListArrayPtr)[225] = 58;
  (*edgesListArrayPtr)[226] = 58;
  (*edgesListArrayPtr)[227] = 59;
  (*edgesListArrayPtr)[228] = 59;
  (*edgesListArrayPtr)[229] = 71;
  (*edgesListArrayPtr)[230] = 72;
  (*edgesListArrayPtr)[231] = 60;
  (*edgesListArrayPtr)[232] = 60;
  (*edgesListArrayPtr)[233] = 61;
  (*edgesListArrayPtr)[234] = 73;
  (*edgesListArrayPtr)[235] = 61;
  (*edgesListArrayPtr)[236] = 61;
  (*edgesListArrayPtr)[237] = 62;
  (*edgesListArrayPtr)[238] = 74;
  (*edgesListArrayPtr)[239] = 62;
  (*edgesListArrayPtr)[240] = 62;
  (*edgesListArrayPtr)[241] = 63;
  (*edgesListArrayPtr)[242] = 75;
  (*edgesListArrayPtr)[243] = 63;
  (*edgesListArrayPtr)[244] = 63;
  (*edgesListArrayPtr)[245] = 64;
  (*edgesListArrayPtr)[246] = 76;
  (*edgesListArrayPtr)[247] = 64;
  (*edgesListArrayPtr)[248] = 64;
  (*edgesListArrayPtr)[249] = 65;
  (*edgesListArrayPtr)[250] = 77;
  (*edgesListArrayPtr)[251] = 65;
  (*edgesListArrayPtr)[252] = 65;
  (*edgesListArrayPtr)[253] = 66;
  (*edgesListArrayPtr)[254] = 78;
  (*edgesListArrayPtr)[255] = 66;
  (*edgesListArrayPtr)[256] = 66;
  (*edgesListArrayPtr)[257] = 67;
  (*edgesListArrayPtr)[258] = 79;
  (*edgesListArrayPtr)[259] = 67;
  (*edgesListArrayPtr)[260] = 67;
  (*edgesListArrayPtr)[261] = 68;
  (*edgesListArrayPtr)[262] = 80;
  (*edgesListArrayPtr)[263] = 68;
  (*edgesListArrayPtr)[264] = 68;
  (*edgesListArrayPtr)[265] = 69;
  (*edgesListArrayPtr)[266] = 81;
  (*edgesListArrayPtr)[267] = 69;
  (*edgesListArrayPtr)[268] = 69;
  (*edgesListArrayPtr)[269] = 70;
  (*edgesListArrayPtr)[270] = 82;
  (*edgesListArrayPtr)[271] = 70;
  (*edgesListArrayPtr)[272] = 70;
  (*edgesListArrayPtr)[273] = 71;
  (*edgesListArrayPtr)[274] = 71;
  (*edgesListArrayPtr)[275] = 83;
  (*edgesListArrayPtr)[276] = 84;
  (*edgesListArrayPtr)[277] = 72;
  (*edgesListArrayPtr)[278] = 72;
  (*edgesListArrayPtr)[279] = 73;
  (*edgesListArrayPtr)[280] = 85;
  (*edgesListArrayPtr)[281] = 73;
  (*edgesListArrayPtr)[282] = 73;
  (*edgesListArrayPtr)[283] = 74;
  (*edgesListArrayPtr)[284] = 86;
  (*edgesListArrayPtr)[285] = 74;
  (*edgesListArrayPtr)[286] = 74;
  (*edgesListArrayPtr)[287] = 75;
  (*edgesListArrayPtr)[288] = 87;
  (*edgesListArrayPtr)[289] = 75;
  (*edgesListArrayPtr)[290] = 75;
  (*edgesListArrayPtr)[291] = 76;
  (*edgesListArrayPtr)[292] = 88;
  (*edgesListArrayPtr)[293] = 76;
  (*edgesListArrayPtr)[294] = 76;
  (*edgesListArrayPtr)[295] = 77;
  (*edgesListArrayPtr)[296] = 89;
  (*edgesListArrayPtr)[297] = 77;
  (*edgesListArrayPtr)[298] = 77;
  (*edgesListArrayPtr)[299] = 78;
  (*edgesListArrayPtr)[300] = 90;
  (*edgesListArrayPtr)[301] = 78;
  (*edgesListArrayPtr)[302] = 78;
  (*edgesListArrayPtr)[303] = 79;
  (*edgesListArrayPtr)[304] = 91;
  (*edgesListArrayPtr)[305] = 79;
  (*edgesListArrayPtr)[306] = 79;
  (*edgesListArrayPtr)[307] = 80;
  (*edgesListArrayPtr)[308] = 92;
  (*edgesListArrayPtr)[309] = 80;
  (*edgesListArrayPtr)[310] = 80;
  (*edgesListArrayPtr)[311] = 81;
  (*edgesListArrayPtr)[312] = 93;
  (*edgesListArrayPtr)[313] = 81;
  (*edgesListArrayPtr)[314] = 81;
  (*edgesListArrayPtr)[315] = 82;
  (*edgesListArrayPtr)[316] = 94;
  (*edgesListArrayPtr)[317] = 82;
  (*edgesListArrayPtr)[318] = 82;
  (*edgesListArrayPtr)[319] = 83;
  (*edgesListArrayPtr)[320] = 83;
  (*edgesListArrayPtr)[321] = 95;
  (*edgesListArrayPtr)[322] = 96;
  (*edgesListArrayPtr)[323] = 84;
  (*edgesListArrayPtr)[324] = 84;
  (*edgesListArrayPtr)[325] = 85;
  (*edgesListArrayPtr)[326] = 97;
  (*edgesListArrayPtr)[327] = 85;
  (*edgesListArrayPtr)[328] = 85;
  (*edgesListArrayPtr)[329] = 86;
  (*edgesListArrayPtr)[330] = 98;
  (*edgesListArrayPtr)[331] = 86;
  (*edgesListArrayPtr)[332] = 86;
  (*edgesListArrayPtr)[333] = 87;
  (*edgesListArrayPtr)[334] = 99;
  (*edgesListArrayPtr)[335] = 87;
  (*edgesListArrayPtr)[336] = 87;
  (*edgesListArrayPtr)[337] = 88;
  (*edgesListArrayPtr)[338] = 100;
  (*edgesListArrayPtr)[339] = 88;
  (*edgesListArrayPtr)[340] = 88;
  (*edgesListArrayPtr)[341] = 89;
  (*edgesListArrayPtr)[342] = 101;
  (*edgesListArrayPtr)[343] = 89;
  (*edgesListArrayPtr)[344] = 89;
  (*edgesListArrayPtr)[345] = 90;
  (*edgesListArrayPtr)[346] = 102;
  (*edgesListArrayPtr)[347] = 90;
  (*edgesListArrayPtr)[348] = 90;
  (*edgesListArrayPtr)[349] = 91;
  (*edgesListArrayPtr)[350] = 103;
  (*edgesListArrayPtr)[351] = 91;
  (*edgesListArrayPtr)[352] = 91;
  (*edgesListArrayPtr)[353] = 92;
  (*edgesListArrayPtr)[354] = 104;
  (*edgesListArrayPtr)[355] = 92;
  (*edgesListArrayPtr)[356] = 92;
  (*edgesListArrayPtr)[357] = 93;
  (*edgesListArrayPtr)[358] = 105;
  (*edgesListArrayPtr)[359] = 93;
  (*edgesListArrayPtr)[360] = 93;
  (*edgesListArrayPtr)[361] = 94;
  (*edgesListArrayPtr)[362] = 106;
  (*edgesListArrayPtr)[363] = 94;
  (*edgesListArrayPtr)[364] = 94;
  (*edgesListArrayPtr)[365] = 95;
  (*edgesListArrayPtr)[366] = 95;
  (*edgesListArrayPtr)[367] = 107;
  (*edgesListArrayPtr)[368] = 108;
  (*edgesListArrayPtr)[369] = 96;
  (*edgesListArrayPtr)[370] = 96;
  (*edgesListArrayPtr)[371] = 97;
  (*edgesListArrayPtr)[372] = 109;
  (*edgesListArrayPtr)[373] = 97;
  (*edgesListArrayPtr)[374] = 97;
  (*edgesListArrayPtr)[375] = 98;
  (*edgesListArrayPtr)[376] = 110;
  (*edgesListArrayPtr)[377] = 98;
  (*edgesListArrayPtr)[378] = 98;
  (*edgesListArrayPtr)[379] = 99;
  (*edgesListArrayPtr)[380] = 111;
  (*edgesListArrayPtr)[381] = 99;
  (*edgesListArrayPtr)[382] = 99;
  (*edgesListArrayPtr)[383] = 100;
  (*edgesListArrayPtr)[384] = 112;
  (*edgesListArrayPtr)[385] = 100;
  (*edgesListArrayPtr)[386] = 100;
  (*edgesListArrayPtr)[387] = 101;
  (*edgesListArrayPtr)[388] = 113;
  (*edgesListArrayPtr)[389] = 101;
  (*edgesListArrayPtr)[390] = 101;
  (*edgesListArrayPtr)[391] = 102;
  (*edgesListArrayPtr)[392] = 114;
  (*edgesListArrayPtr)[393] = 102;
  (*edgesListArrayPtr)[394] = 102;
  (*edgesListArrayPtr)[395] = 103;
  (*edgesListArrayPtr)[396] = 115;
  (*edgesListArrayPtr)[397] = 103;
  (*edgesListArrayPtr)[398] = 103;
  (*edgesListArrayPtr)[399] = 104;
  (*edgesListArrayPtr)[400] = 116;
  (*edgesListArrayPtr)[401] = 104;
  (*edgesListArrayPtr)[402] = 104;
  (*edgesListArrayPtr)[403] = 105;
  (*edgesListArrayPtr)[404] = 117;
  (*edgesListArrayPtr)[405] = 105;
  (*edgesListArrayPtr)[406] = 105;
  (*edgesListArrayPtr)[407] = 106;
  (*edgesListArrayPtr)[408] = 118;
  (*edgesListArrayPtr)[409] = 106;
  (*edgesListArrayPtr)[410] = 106;
  (*edgesListArrayPtr)[411] = 107;
  (*edgesListArrayPtr)[412] = 107;
  (*edgesListArrayPtr)[413] = 119;
  (*edgesListArrayPtr)[414] = 120;
  (*edgesListArrayPtr)[415] = 108;
  (*edgesListArrayPtr)[416] = 108;
  (*edgesListArrayPtr)[417] = 109;
  (*edgesListArrayPtr)[418] = 121;
  (*edgesListArrayPtr)[419] = 109;
  (*edgesListArrayPtr)[420] = 109;
  (*edgesListArrayPtr)[421] = 110;
  (*edgesListArrayPtr)[422] = 122;
  (*edgesListArrayPtr)[423] = 110;
  (*edgesListArrayPtr)[424] = 110;
  (*edgesListArrayPtr)[425] = 111;
  (*edgesListArrayPtr)[426] = 123;
  (*edgesListArrayPtr)[427] = 111;
  (*edgesListArrayPtr)[428] = 111;
  (*edgesListArrayPtr)[429] = 112;
  (*edgesListArrayPtr)[430] = 124;
  (*edgesListArrayPtr)[431] = 112;
  (*edgesListArrayPtr)[432] = 112;
  (*edgesListArrayPtr)[433] = 113;
  (*edgesListArrayPtr)[434] = 125;
  (*edgesListArrayPtr)[435] = 113;
  (*edgesListArrayPtr)[436] = 113;
  (*edgesListArrayPtr)[437] = 114;
  (*edgesListArrayPtr)[438] = 126;
  (*edgesListArrayPtr)[439] = 114;
  (*edgesListArrayPtr)[440] = 114;
  (*edgesListArrayPtr)[441] = 115;
  (*edgesListArrayPtr)[442] = 127;
  (*edgesListArrayPtr)[443] = 115;
  (*edgesListArrayPtr)[444] = 115;
  (*edgesListArrayPtr)[445] = 116;
  (*edgesListArrayPtr)[446] = 128;
  (*edgesListArrayPtr)[447] = 116;
  (*edgesListArrayPtr)[448] = 116;
  (*edgesListArrayPtr)[449] = 117;
  (*edgesListArrayPtr)[450] = 129;
  (*edgesListArrayPtr)[451] = 117;
  (*edgesListArrayPtr)[452] = 117;
  (*edgesListArrayPtr)[453] = 118;
  (*edgesListArrayPtr)[454] = 130;
  (*edgesListArrayPtr)[455] = 118;
  (*edgesListArrayPtr)[456] = 118;
  (*edgesListArrayPtr)[457] = 119;
  (*edgesListArrayPtr)[458] = 119;
  (*edgesListArrayPtr)[459] = 131;
  (*edgesListArrayPtr)[460] = 132;
  (*edgesListArrayPtr)[461] = 120;
  (*edgesListArrayPtr)[462] = 120;
  (*edgesListArrayPtr)[463] = 121;
  (*edgesListArrayPtr)[464] = 133;
  (*edgesListArrayPtr)[465] = 132;
  (*edgesListArrayPtr)[466] = 133;
  (*edgesListArrayPtr)[467] = 121;
  (*edgesListArrayPtr)[468] = 121;
  (*edgesListArrayPtr)[469] = 122;
  (*edgesListArrayPtr)[470] = 134;
  (*edgesListArrayPtr)[471] = 133;
  (*edgesListArrayPtr)[472] = 134;
  (*edgesListArrayPtr)[473] = 122;
  (*edgesListArrayPtr)[474] = 122;
  (*edgesListArrayPtr)[475] = 123;
  (*edgesListArrayPtr)[476] = 135;
  (*edgesListArrayPtr)[477] = 134;
  (*edgesListArrayPtr)[478] = 135;
  (*edgesListArrayPtr)[479] = 123;
  (*edgesListArrayPtr)[480] = 123;
  (*edgesListArrayPtr)[481] = 124;
  (*edgesListArrayPtr)[482] = 136;
  (*edgesListArrayPtr)[483] = 135;
  (*edgesListArrayPtr)[484] = 136;
  (*edgesListArrayPtr)[485] = 124;
  (*edgesListArrayPtr)[486] = 124;
  (*edgesListArrayPtr)[487] = 125;
  (*edgesListArrayPtr)[488] = 137;
  (*edgesListArrayPtr)[489] = 136;
  (*edgesListArrayPtr)[490] = 137;
  (*edgesListArrayPtr)[491] = 125;
  (*edgesListArrayPtr)[492] = 125;
  (*edgesListArrayPtr)[493] = 126;
  (*edgesListArrayPtr)[494] = 138;
  (*edgesListArrayPtr)[495] = 137;
  (*edgesListArrayPtr)[496] = 138;
  (*edgesListArrayPtr)[497] = 126;
  (*edgesListArrayPtr)[498] = 126;
  (*edgesListArrayPtr)[499] = 127;
  (*edgesListArrayPtr)[500] = 139;
  (*edgesListArrayPtr)[501] = 138;
  (*edgesListArrayPtr)[502] = 139;
  (*edgesListArrayPtr)[503] = 127;
  (*edgesListArrayPtr)[504] = 127;
  (*edgesListArrayPtr)[505] = 128;
  (*edgesListArrayPtr)[506] = 140;
  (*edgesListArrayPtr)[507] = 139;
  (*edgesListArrayPtr)[508] = 140;
  (*edgesListArrayPtr)[509] = 128;
  (*edgesListArrayPtr)[510] = 128;
  (*edgesListArrayPtr)[511] = 129;
  (*edgesListArrayPtr)[512] = 141;
  (*edgesListArrayPtr)[513] = 140;
  (*edgesListArrayPtr)[514] = 141;
  (*edgesListArrayPtr)[515] = 129;
  (*edgesListArrayPtr)[516] = 129;
  (*edgesListArrayPtr)[517] = 130;
  (*edgesListArrayPtr)[518] = 142;
  (*edgesListArrayPtr)[519] = 141;
  (*edgesListArrayPtr)[520] = 142;
  (*edgesListArrayPtr)[521] = 130;
  (*edgesListArrayPtr)[522] = 130;
  (*edgesListArrayPtr)[523] = 131;
  (*edgesListArrayPtr)[524] = 131;
  (*edgesListArrayPtr)[525] = 143;
  (*edgesListArrayPtr)[526] = 143;
  (*edgesListArrayPtr)[527] = 142;
}

static void CreateTrianglesArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  IGeometry::MeshIndexArrayType* trianglesListArrayPtr = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, name, {242}, {3}, parentId);
  (*trianglesListArrayPtr)[0] = 0;
  (*trianglesListArrayPtr)[1] = 1;
  (*trianglesListArrayPtr)[2] = 13;
  (*trianglesListArrayPtr)[3] = 0;
  (*trianglesListArrayPtr)[4] = 13;
  (*trianglesListArrayPtr)[5] = 12;
  (*trianglesListArrayPtr)[6] = 1;
  (*trianglesListArrayPtr)[7] = 2;
  (*trianglesListArrayPtr)[8] = 14;
  (*trianglesListArrayPtr)[9] = 1;
  (*trianglesListArrayPtr)[10] = 14;
  (*trianglesListArrayPtr)[11] = 13;
  (*trianglesListArrayPtr)[12] = 2;
  (*trianglesListArrayPtr)[13] = 3;
  (*trianglesListArrayPtr)[14] = 15;
  (*trianglesListArrayPtr)[15] = 2;
  (*trianglesListArrayPtr)[16] = 15;
  (*trianglesListArrayPtr)[17] = 14;
  (*trianglesListArrayPtr)[18] = 3;
  (*trianglesListArrayPtr)[19] = 4;
  (*trianglesListArrayPtr)[20] = 16;
  (*trianglesListArrayPtr)[21] = 3;
  (*trianglesListArrayPtr)[22] = 16;
  (*trianglesListArrayPtr)[23] = 15;
  (*trianglesListArrayPtr)[24] = 4;
  (*trianglesListArrayPtr)[25] = 5;
  (*trianglesListArrayPtr)[26] = 17;
  (*trianglesListArrayPtr)[27] = 4;
  (*trianglesListArrayPtr)[28] = 17;
  (*trianglesListArrayPtr)[29] = 16;
  (*trianglesListArrayPtr)[30] = 5;
  (*trianglesListArrayPtr)[31] = 6;
  (*trianglesListArrayPtr)[32] = 18;
  (*trianglesListArrayPtr)[33] = 5;
  (*trianglesListArrayPtr)[34] = 18;
  (*trianglesListArrayPtr)[35] = 17;
  (*trianglesListArrayPtr)[36] = 6;
  (*trianglesListArrayPtr)[37] = 7;
  (*trianglesListArrayPtr)[38] = 19;
  (*trianglesListArrayPtr)[39] = 6;
  (*trianglesListArrayPtr)[40] = 19;
  (*trianglesListArrayPtr)[41] = 18;
  (*trianglesListArrayPtr)[42] = 7;
  (*trianglesListArrayPtr)[43] = 8;
  (*trianglesListArrayPtr)[44] = 20;
  (*trianglesListArrayPtr)[45] = 7;
  (*trianglesListArrayPtr)[46] = 20;
  (*trianglesListArrayPtr)[47] = 19;
  (*trianglesListArrayPtr)[48] = 8;
  (*trianglesListArrayPtr)[49] = 9;
  (*trianglesListArrayPtr)[50] = 21;
  (*trianglesListArrayPtr)[51] = 8;
  (*trianglesListArrayPtr)[52] = 21;
  (*trianglesListArrayPtr)[53] = 20;
  (*trianglesListArrayPtr)[54] = 9;
  (*trianglesListArrayPtr)[55] = 10;
  (*trianglesListArrayPtr)[56] = 22;
  (*trianglesListArrayPtr)[57] = 9;
  (*trianglesListArrayPtr)[58] = 22;
  (*trianglesListArrayPtr)[59] = 21;
  (*trianglesListArrayPtr)[60] = 10;
  (*trianglesListArrayPtr)[61] = 11;
  (*trianglesListArrayPtr)[62] = 23;
  (*trianglesListArrayPtr)[63] = 10;
  (*trianglesListArrayPtr)[64] = 23;
  (*trianglesListArrayPtr)[65] = 22;
  (*trianglesListArrayPtr)[66] = 12;
  (*trianglesListArrayPtr)[67] = 13;
  (*trianglesListArrayPtr)[68] = 25;
  (*trianglesListArrayPtr)[69] = 12;
  (*trianglesListArrayPtr)[70] = 25;
  (*trianglesListArrayPtr)[71] = 24;
  (*trianglesListArrayPtr)[72] = 13;
  (*trianglesListArrayPtr)[73] = 14;
  (*trianglesListArrayPtr)[74] = 26;
  (*trianglesListArrayPtr)[75] = 13;
  (*trianglesListArrayPtr)[76] = 26;
  (*trianglesListArrayPtr)[77] = 25;
  (*trianglesListArrayPtr)[78] = 14;
  (*trianglesListArrayPtr)[79] = 15;
  (*trianglesListArrayPtr)[80] = 27;
  (*trianglesListArrayPtr)[81] = 14;
  (*trianglesListArrayPtr)[82] = 27;
  (*trianglesListArrayPtr)[83] = 26;
  (*trianglesListArrayPtr)[84] = 15;
  (*trianglesListArrayPtr)[85] = 16;
  (*trianglesListArrayPtr)[86] = 28;
  (*trianglesListArrayPtr)[87] = 15;
  (*trianglesListArrayPtr)[88] = 28;
  (*trianglesListArrayPtr)[89] = 27;
  (*trianglesListArrayPtr)[90] = 16;
  (*trianglesListArrayPtr)[91] = 17;
  (*trianglesListArrayPtr)[92] = 29;
  (*trianglesListArrayPtr)[93] = 16;
  (*trianglesListArrayPtr)[94] = 29;
  (*trianglesListArrayPtr)[95] = 28;
  (*trianglesListArrayPtr)[96] = 17;
  (*trianglesListArrayPtr)[97] = 18;
  (*trianglesListArrayPtr)[98] = 30;
  (*trianglesListArrayPtr)[99] = 17;
  (*trianglesListArrayPtr)[100] = 30;
  (*trianglesListArrayPtr)[101] = 29;
  (*trianglesListArrayPtr)[102] = 18;
  (*trianglesListArrayPtr)[103] = 19;
  (*trianglesListArrayPtr)[104] = 31;
  (*trianglesListArrayPtr)[105] = 18;
  (*trianglesListArrayPtr)[106] = 31;
  (*trianglesListArrayPtr)[107] = 30;
  (*trianglesListArrayPtr)[108] = 19;
  (*trianglesListArrayPtr)[109] = 20;
  (*trianglesListArrayPtr)[110] = 32;
  (*trianglesListArrayPtr)[111] = 19;
  (*trianglesListArrayPtr)[112] = 32;
  (*trianglesListArrayPtr)[113] = 31;
  (*trianglesListArrayPtr)[114] = 20;
  (*trianglesListArrayPtr)[115] = 21;
  (*trianglesListArrayPtr)[116] = 33;
  (*trianglesListArrayPtr)[117] = 20;
  (*trianglesListArrayPtr)[118] = 33;
  (*trianglesListArrayPtr)[119] = 32;
  (*trianglesListArrayPtr)[120] = 21;
  (*trianglesListArrayPtr)[121] = 22;
  (*trianglesListArrayPtr)[122] = 34;
  (*trianglesListArrayPtr)[123] = 21;
  (*trianglesListArrayPtr)[124] = 34;
  (*trianglesListArrayPtr)[125] = 33;
  (*trianglesListArrayPtr)[126] = 22;
  (*trianglesListArrayPtr)[127] = 23;
  (*trianglesListArrayPtr)[128] = 35;
  (*trianglesListArrayPtr)[129] = 22;
  (*trianglesListArrayPtr)[130] = 35;
  (*trianglesListArrayPtr)[131] = 34;
  (*trianglesListArrayPtr)[132] = 24;
  (*trianglesListArrayPtr)[133] = 25;
  (*trianglesListArrayPtr)[134] = 37;
  (*trianglesListArrayPtr)[135] = 24;
  (*trianglesListArrayPtr)[136] = 37;
  (*trianglesListArrayPtr)[137] = 36;
  (*trianglesListArrayPtr)[138] = 25;
  (*trianglesListArrayPtr)[139] = 26;
  (*trianglesListArrayPtr)[140] = 38;
  (*trianglesListArrayPtr)[141] = 25;
  (*trianglesListArrayPtr)[142] = 38;
  (*trianglesListArrayPtr)[143] = 37;
  (*trianglesListArrayPtr)[144] = 26;
  (*trianglesListArrayPtr)[145] = 27;
  (*trianglesListArrayPtr)[146] = 39;
  (*trianglesListArrayPtr)[147] = 26;
  (*trianglesListArrayPtr)[148] = 39;
  (*trianglesListArrayPtr)[149] = 38;
  (*trianglesListArrayPtr)[150] = 27;
  (*trianglesListArrayPtr)[151] = 28;
  (*trianglesListArrayPtr)[152] = 40;
  (*trianglesListArrayPtr)[153] = 27;
  (*trianglesListArrayPtr)[154] = 40;
  (*trianglesListArrayPtr)[155] = 39;
  (*trianglesListArrayPtr)[156] = 28;
  (*trianglesListArrayPtr)[157] = 29;
  (*trianglesListArrayPtr)[158] = 41;
  (*trianglesListArrayPtr)[159] = 28;
  (*trianglesListArrayPtr)[160] = 41;
  (*trianglesListArrayPtr)[161] = 40;
  (*trianglesListArrayPtr)[162] = 29;
  (*trianglesListArrayPtr)[163] = 30;
  (*trianglesListArrayPtr)[164] = 42;
  (*trianglesListArrayPtr)[165] = 29;
  (*trianglesListArrayPtr)[166] = 42;
  (*trianglesListArrayPtr)[167] = 41;
  (*trianglesListArrayPtr)[168] = 30;
  (*trianglesListArrayPtr)[169] = 31;
  (*trianglesListArrayPtr)[170] = 43;
  (*trianglesListArrayPtr)[171] = 30;
  (*trianglesListArrayPtr)[172] = 43;
  (*trianglesListArrayPtr)[173] = 42;
  (*trianglesListArrayPtr)[174] = 31;
  (*trianglesListArrayPtr)[175] = 32;
  (*trianglesListArrayPtr)[176] = 44;
  (*trianglesListArrayPtr)[177] = 31;
  (*trianglesListArrayPtr)[178] = 44;
  (*trianglesListArrayPtr)[179] = 43;
  (*trianglesListArrayPtr)[180] = 32;
  (*trianglesListArrayPtr)[181] = 33;
  (*trianglesListArrayPtr)[182] = 45;
  (*trianglesListArrayPtr)[183] = 32;
  (*trianglesListArrayPtr)[184] = 45;
  (*trianglesListArrayPtr)[185] = 44;
  (*trianglesListArrayPtr)[186] = 33;
  (*trianglesListArrayPtr)[187] = 34;
  (*trianglesListArrayPtr)[188] = 46;
  (*trianglesListArrayPtr)[189] = 33;
  (*trianglesListArrayPtr)[190] = 46;
  (*trianglesListArrayPtr)[191] = 45;
  (*trianglesListArrayPtr)[192] = 34;
  (*trianglesListArrayPtr)[193] = 35;
  (*trianglesListArrayPtr)[194] = 47;
  (*trianglesListArrayPtr)[195] = 34;
  (*trianglesListArrayPtr)[196] = 47;
  (*trianglesListArrayPtr)[197] = 46;
  (*trianglesListArrayPtr)[198] = 36;
  (*trianglesListArrayPtr)[199] = 37;
  (*trianglesListArrayPtr)[200] = 49;
  (*trianglesListArrayPtr)[201] = 36;
  (*trianglesListArrayPtr)[202] = 49;
  (*trianglesListArrayPtr)[203] = 48;
  (*trianglesListArrayPtr)[204] = 37;
  (*trianglesListArrayPtr)[205] = 38;
  (*trianglesListArrayPtr)[206] = 50;
  (*trianglesListArrayPtr)[207] = 37;
  (*trianglesListArrayPtr)[208] = 50;
  (*trianglesListArrayPtr)[209] = 49;
  (*trianglesListArrayPtr)[210] = 38;
  (*trianglesListArrayPtr)[211] = 39;
  (*trianglesListArrayPtr)[212] = 51;
  (*trianglesListArrayPtr)[213] = 38;
  (*trianglesListArrayPtr)[214] = 51;
  (*trianglesListArrayPtr)[215] = 50;
  (*trianglesListArrayPtr)[216] = 39;
  (*trianglesListArrayPtr)[217] = 40;
  (*trianglesListArrayPtr)[218] = 52;
  (*trianglesListArrayPtr)[219] = 39;
  (*trianglesListArrayPtr)[220] = 52;
  (*trianglesListArrayPtr)[221] = 51;
  (*trianglesListArrayPtr)[222] = 40;
  (*trianglesListArrayPtr)[223] = 41;
  (*trianglesListArrayPtr)[224] = 53;
  (*trianglesListArrayPtr)[225] = 40;
  (*trianglesListArrayPtr)[226] = 53;
  (*trianglesListArrayPtr)[227] = 52;
  (*trianglesListArrayPtr)[228] = 41;
  (*trianglesListArrayPtr)[229] = 42;
  (*trianglesListArrayPtr)[230] = 54;
  (*trianglesListArrayPtr)[231] = 41;
  (*trianglesListArrayPtr)[232] = 54;
  (*trianglesListArrayPtr)[233] = 53;
  (*trianglesListArrayPtr)[234] = 42;
  (*trianglesListArrayPtr)[235] = 43;
  (*trianglesListArrayPtr)[236] = 55;
  (*trianglesListArrayPtr)[237] = 42;
  (*trianglesListArrayPtr)[238] = 55;
  (*trianglesListArrayPtr)[239] = 54;
  (*trianglesListArrayPtr)[240] = 43;
  (*trianglesListArrayPtr)[241] = 44;
  (*trianglesListArrayPtr)[242] = 56;
  (*trianglesListArrayPtr)[243] = 43;
  (*trianglesListArrayPtr)[244] = 56;
  (*trianglesListArrayPtr)[245] = 55;
  (*trianglesListArrayPtr)[246] = 44;
  (*trianglesListArrayPtr)[247] = 45;
  (*trianglesListArrayPtr)[248] = 57;
  (*trianglesListArrayPtr)[249] = 44;
  (*trianglesListArrayPtr)[250] = 57;
  (*trianglesListArrayPtr)[251] = 56;
  (*trianglesListArrayPtr)[252] = 45;
  (*trianglesListArrayPtr)[253] = 46;
  (*trianglesListArrayPtr)[254] = 58;
  (*trianglesListArrayPtr)[255] = 45;
  (*trianglesListArrayPtr)[256] = 58;
  (*trianglesListArrayPtr)[257] = 57;
  (*trianglesListArrayPtr)[258] = 46;
  (*trianglesListArrayPtr)[259] = 47;
  (*trianglesListArrayPtr)[260] = 59;
  (*trianglesListArrayPtr)[261] = 46;
  (*trianglesListArrayPtr)[262] = 59;
  (*trianglesListArrayPtr)[263] = 58;
  (*trianglesListArrayPtr)[264] = 48;
  (*trianglesListArrayPtr)[265] = 49;
  (*trianglesListArrayPtr)[266] = 61;
  (*trianglesListArrayPtr)[267] = 48;
  (*trianglesListArrayPtr)[268] = 61;
  (*trianglesListArrayPtr)[269] = 60;
  (*trianglesListArrayPtr)[270] = 49;
  (*trianglesListArrayPtr)[271] = 50;
  (*trianglesListArrayPtr)[272] = 62;
  (*trianglesListArrayPtr)[273] = 49;
  (*trianglesListArrayPtr)[274] = 62;
  (*trianglesListArrayPtr)[275] = 61;
  (*trianglesListArrayPtr)[276] = 50;
  (*trianglesListArrayPtr)[277] = 51;
  (*trianglesListArrayPtr)[278] = 63;
  (*trianglesListArrayPtr)[279] = 50;
  (*trianglesListArrayPtr)[280] = 63;
  (*trianglesListArrayPtr)[281] = 62;
  (*trianglesListArrayPtr)[282] = 51;
  (*trianglesListArrayPtr)[283] = 52;
  (*trianglesListArrayPtr)[284] = 64;
  (*trianglesListArrayPtr)[285] = 51;
  (*trianglesListArrayPtr)[286] = 64;
  (*trianglesListArrayPtr)[287] = 63;
  (*trianglesListArrayPtr)[288] = 52;
  (*trianglesListArrayPtr)[289] = 53;
  (*trianglesListArrayPtr)[290] = 65;
  (*trianglesListArrayPtr)[291] = 52;
  (*trianglesListArrayPtr)[292] = 65;
  (*trianglesListArrayPtr)[293] = 64;
  (*trianglesListArrayPtr)[294] = 53;
  (*trianglesListArrayPtr)[295] = 54;
  (*trianglesListArrayPtr)[296] = 66;
  (*trianglesListArrayPtr)[297] = 53;
  (*trianglesListArrayPtr)[298] = 66;
  (*trianglesListArrayPtr)[299] = 65;
  (*trianglesListArrayPtr)[300] = 54;
  (*trianglesListArrayPtr)[301] = 55;
  (*trianglesListArrayPtr)[302] = 67;
  (*trianglesListArrayPtr)[303] = 54;
  (*trianglesListArrayPtr)[304] = 67;
  (*trianglesListArrayPtr)[305] = 66;
  (*trianglesListArrayPtr)[306] = 55;
  (*trianglesListArrayPtr)[307] = 56;
  (*trianglesListArrayPtr)[308] = 68;
  (*trianglesListArrayPtr)[309] = 55;
  (*trianglesListArrayPtr)[310] = 68;
  (*trianglesListArrayPtr)[311] = 67;
  (*trianglesListArrayPtr)[312] = 56;
  (*trianglesListArrayPtr)[313] = 57;
  (*trianglesListArrayPtr)[314] = 69;
  (*trianglesListArrayPtr)[315] = 56;
  (*trianglesListArrayPtr)[316] = 69;
  (*trianglesListArrayPtr)[317] = 68;
  (*trianglesListArrayPtr)[318] = 57;
  (*trianglesListArrayPtr)[319] = 58;
  (*trianglesListArrayPtr)[320] = 70;
  (*trianglesListArrayPtr)[321] = 57;
  (*trianglesListArrayPtr)[322] = 70;
  (*trianglesListArrayPtr)[323] = 69;
  (*trianglesListArrayPtr)[324] = 58;
  (*trianglesListArrayPtr)[325] = 59;
  (*trianglesListArrayPtr)[326] = 71;
  (*trianglesListArrayPtr)[327] = 58;
  (*trianglesListArrayPtr)[328] = 71;
  (*trianglesListArrayPtr)[329] = 70;
  (*trianglesListArrayPtr)[330] = 60;
  (*trianglesListArrayPtr)[331] = 61;
  (*trianglesListArrayPtr)[332] = 73;
  (*trianglesListArrayPtr)[333] = 60;
  (*trianglesListArrayPtr)[334] = 73;
  (*trianglesListArrayPtr)[335] = 72;
  (*trianglesListArrayPtr)[336] = 61;
  (*trianglesListArrayPtr)[337] = 62;
  (*trianglesListArrayPtr)[338] = 74;
  (*trianglesListArrayPtr)[339] = 61;
  (*trianglesListArrayPtr)[340] = 74;
  (*trianglesListArrayPtr)[341] = 73;
  (*trianglesListArrayPtr)[342] = 62;
  (*trianglesListArrayPtr)[343] = 63;
  (*trianglesListArrayPtr)[344] = 75;
  (*trianglesListArrayPtr)[345] = 62;
  (*trianglesListArrayPtr)[346] = 75;
  (*trianglesListArrayPtr)[347] = 74;
  (*trianglesListArrayPtr)[348] = 63;
  (*trianglesListArrayPtr)[349] = 64;
  (*trianglesListArrayPtr)[350] = 76;
  (*trianglesListArrayPtr)[351] = 63;
  (*trianglesListArrayPtr)[352] = 76;
  (*trianglesListArrayPtr)[353] = 75;
  (*trianglesListArrayPtr)[354] = 64;
  (*trianglesListArrayPtr)[355] = 65;
  (*trianglesListArrayPtr)[356] = 77;
  (*trianglesListArrayPtr)[357] = 64;
  (*trianglesListArrayPtr)[358] = 77;
  (*trianglesListArrayPtr)[359] = 76;
  (*trianglesListArrayPtr)[360] = 65;
  (*trianglesListArrayPtr)[361] = 66;
  (*trianglesListArrayPtr)[362] = 78;
  (*trianglesListArrayPtr)[363] = 65;
  (*trianglesListArrayPtr)[364] = 78;
  (*trianglesListArrayPtr)[365] = 77;
  (*trianglesListArrayPtr)[366] = 66;
  (*trianglesListArrayPtr)[367] = 67;
  (*trianglesListArrayPtr)[368] = 79;
  (*trianglesListArrayPtr)[369] = 66;
  (*trianglesListArrayPtr)[370] = 79;
  (*trianglesListArrayPtr)[371] = 78;
  (*trianglesListArrayPtr)[372] = 67;
  (*trianglesListArrayPtr)[373] = 68;
  (*trianglesListArrayPtr)[374] = 80;
  (*trianglesListArrayPtr)[375] = 67;
  (*trianglesListArrayPtr)[376] = 80;
  (*trianglesListArrayPtr)[377] = 79;
  (*trianglesListArrayPtr)[378] = 68;
  (*trianglesListArrayPtr)[379] = 69;
  (*trianglesListArrayPtr)[380] = 81;
  (*trianglesListArrayPtr)[381] = 68;
  (*trianglesListArrayPtr)[382] = 81;
  (*trianglesListArrayPtr)[383] = 80;
  (*trianglesListArrayPtr)[384] = 69;
  (*trianglesListArrayPtr)[385] = 70;
  (*trianglesListArrayPtr)[386] = 82;
  (*trianglesListArrayPtr)[387] = 69;
  (*trianglesListArrayPtr)[388] = 82;
  (*trianglesListArrayPtr)[389] = 81;
  (*trianglesListArrayPtr)[390] = 70;
  (*trianglesListArrayPtr)[391] = 71;
  (*trianglesListArrayPtr)[392] = 83;
  (*trianglesListArrayPtr)[393] = 70;
  (*trianglesListArrayPtr)[394] = 83;
  (*trianglesListArrayPtr)[395] = 82;
  (*trianglesListArrayPtr)[396] = 72;
  (*trianglesListArrayPtr)[397] = 73;
  (*trianglesListArrayPtr)[398] = 85;
  (*trianglesListArrayPtr)[399] = 72;
  (*trianglesListArrayPtr)[400] = 85;
  (*trianglesListArrayPtr)[401] = 84;
  (*trianglesListArrayPtr)[402] = 73;
  (*trianglesListArrayPtr)[403] = 74;
  (*trianglesListArrayPtr)[404] = 86;
  (*trianglesListArrayPtr)[405] = 73;
  (*trianglesListArrayPtr)[406] = 86;
  (*trianglesListArrayPtr)[407] = 85;
  (*trianglesListArrayPtr)[408] = 74;
  (*trianglesListArrayPtr)[409] = 75;
  (*trianglesListArrayPtr)[410] = 87;
  (*trianglesListArrayPtr)[411] = 74;
  (*trianglesListArrayPtr)[412] = 87;
  (*trianglesListArrayPtr)[413] = 86;
  (*trianglesListArrayPtr)[414] = 75;
  (*trianglesListArrayPtr)[415] = 76;
  (*trianglesListArrayPtr)[416] = 88;
  (*trianglesListArrayPtr)[417] = 75;
  (*trianglesListArrayPtr)[418] = 88;
  (*trianglesListArrayPtr)[419] = 87;
  (*trianglesListArrayPtr)[420] = 76;
  (*trianglesListArrayPtr)[421] = 77;
  (*trianglesListArrayPtr)[422] = 89;
  (*trianglesListArrayPtr)[423] = 76;
  (*trianglesListArrayPtr)[424] = 89;
  (*trianglesListArrayPtr)[425] = 88;
  (*trianglesListArrayPtr)[426] = 77;
  (*trianglesListArrayPtr)[427] = 78;
  (*trianglesListArrayPtr)[428] = 90;
  (*trianglesListArrayPtr)[429] = 77;
  (*trianglesListArrayPtr)[430] = 90;
  (*trianglesListArrayPtr)[431] = 89;
  (*trianglesListArrayPtr)[432] = 78;
  (*trianglesListArrayPtr)[433] = 79;
  (*trianglesListArrayPtr)[434] = 91;
  (*trianglesListArrayPtr)[435] = 78;
  (*trianglesListArrayPtr)[436] = 91;
  (*trianglesListArrayPtr)[437] = 90;
  (*trianglesListArrayPtr)[438] = 79;
  (*trianglesListArrayPtr)[439] = 80;
  (*trianglesListArrayPtr)[440] = 92;
  (*trianglesListArrayPtr)[441] = 79;
  (*trianglesListArrayPtr)[442] = 92;
  (*trianglesListArrayPtr)[443] = 91;
  (*trianglesListArrayPtr)[444] = 80;
  (*trianglesListArrayPtr)[445] = 81;
  (*trianglesListArrayPtr)[446] = 93;
  (*trianglesListArrayPtr)[447] = 80;
  (*trianglesListArrayPtr)[448] = 93;
  (*trianglesListArrayPtr)[449] = 92;
  (*trianglesListArrayPtr)[450] = 81;
  (*trianglesListArrayPtr)[451] = 82;
  (*trianglesListArrayPtr)[452] = 94;
  (*trianglesListArrayPtr)[453] = 81;
  (*trianglesListArrayPtr)[454] = 94;
  (*trianglesListArrayPtr)[455] = 93;
  (*trianglesListArrayPtr)[456] = 82;
  (*trianglesListArrayPtr)[457] = 83;
  (*trianglesListArrayPtr)[458] = 95;
  (*trianglesListArrayPtr)[459] = 82;
  (*trianglesListArrayPtr)[460] = 95;
  (*trianglesListArrayPtr)[461] = 94;
  (*trianglesListArrayPtr)[462] = 84;
  (*trianglesListArrayPtr)[463] = 85;
  (*trianglesListArrayPtr)[464] = 97;
  (*trianglesListArrayPtr)[465] = 84;
  (*trianglesListArrayPtr)[466] = 97;
  (*trianglesListArrayPtr)[467] = 96;
  (*trianglesListArrayPtr)[468] = 85;
  (*trianglesListArrayPtr)[469] = 86;
  (*trianglesListArrayPtr)[470] = 98;
  (*trianglesListArrayPtr)[471] = 85;
  (*trianglesListArrayPtr)[472] = 98;
  (*trianglesListArrayPtr)[473] = 97;
  (*trianglesListArrayPtr)[474] = 86;
  (*trianglesListArrayPtr)[475] = 87;
  (*trianglesListArrayPtr)[476] = 99;
  (*trianglesListArrayPtr)[477] = 86;
  (*trianglesListArrayPtr)[478] = 99;
  (*trianglesListArrayPtr)[479] = 98;
  (*trianglesListArrayPtr)[480] = 87;
  (*trianglesListArrayPtr)[481] = 88;
  (*trianglesListArrayPtr)[482] = 100;
  (*trianglesListArrayPtr)[483] = 87;
  (*trianglesListArrayPtr)[484] = 100;
  (*trianglesListArrayPtr)[485] = 99;
  (*trianglesListArrayPtr)[486] = 88;
  (*trianglesListArrayPtr)[487] = 89;
  (*trianglesListArrayPtr)[488] = 101;
  (*trianglesListArrayPtr)[489] = 88;
  (*trianglesListArrayPtr)[490] = 101;
  (*trianglesListArrayPtr)[491] = 100;
  (*trianglesListArrayPtr)[492] = 89;
  (*trianglesListArrayPtr)[493] = 90;
  (*trianglesListArrayPtr)[494] = 102;
  (*trianglesListArrayPtr)[495] = 89;
  (*trianglesListArrayPtr)[496] = 102;
  (*trianglesListArrayPtr)[497] = 101;
  (*trianglesListArrayPtr)[498] = 90;
  (*trianglesListArrayPtr)[499] = 91;
  (*trianglesListArrayPtr)[500] = 103;
  (*trianglesListArrayPtr)[501] = 90;
  (*trianglesListArrayPtr)[502] = 103;
  (*trianglesListArrayPtr)[503] = 102;
  (*trianglesListArrayPtr)[504] = 91;
  (*trianglesListArrayPtr)[505] = 92;
  (*trianglesListArrayPtr)[506] = 104;
  (*trianglesListArrayPtr)[507] = 91;
  (*trianglesListArrayPtr)[508] = 104;
  (*trianglesListArrayPtr)[509] = 103;
  (*trianglesListArrayPtr)[510] = 92;
  (*trianglesListArrayPtr)[511] = 93;
  (*trianglesListArrayPtr)[512] = 105;
  (*trianglesListArrayPtr)[513] = 92;
  (*trianglesListArrayPtr)[514] = 105;
  (*trianglesListArrayPtr)[515] = 104;
  (*trianglesListArrayPtr)[516] = 93;
  (*trianglesListArrayPtr)[517] = 94;
  (*trianglesListArrayPtr)[518] = 106;
  (*trianglesListArrayPtr)[519] = 93;
  (*trianglesListArrayPtr)[520] = 106;
  (*trianglesListArrayPtr)[521] = 105;
  (*trianglesListArrayPtr)[522] = 94;
  (*trianglesListArrayPtr)[523] = 95;
  (*trianglesListArrayPtr)[524] = 107;
  (*trianglesListArrayPtr)[525] = 94;
  (*trianglesListArrayPtr)[526] = 107;
  (*trianglesListArrayPtr)[527] = 106;
  (*trianglesListArrayPtr)[528] = 96;
  (*trianglesListArrayPtr)[529] = 97;
  (*trianglesListArrayPtr)[530] = 109;
  (*trianglesListArrayPtr)[531] = 96;
  (*trianglesListArrayPtr)[532] = 109;
  (*trianglesListArrayPtr)[533] = 108;
  (*trianglesListArrayPtr)[534] = 97;
  (*trianglesListArrayPtr)[535] = 98;
  (*trianglesListArrayPtr)[536] = 110;
  (*trianglesListArrayPtr)[537] = 97;
  (*trianglesListArrayPtr)[538] = 110;
  (*trianglesListArrayPtr)[539] = 109;
  (*trianglesListArrayPtr)[540] = 98;
  (*trianglesListArrayPtr)[541] = 99;
  (*trianglesListArrayPtr)[542] = 111;
  (*trianglesListArrayPtr)[543] = 98;
  (*trianglesListArrayPtr)[544] = 111;
  (*trianglesListArrayPtr)[545] = 110;
  (*trianglesListArrayPtr)[546] = 99;
  (*trianglesListArrayPtr)[547] = 100;
  (*trianglesListArrayPtr)[548] = 112;
  (*trianglesListArrayPtr)[549] = 99;
  (*trianglesListArrayPtr)[550] = 112;
  (*trianglesListArrayPtr)[551] = 111;
  (*trianglesListArrayPtr)[552] = 100;
  (*trianglesListArrayPtr)[553] = 101;
  (*trianglesListArrayPtr)[554] = 113;
  (*trianglesListArrayPtr)[555] = 100;
  (*trianglesListArrayPtr)[556] = 113;
  (*trianglesListArrayPtr)[557] = 112;
  (*trianglesListArrayPtr)[558] = 101;
  (*trianglesListArrayPtr)[559] = 102;
  (*trianglesListArrayPtr)[560] = 114;
  (*trianglesListArrayPtr)[561] = 101;
  (*trianglesListArrayPtr)[562] = 114;
  (*trianglesListArrayPtr)[563] = 113;
  (*trianglesListArrayPtr)[564] = 102;
  (*trianglesListArrayPtr)[565] = 103;
  (*trianglesListArrayPtr)[566] = 115;
  (*trianglesListArrayPtr)[567] = 102;
  (*trianglesListArrayPtr)[568] = 115;
  (*trianglesListArrayPtr)[569] = 114;
  (*trianglesListArrayPtr)[570] = 103;
  (*trianglesListArrayPtr)[571] = 104;
  (*trianglesListArrayPtr)[572] = 116;
  (*trianglesListArrayPtr)[573] = 103;
  (*trianglesListArrayPtr)[574] = 116;
  (*trianglesListArrayPtr)[575] = 115;
  (*trianglesListArrayPtr)[576] = 104;
  (*trianglesListArrayPtr)[577] = 105;
  (*trianglesListArrayPtr)[578] = 117;
  (*trianglesListArrayPtr)[579] = 104;
  (*trianglesListArrayPtr)[580] = 117;
  (*trianglesListArrayPtr)[581] = 116;
  (*trianglesListArrayPtr)[582] = 105;
  (*trianglesListArrayPtr)[583] = 106;
  (*trianglesListArrayPtr)[584] = 118;
  (*trianglesListArrayPtr)[585] = 105;
  (*trianglesListArrayPtr)[586] = 118;
  (*trianglesListArrayPtr)[587] = 117;
  (*trianglesListArrayPtr)[588] = 106;
  (*trianglesListArrayPtr)[589] = 107;
  (*trianglesListArrayPtr)[590] = 119;
  (*trianglesListArrayPtr)[591] = 106;
  (*trianglesListArrayPtr)[592] = 119;
  (*trianglesListArrayPtr)[593] = 118;
  (*trianglesListArrayPtr)[594] = 108;
  (*trianglesListArrayPtr)[595] = 109;
  (*trianglesListArrayPtr)[596] = 121;
  (*trianglesListArrayPtr)[597] = 108;
  (*trianglesListArrayPtr)[598] = 121;
  (*trianglesListArrayPtr)[599] = 120;
  (*trianglesListArrayPtr)[600] = 109;
  (*trianglesListArrayPtr)[601] = 110;
  (*trianglesListArrayPtr)[602] = 122;
  (*trianglesListArrayPtr)[603] = 109;
  (*trianglesListArrayPtr)[604] = 122;
  (*trianglesListArrayPtr)[605] = 121;
  (*trianglesListArrayPtr)[606] = 110;
  (*trianglesListArrayPtr)[607] = 111;
  (*trianglesListArrayPtr)[608] = 123;
  (*trianglesListArrayPtr)[609] = 110;
  (*trianglesListArrayPtr)[610] = 123;
  (*trianglesListArrayPtr)[611] = 122;
  (*trianglesListArrayPtr)[612] = 111;
  (*trianglesListArrayPtr)[613] = 112;
  (*trianglesListArrayPtr)[614] = 124;
  (*trianglesListArrayPtr)[615] = 111;
  (*trianglesListArrayPtr)[616] = 124;
  (*trianglesListArrayPtr)[617] = 123;
  (*trianglesListArrayPtr)[618] = 112;
  (*trianglesListArrayPtr)[619] = 113;
  (*trianglesListArrayPtr)[620] = 125;
  (*trianglesListArrayPtr)[621] = 112;
  (*trianglesListArrayPtr)[622] = 125;
  (*trianglesListArrayPtr)[623] = 124;
  (*trianglesListArrayPtr)[624] = 113;
  (*trianglesListArrayPtr)[625] = 114;
  (*trianglesListArrayPtr)[626] = 126;
  (*trianglesListArrayPtr)[627] = 113;
  (*trianglesListArrayPtr)[628] = 126;
  (*trianglesListArrayPtr)[629] = 125;
  (*trianglesListArrayPtr)[630] = 114;
  (*trianglesListArrayPtr)[631] = 115;
  (*trianglesListArrayPtr)[632] = 127;
  (*trianglesListArrayPtr)[633] = 114;
  (*trianglesListArrayPtr)[634] = 127;
  (*trianglesListArrayPtr)[635] = 126;
  (*trianglesListArrayPtr)[636] = 115;
  (*trianglesListArrayPtr)[637] = 116;
  (*trianglesListArrayPtr)[638] = 128;
  (*trianglesListArrayPtr)[639] = 115;
  (*trianglesListArrayPtr)[640] = 128;
  (*trianglesListArrayPtr)[641] = 127;
  (*trianglesListArrayPtr)[642] = 116;
  (*trianglesListArrayPtr)[643] = 117;
  (*trianglesListArrayPtr)[644] = 129;
  (*trianglesListArrayPtr)[645] = 116;
  (*trianglesListArrayPtr)[646] = 129;
  (*trianglesListArrayPtr)[647] = 128;
  (*trianglesListArrayPtr)[648] = 117;
  (*trianglesListArrayPtr)[649] = 118;
  (*trianglesListArrayPtr)[650] = 130;
  (*trianglesListArrayPtr)[651] = 117;
  (*trianglesListArrayPtr)[652] = 130;
  (*trianglesListArrayPtr)[653] = 129;
  (*trianglesListArrayPtr)[654] = 118;
  (*trianglesListArrayPtr)[655] = 119;
  (*trianglesListArrayPtr)[656] = 131;
  (*trianglesListArrayPtr)[657] = 118;
  (*trianglesListArrayPtr)[658] = 131;
  (*trianglesListArrayPtr)[659] = 130;
  (*trianglesListArrayPtr)[660] = 120;
  (*trianglesListArrayPtr)[661] = 121;
  (*trianglesListArrayPtr)[662] = 133;
  (*trianglesListArrayPtr)[663] = 120;
  (*trianglesListArrayPtr)[664] = 133;
  (*trianglesListArrayPtr)[665] = 132;
  (*trianglesListArrayPtr)[666] = 121;
  (*trianglesListArrayPtr)[667] = 122;
  (*trianglesListArrayPtr)[668] = 134;
  (*trianglesListArrayPtr)[669] = 121;
  (*trianglesListArrayPtr)[670] = 134;
  (*trianglesListArrayPtr)[671] = 133;
  (*trianglesListArrayPtr)[672] = 122;
  (*trianglesListArrayPtr)[673] = 123;
  (*trianglesListArrayPtr)[674] = 135;
  (*trianglesListArrayPtr)[675] = 122;
  (*trianglesListArrayPtr)[676] = 135;
  (*trianglesListArrayPtr)[677] = 134;
  (*trianglesListArrayPtr)[678] = 123;
  (*trianglesListArrayPtr)[679] = 124;
  (*trianglesListArrayPtr)[680] = 136;
  (*trianglesListArrayPtr)[681] = 123;
  (*trianglesListArrayPtr)[682] = 136;
  (*trianglesListArrayPtr)[683] = 135;
  (*trianglesListArrayPtr)[684] = 124;
  (*trianglesListArrayPtr)[685] = 125;
  (*trianglesListArrayPtr)[686] = 137;
  (*trianglesListArrayPtr)[687] = 124;
  (*trianglesListArrayPtr)[688] = 137;
  (*trianglesListArrayPtr)[689] = 136;
  (*trianglesListArrayPtr)[690] = 125;
  (*trianglesListArrayPtr)[691] = 126;
  (*trianglesListArrayPtr)[692] = 138;
  (*trianglesListArrayPtr)[693] = 125;
  (*trianglesListArrayPtr)[694] = 138;
  (*trianglesListArrayPtr)[695] = 137;
  (*trianglesListArrayPtr)[696] = 126;
  (*trianglesListArrayPtr)[697] = 127;
  (*trianglesListArrayPtr)[698] = 139;
  (*trianglesListArrayPtr)[699] = 126;
  (*trianglesListArrayPtr)[700] = 139;
  (*trianglesListArrayPtr)[701] = 138;
  (*trianglesListArrayPtr)[702] = 127;
  (*trianglesListArrayPtr)[703] = 128;
  (*trianglesListArrayPtr)[704] = 140;
  (*trianglesListArrayPtr)[705] = 127;
  (*trianglesListArrayPtr)[706] = 140;
  (*trianglesListArrayPtr)[707] = 139;
  (*trianglesListArrayPtr)[708] = 128;
  (*trianglesListArrayPtr)[709] = 129;
  (*trianglesListArrayPtr)[710] = 141;
  (*trianglesListArrayPtr)[711] = 128;
  (*trianglesListArrayPtr)[712] = 141;
  (*trianglesListArrayPtr)[713] = 140;
  (*trianglesListArrayPtr)[714] = 129;
  (*trianglesListArrayPtr)[715] = 130;
  (*trianglesListArrayPtr)[716] = 142;
  (*trianglesListArrayPtr)[717] = 129;
  (*trianglesListArrayPtr)[718] = 142;
  (*trianglesListArrayPtr)[719] = 141;
  (*trianglesListArrayPtr)[720] = 130;
  (*trianglesListArrayPtr)[721] = 131;
  (*trianglesListArrayPtr)[722] = 143;
  (*trianglesListArrayPtr)[723] = 130;
  (*trianglesListArrayPtr)[724] = 143;
  (*trianglesListArrayPtr)[725] = 142;
}

static void CreateQuadsArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  IGeometry::MeshIndexArrayType* quadsListArrayPtr = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, name, {121}, {4}, parentId);
  (*quadsListArrayPtr)[0] = 0;
  (*quadsListArrayPtr)[1] = 1;
  (*quadsListArrayPtr)[2] = 13;
  (*quadsListArrayPtr)[3] = 12;
  (*quadsListArrayPtr)[4] = 1;
  (*quadsListArrayPtr)[5] = 2;
  (*quadsListArrayPtr)[6] = 14;
  (*quadsListArrayPtr)[7] = 13;
  (*quadsListArrayPtr)[8] = 2;
  (*quadsListArrayPtr)[9] = 3;
  (*quadsListArrayPtr)[10] = 15;
  (*quadsListArrayPtr)[11] = 14;
  (*quadsListArrayPtr)[12] = 3;
  (*quadsListArrayPtr)[13] = 4;
  (*quadsListArrayPtr)[14] = 16;
  (*quadsListArrayPtr)[15] = 15;
  (*quadsListArrayPtr)[16] = 4;
  (*quadsListArrayPtr)[17] = 5;
  (*quadsListArrayPtr)[18] = 17;
  (*quadsListArrayPtr)[19] = 16;
  (*quadsListArrayPtr)[20] = 5;
  (*quadsListArrayPtr)[21] = 6;
  (*quadsListArrayPtr)[22] = 18;
  (*quadsListArrayPtr)[23] = 17;
  (*quadsListArrayPtr)[24] = 6;
  (*quadsListArrayPtr)[25] = 7;
  (*quadsListArrayPtr)[26] = 19;
  (*quadsListArrayPtr)[27] = 18;
  (*quadsListArrayPtr)[28] = 7;
  (*quadsListArrayPtr)[29] = 8;
  (*quadsListArrayPtr)[30] = 20;
  (*quadsListArrayPtr)[31] = 19;
  (*quadsListArrayPtr)[32] = 8;
  (*quadsListArrayPtr)[33] = 9;
  (*quadsListArrayPtr)[34] = 21;
  (*quadsListArrayPtr)[35] = 20;
  (*quadsListArrayPtr)[36] = 9;
  (*quadsListArrayPtr)[37] = 10;
  (*quadsListArrayPtr)[38] = 22;
  (*quadsListArrayPtr)[39] = 21;
  (*quadsListArrayPtr)[40] = 10;
  (*quadsListArrayPtr)[41] = 11;
  (*quadsListArrayPtr)[42] = 23;
  (*quadsListArrayPtr)[43] = 22;
  (*quadsListArrayPtr)[44] = 12;
  (*quadsListArrayPtr)[45] = 13;
  (*quadsListArrayPtr)[46] = 25;
  (*quadsListArrayPtr)[47] = 24;
  (*quadsListArrayPtr)[48] = 13;
  (*quadsListArrayPtr)[49] = 14;
  (*quadsListArrayPtr)[50] = 26;
  (*quadsListArrayPtr)[51] = 25;
  (*quadsListArrayPtr)[52] = 14;
  (*quadsListArrayPtr)[53] = 15;
  (*quadsListArrayPtr)[54] = 27;
  (*quadsListArrayPtr)[55] = 26;
  (*quadsListArrayPtr)[56] = 15;
  (*quadsListArrayPtr)[57] = 16;
  (*quadsListArrayPtr)[58] = 28;
  (*quadsListArrayPtr)[59] = 27;
  (*quadsListArrayPtr)[60] = 16;
  (*quadsListArrayPtr)[61] = 17;
  (*quadsListArrayPtr)[62] = 29;
  (*quadsListArrayPtr)[63] = 28;
  (*quadsListArrayPtr)[64] = 17;
  (*quadsListArrayPtr)[65] = 18;
  (*quadsListArrayPtr)[66] = 30;
  (*quadsListArrayPtr)[67] = 29;
  (*quadsListArrayPtr)[68] = 18;
  (*quadsListArrayPtr)[69] = 19;
  (*quadsListArrayPtr)[70] = 31;
  (*quadsListArrayPtr)[71] = 30;
  (*quadsListArrayPtr)[72] = 19;
  (*quadsListArrayPtr)[73] = 20;
  (*quadsListArrayPtr)[74] = 32;
  (*quadsListArrayPtr)[75] = 31;
  (*quadsListArrayPtr)[76] = 20;
  (*quadsListArrayPtr)[77] = 21;
  (*quadsListArrayPtr)[78] = 33;
  (*quadsListArrayPtr)[79] = 32;
  (*quadsListArrayPtr)[80] = 21;
  (*quadsListArrayPtr)[81] = 22;
  (*quadsListArrayPtr)[82] = 34;
  (*quadsListArrayPtr)[83] = 33;
  (*quadsListArrayPtr)[84] = 22;
  (*quadsListArrayPtr)[85] = 23;
  (*quadsListArrayPtr)[86] = 35;
  (*quadsListArrayPtr)[87] = 34;
  (*quadsListArrayPtr)[88] = 24;
  (*quadsListArrayPtr)[89] = 25;
  (*quadsListArrayPtr)[90] = 37;
  (*quadsListArrayPtr)[91] = 36;
  (*quadsListArrayPtr)[92] = 25;
  (*quadsListArrayPtr)[93] = 26;
  (*quadsListArrayPtr)[94] = 38;
  (*quadsListArrayPtr)[95] = 37;
  (*quadsListArrayPtr)[96] = 26;
  (*quadsListArrayPtr)[97] = 27;
  (*quadsListArrayPtr)[98] = 39;
  (*quadsListArrayPtr)[99] = 38;
  (*quadsListArrayPtr)[100] = 27;
  (*quadsListArrayPtr)[101] = 28;
  (*quadsListArrayPtr)[102] = 40;
  (*quadsListArrayPtr)[103] = 39;
  (*quadsListArrayPtr)[104] = 28;
  (*quadsListArrayPtr)[105] = 29;
  (*quadsListArrayPtr)[106] = 41;
  (*quadsListArrayPtr)[107] = 40;
  (*quadsListArrayPtr)[108] = 29;
  (*quadsListArrayPtr)[109] = 30;
  (*quadsListArrayPtr)[110] = 42;
  (*quadsListArrayPtr)[111] = 41;
  (*quadsListArrayPtr)[112] = 30;
  (*quadsListArrayPtr)[113] = 31;
  (*quadsListArrayPtr)[114] = 43;
  (*quadsListArrayPtr)[115] = 42;
  (*quadsListArrayPtr)[116] = 31;
  (*quadsListArrayPtr)[117] = 32;
  (*quadsListArrayPtr)[118] = 44;
  (*quadsListArrayPtr)[119] = 43;
  (*quadsListArrayPtr)[120] = 32;
  (*quadsListArrayPtr)[121] = 33;
  (*quadsListArrayPtr)[122] = 45;
  (*quadsListArrayPtr)[123] = 44;
  (*quadsListArrayPtr)[124] = 33;
  (*quadsListArrayPtr)[125] = 34;
  (*quadsListArrayPtr)[126] = 46;
  (*quadsListArrayPtr)[127] = 45;
  (*quadsListArrayPtr)[128] = 34;
  (*quadsListArrayPtr)[129] = 35;
  (*quadsListArrayPtr)[130] = 47;
  (*quadsListArrayPtr)[131] = 46;
  (*quadsListArrayPtr)[132] = 36;
  (*quadsListArrayPtr)[133] = 37;
  (*quadsListArrayPtr)[134] = 49;
  (*quadsListArrayPtr)[135] = 48;
  (*quadsListArrayPtr)[136] = 37;
  (*quadsListArrayPtr)[137] = 38;
  (*quadsListArrayPtr)[138] = 50;
  (*quadsListArrayPtr)[139] = 49;
  (*quadsListArrayPtr)[140] = 38;
  (*quadsListArrayPtr)[141] = 39;
  (*quadsListArrayPtr)[142] = 51;
  (*quadsListArrayPtr)[143] = 50;
  (*quadsListArrayPtr)[144] = 39;
  (*quadsListArrayPtr)[145] = 40;
  (*quadsListArrayPtr)[146] = 52;
  (*quadsListArrayPtr)[147] = 51;
  (*quadsListArrayPtr)[148] = 40;
  (*quadsListArrayPtr)[149] = 41;
  (*quadsListArrayPtr)[150] = 53;
  (*quadsListArrayPtr)[151] = 52;
  (*quadsListArrayPtr)[152] = 41;
  (*quadsListArrayPtr)[153] = 42;
  (*quadsListArrayPtr)[154] = 54;
  (*quadsListArrayPtr)[155] = 53;
  (*quadsListArrayPtr)[156] = 42;
  (*quadsListArrayPtr)[157] = 43;
  (*quadsListArrayPtr)[158] = 55;
  (*quadsListArrayPtr)[159] = 54;
  (*quadsListArrayPtr)[160] = 43;
  (*quadsListArrayPtr)[161] = 44;
  (*quadsListArrayPtr)[162] = 56;
  (*quadsListArrayPtr)[163] = 55;
  (*quadsListArrayPtr)[164] = 44;
  (*quadsListArrayPtr)[165] = 45;
  (*quadsListArrayPtr)[166] = 57;
  (*quadsListArrayPtr)[167] = 56;
  (*quadsListArrayPtr)[168] = 45;
  (*quadsListArrayPtr)[169] = 46;
  (*quadsListArrayPtr)[170] = 58;
  (*quadsListArrayPtr)[171] = 57;
  (*quadsListArrayPtr)[172] = 46;
  (*quadsListArrayPtr)[173] = 47;
  (*quadsListArrayPtr)[174] = 59;
  (*quadsListArrayPtr)[175] = 58;
  (*quadsListArrayPtr)[176] = 48;
  (*quadsListArrayPtr)[177] = 49;
  (*quadsListArrayPtr)[178] = 61;
  (*quadsListArrayPtr)[179] = 60;
  (*quadsListArrayPtr)[180] = 49;
  (*quadsListArrayPtr)[181] = 50;
  (*quadsListArrayPtr)[182] = 62;
  (*quadsListArrayPtr)[183] = 61;
  (*quadsListArrayPtr)[184] = 50;
  (*quadsListArrayPtr)[185] = 51;
  (*quadsListArrayPtr)[186] = 63;
  (*quadsListArrayPtr)[187] = 62;
  (*quadsListArrayPtr)[188] = 51;
  (*quadsListArrayPtr)[189] = 52;
  (*quadsListArrayPtr)[190] = 64;
  (*quadsListArrayPtr)[191] = 63;
  (*quadsListArrayPtr)[192] = 52;
  (*quadsListArrayPtr)[193] = 53;
  (*quadsListArrayPtr)[194] = 65;
  (*quadsListArrayPtr)[195] = 64;
  (*quadsListArrayPtr)[196] = 53;
  (*quadsListArrayPtr)[197] = 54;
  (*quadsListArrayPtr)[198] = 66;
  (*quadsListArrayPtr)[199] = 65;
  (*quadsListArrayPtr)[200] = 54;
  (*quadsListArrayPtr)[201] = 55;
  (*quadsListArrayPtr)[202] = 67;
  (*quadsListArrayPtr)[203] = 66;
  (*quadsListArrayPtr)[204] = 55;
  (*quadsListArrayPtr)[205] = 56;
  (*quadsListArrayPtr)[206] = 68;
  (*quadsListArrayPtr)[207] = 67;
  (*quadsListArrayPtr)[208] = 56;
  (*quadsListArrayPtr)[209] = 57;
  (*quadsListArrayPtr)[210] = 69;
  (*quadsListArrayPtr)[211] = 68;
  (*quadsListArrayPtr)[212] = 57;
  (*quadsListArrayPtr)[213] = 58;
  (*quadsListArrayPtr)[214] = 70;
  (*quadsListArrayPtr)[215] = 69;
  (*quadsListArrayPtr)[216] = 58;
  (*quadsListArrayPtr)[217] = 59;
  (*quadsListArrayPtr)[218] = 71;
  (*quadsListArrayPtr)[219] = 70;
  (*quadsListArrayPtr)[220] = 60;
  (*quadsListArrayPtr)[221] = 61;
  (*quadsListArrayPtr)[222] = 73;
  (*quadsListArrayPtr)[223] = 72;
  (*quadsListArrayPtr)[224] = 61;
  (*quadsListArrayPtr)[225] = 62;
  (*quadsListArrayPtr)[226] = 74;
  (*quadsListArrayPtr)[227] = 73;
  (*quadsListArrayPtr)[228] = 62;
  (*quadsListArrayPtr)[229] = 63;
  (*quadsListArrayPtr)[230] = 75;
  (*quadsListArrayPtr)[231] = 74;
  (*quadsListArrayPtr)[232] = 63;
  (*quadsListArrayPtr)[233] = 64;
  (*quadsListArrayPtr)[234] = 76;
  (*quadsListArrayPtr)[235] = 75;
  (*quadsListArrayPtr)[236] = 64;
  (*quadsListArrayPtr)[237] = 65;
  (*quadsListArrayPtr)[238] = 77;
  (*quadsListArrayPtr)[239] = 76;
  (*quadsListArrayPtr)[240] = 65;
  (*quadsListArrayPtr)[241] = 66;
  (*quadsListArrayPtr)[242] = 78;
  (*quadsListArrayPtr)[243] = 77;
  (*quadsListArrayPtr)[244] = 66;
  (*quadsListArrayPtr)[245] = 67;
  (*quadsListArrayPtr)[246] = 79;
  (*quadsListArrayPtr)[247] = 78;
  (*quadsListArrayPtr)[248] = 67;
  (*quadsListArrayPtr)[249] = 68;
  (*quadsListArrayPtr)[250] = 80;
  (*quadsListArrayPtr)[251] = 79;
  (*quadsListArrayPtr)[252] = 68;
  (*quadsListArrayPtr)[253] = 69;
  (*quadsListArrayPtr)[254] = 81;
  (*quadsListArrayPtr)[255] = 80;
  (*quadsListArrayPtr)[256] = 69;
  (*quadsListArrayPtr)[257] = 70;
  (*quadsListArrayPtr)[258] = 82;
  (*quadsListArrayPtr)[259] = 81;
  (*quadsListArrayPtr)[260] = 70;
  (*quadsListArrayPtr)[261] = 71;
  (*quadsListArrayPtr)[262] = 83;
  (*quadsListArrayPtr)[263] = 82;
  (*quadsListArrayPtr)[264] = 72;
  (*quadsListArrayPtr)[265] = 73;
  (*quadsListArrayPtr)[266] = 85;
  (*quadsListArrayPtr)[267] = 84;
  (*quadsListArrayPtr)[268] = 73;
  (*quadsListArrayPtr)[269] = 74;
  (*quadsListArrayPtr)[270] = 86;
  (*quadsListArrayPtr)[271] = 85;
  (*quadsListArrayPtr)[272] = 74;
  (*quadsListArrayPtr)[273] = 75;
  (*quadsListArrayPtr)[274] = 87;
  (*quadsListArrayPtr)[275] = 86;
  (*quadsListArrayPtr)[276] = 75;
  (*quadsListArrayPtr)[277] = 76;
  (*quadsListArrayPtr)[278] = 88;
  (*quadsListArrayPtr)[279] = 87;
  (*quadsListArrayPtr)[280] = 76;
  (*quadsListArrayPtr)[281] = 77;
  (*quadsListArrayPtr)[282] = 89;
  (*quadsListArrayPtr)[283] = 88;
  (*quadsListArrayPtr)[284] = 77;
  (*quadsListArrayPtr)[285] = 78;
  (*quadsListArrayPtr)[286] = 90;
  (*quadsListArrayPtr)[287] = 89;
  (*quadsListArrayPtr)[288] = 78;
  (*quadsListArrayPtr)[289] = 79;
  (*quadsListArrayPtr)[290] = 91;
  (*quadsListArrayPtr)[291] = 90;
  (*quadsListArrayPtr)[292] = 79;
  (*quadsListArrayPtr)[293] = 80;
  (*quadsListArrayPtr)[294] = 92;
  (*quadsListArrayPtr)[295] = 91;
  (*quadsListArrayPtr)[296] = 80;
  (*quadsListArrayPtr)[297] = 81;
  (*quadsListArrayPtr)[298] = 93;
  (*quadsListArrayPtr)[299] = 92;
  (*quadsListArrayPtr)[300] = 81;
  (*quadsListArrayPtr)[301] = 82;
  (*quadsListArrayPtr)[302] = 94;
  (*quadsListArrayPtr)[303] = 93;
  (*quadsListArrayPtr)[304] = 82;
  (*quadsListArrayPtr)[305] = 83;
  (*quadsListArrayPtr)[306] = 95;
  (*quadsListArrayPtr)[307] = 94;
  (*quadsListArrayPtr)[308] = 84;
  (*quadsListArrayPtr)[309] = 85;
  (*quadsListArrayPtr)[310] = 97;
  (*quadsListArrayPtr)[311] = 96;
  (*quadsListArrayPtr)[312] = 85;
  (*quadsListArrayPtr)[313] = 86;
  (*quadsListArrayPtr)[314] = 98;
  (*quadsListArrayPtr)[315] = 97;
  (*quadsListArrayPtr)[316] = 86;
  (*quadsListArrayPtr)[317] = 87;
  (*quadsListArrayPtr)[318] = 99;
  (*quadsListArrayPtr)[319] = 98;
  (*quadsListArrayPtr)[320] = 87;
  (*quadsListArrayPtr)[321] = 88;
  (*quadsListArrayPtr)[322] = 100;
  (*quadsListArrayPtr)[323] = 99;
  (*quadsListArrayPtr)[324] = 88;
  (*quadsListArrayPtr)[325] = 89;
  (*quadsListArrayPtr)[326] = 101;
  (*quadsListArrayPtr)[327] = 100;
  (*quadsListArrayPtr)[328] = 89;
  (*quadsListArrayPtr)[329] = 90;
  (*quadsListArrayPtr)[330] = 102;
  (*quadsListArrayPtr)[331] = 101;
  (*quadsListArrayPtr)[332] = 90;
  (*quadsListArrayPtr)[333] = 91;
  (*quadsListArrayPtr)[334] = 103;
  (*quadsListArrayPtr)[335] = 102;
  (*quadsListArrayPtr)[336] = 91;
  (*quadsListArrayPtr)[337] = 92;
  (*quadsListArrayPtr)[338] = 104;
  (*quadsListArrayPtr)[339] = 103;
  (*quadsListArrayPtr)[340] = 92;
  (*quadsListArrayPtr)[341] = 93;
  (*quadsListArrayPtr)[342] = 105;
  (*quadsListArrayPtr)[343] = 104;
  (*quadsListArrayPtr)[344] = 93;
  (*quadsListArrayPtr)[345] = 94;
  (*quadsListArrayPtr)[346] = 106;
  (*quadsListArrayPtr)[347] = 105;
  (*quadsListArrayPtr)[348] = 94;
  (*quadsListArrayPtr)[349] = 95;
  (*quadsListArrayPtr)[350] = 107;
  (*quadsListArrayPtr)[351] = 106;
  (*quadsListArrayPtr)[352] = 96;
  (*quadsListArrayPtr)[353] = 97;
  (*quadsListArrayPtr)[354] = 109;
  (*quadsListArrayPtr)[355] = 108;
  (*quadsListArrayPtr)[356] = 97;
  (*quadsListArrayPtr)[357] = 98;
  (*quadsListArrayPtr)[358] = 110;
  (*quadsListArrayPtr)[359] = 109;
  (*quadsListArrayPtr)[360] = 98;
  (*quadsListArrayPtr)[361] = 99;
  (*quadsListArrayPtr)[362] = 111;
  (*quadsListArrayPtr)[363] = 110;
  (*quadsListArrayPtr)[364] = 99;
  (*quadsListArrayPtr)[365] = 100;
  (*quadsListArrayPtr)[366] = 112;
  (*quadsListArrayPtr)[367] = 111;
  (*quadsListArrayPtr)[368] = 100;
  (*quadsListArrayPtr)[369] = 101;
  (*quadsListArrayPtr)[370] = 113;
  (*quadsListArrayPtr)[371] = 112;
  (*quadsListArrayPtr)[372] = 101;
  (*quadsListArrayPtr)[373] = 102;
  (*quadsListArrayPtr)[374] = 114;
  (*quadsListArrayPtr)[375] = 113;
  (*quadsListArrayPtr)[376] = 102;
  (*quadsListArrayPtr)[377] = 103;
  (*quadsListArrayPtr)[378] = 115;
  (*quadsListArrayPtr)[379] = 114;
  (*quadsListArrayPtr)[380] = 103;
  (*quadsListArrayPtr)[381] = 104;
  (*quadsListArrayPtr)[382] = 116;
  (*quadsListArrayPtr)[383] = 115;
  (*quadsListArrayPtr)[384] = 104;
  (*quadsListArrayPtr)[385] = 105;
  (*quadsListArrayPtr)[386] = 117;
  (*quadsListArrayPtr)[387] = 116;
  (*quadsListArrayPtr)[388] = 105;
  (*quadsListArrayPtr)[389] = 106;
  (*quadsListArrayPtr)[390] = 118;
  (*quadsListArrayPtr)[391] = 117;
  (*quadsListArrayPtr)[392] = 106;
  (*quadsListArrayPtr)[393] = 107;
  (*quadsListArrayPtr)[394] = 119;
  (*quadsListArrayPtr)[395] = 118;
  (*quadsListArrayPtr)[396] = 108;
  (*quadsListArrayPtr)[397] = 109;
  (*quadsListArrayPtr)[398] = 121;
  (*quadsListArrayPtr)[399] = 120;
  (*quadsListArrayPtr)[400] = 109;
  (*quadsListArrayPtr)[401] = 110;
  (*quadsListArrayPtr)[402] = 122;
  (*quadsListArrayPtr)[403] = 121;
  (*quadsListArrayPtr)[404] = 110;
  (*quadsListArrayPtr)[405] = 111;
  (*quadsListArrayPtr)[406] = 123;
  (*quadsListArrayPtr)[407] = 122;
  (*quadsListArrayPtr)[408] = 111;
  (*quadsListArrayPtr)[409] = 112;
  (*quadsListArrayPtr)[410] = 124;
  (*quadsListArrayPtr)[411] = 123;
  (*quadsListArrayPtr)[412] = 112;
  (*quadsListArrayPtr)[413] = 113;
  (*quadsListArrayPtr)[414] = 125;
  (*quadsListArrayPtr)[415] = 124;
  (*quadsListArrayPtr)[416] = 113;
  (*quadsListArrayPtr)[417] = 114;
  (*quadsListArrayPtr)[418] = 126;
  (*quadsListArrayPtr)[419] = 125;
  (*quadsListArrayPtr)[420] = 114;
  (*quadsListArrayPtr)[421] = 115;
  (*quadsListArrayPtr)[422] = 127;
  (*quadsListArrayPtr)[423] = 126;
  (*quadsListArrayPtr)[424] = 115;
  (*quadsListArrayPtr)[425] = 116;
  (*quadsListArrayPtr)[426] = 128;
  (*quadsListArrayPtr)[427] = 127;
  (*quadsListArrayPtr)[428] = 116;
  (*quadsListArrayPtr)[429] = 117;
  (*quadsListArrayPtr)[430] = 129;
  (*quadsListArrayPtr)[431] = 128;
  (*quadsListArrayPtr)[432] = 117;
  (*quadsListArrayPtr)[433] = 118;
  (*quadsListArrayPtr)[434] = 130;
  (*quadsListArrayPtr)[435] = 129;
  (*quadsListArrayPtr)[436] = 118;
  (*quadsListArrayPtr)[437] = 119;
  (*quadsListArrayPtr)[438] = 131;
  (*quadsListArrayPtr)[439] = 130;
  (*quadsListArrayPtr)[440] = 120;
  (*quadsListArrayPtr)[441] = 121;
  (*quadsListArrayPtr)[442] = 133;
  (*quadsListArrayPtr)[443] = 132;
  (*quadsListArrayPtr)[444] = 121;
  (*quadsListArrayPtr)[445] = 122;
  (*quadsListArrayPtr)[446] = 134;
  (*quadsListArrayPtr)[447] = 133;
  (*quadsListArrayPtr)[448] = 122;
  (*quadsListArrayPtr)[449] = 123;
  (*quadsListArrayPtr)[450] = 135;
  (*quadsListArrayPtr)[451] = 134;
  (*quadsListArrayPtr)[452] = 123;
  (*quadsListArrayPtr)[453] = 124;
  (*quadsListArrayPtr)[454] = 136;
  (*quadsListArrayPtr)[455] = 135;
  (*quadsListArrayPtr)[456] = 124;
  (*quadsListArrayPtr)[457] = 125;
  (*quadsListArrayPtr)[458] = 137;
  (*quadsListArrayPtr)[459] = 136;
  (*quadsListArrayPtr)[460] = 125;
  (*quadsListArrayPtr)[461] = 126;
  (*quadsListArrayPtr)[462] = 138;
  (*quadsListArrayPtr)[463] = 137;
  (*quadsListArrayPtr)[464] = 126;
  (*quadsListArrayPtr)[465] = 127;
  (*quadsListArrayPtr)[466] = 139;
  (*quadsListArrayPtr)[467] = 138;
  (*quadsListArrayPtr)[468] = 127;
  (*quadsListArrayPtr)[469] = 128;
  (*quadsListArrayPtr)[470] = 140;
  (*quadsListArrayPtr)[471] = 139;
  (*quadsListArrayPtr)[472] = 128;
  (*quadsListArrayPtr)[473] = 129;
  (*quadsListArrayPtr)[474] = 141;
  (*quadsListArrayPtr)[475] = 140;
  (*quadsListArrayPtr)[476] = 129;
  (*quadsListArrayPtr)[477] = 130;
  (*quadsListArrayPtr)[478] = 142;
  (*quadsListArrayPtr)[479] = 141;
  (*quadsListArrayPtr)[480] = 130;
  (*quadsListArrayPtr)[481] = 131;
  (*quadsListArrayPtr)[482] = 143;
  (*quadsListArrayPtr)[483] = 142;
}

static void CreateTetAndVerticesArrays(DataStructure& dataStructure, const std::string& verticesName, const std::string& tetsName, DataObject::IdType parentId)
{
  Float32Array* vertListArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, verticesName, {5}, {3}, parentId);
  (*vertListArrayPtr)[0] = -1;
  (*vertListArrayPtr)[1] = 0.5;
  (*vertListArrayPtr)[2] = 0;
  (*vertListArrayPtr)[3] = 0;
  (*vertListArrayPtr)[4] = 0;
  (*vertListArrayPtr)[5] = 0;
  (*vertListArrayPtr)[6] = 0;
  (*vertListArrayPtr)[7] = 1;
  (*vertListArrayPtr)[8] = 0;
  (*vertListArrayPtr)[9] = -0.5;
  (*vertListArrayPtr)[10] = 0.5;
  (*vertListArrayPtr)[11] = 1;
  (*vertListArrayPtr)[12] = 1;
  (*vertListArrayPtr)[13] = 0.5;
  (*vertListArrayPtr)[14] = 0;
  IGeometry::MeshIndexArrayType* polyListArrayPtr = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, tetsName, {2}, {4}, parentId);
  (*polyListArrayPtr)[0] = 0;
  (*polyListArrayPtr)[1] = 1;
  (*polyListArrayPtr)[2] = 2;
  (*polyListArrayPtr)[3] = 3;
  (*polyListArrayPtr)[4] = 1;
  (*polyListArrayPtr)[5] = 4;
  (*polyListArrayPtr)[6] = 2;
  (*polyListArrayPtr)[7] = 3;
}

static void CreateHexAndVerticesArrays(DataStructure& dataStructure, const std::string& verticesName, const std::string& hexsName, DataObject::IdType parentId)
{
  Float32Array* vertListArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, verticesName, {12}, {3}, parentId);
  (*vertListArrayPtr)[0] = -1;
  (*vertListArrayPtr)[1] = 1;
  (*vertListArrayPtr)[2] = 1;
  (*vertListArrayPtr)[3] = -1;
  (*vertListArrayPtr)[4] = -1;
  (*vertListArrayPtr)[5] = 1;
  (*vertListArrayPtr)[6] = 0;
  (*vertListArrayPtr)[7] = -1;
  (*vertListArrayPtr)[8] = 1;
  (*vertListArrayPtr)[9] = 0;
  (*vertListArrayPtr)[10] = 1;
  (*vertListArrayPtr)[11] = 1;
  (*vertListArrayPtr)[12] = 0;
  (*vertListArrayPtr)[13] = 1;
  (*vertListArrayPtr)[14] = -1;
  (*vertListArrayPtr)[15] = -1;
  (*vertListArrayPtr)[16] = 1;
  (*vertListArrayPtr)[17] = -1;
  (*vertListArrayPtr)[18] = -1;
  (*vertListArrayPtr)[19] = -1;
  (*vertListArrayPtr)[20] = -1;
  (*vertListArrayPtr)[21] = 0;
  (*vertListArrayPtr)[22] = -1;
  (*vertListArrayPtr)[23] = -1;
  (*vertListArrayPtr)[24] = 1;
  (*vertListArrayPtr)[25] = -1;
  (*vertListArrayPtr)[26] = -1;
  (*vertListArrayPtr)[27] = 1;
  (*vertListArrayPtr)[28] = 1;
  (*vertListArrayPtr)[29] = -1;
  (*vertListArrayPtr)[30] = 1;
  (*vertListArrayPtr)[31] = 1;
  (*vertListArrayPtr)[32] = 1;
  (*vertListArrayPtr)[33] = 1;
  (*vertListArrayPtr)[34] = -1;
  (*vertListArrayPtr)[35] = 1;
  IGeometry::MeshIndexArrayType* polyListArrayPtr = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, hexsName, {2}, {8}, parentId);
  (*polyListArrayPtr)[0] = 6;
  (*polyListArrayPtr)[1] = 7;
  (*polyListArrayPtr)[2] = 4;
  (*polyListArrayPtr)[3] = 5;
  (*polyListArrayPtr)[4] = 1;
  (*polyListArrayPtr)[5] = 2;
  (*polyListArrayPtr)[6] = 3;
  (*polyListArrayPtr)[7] = 0;
  (*polyListArrayPtr)[8] = 7;
  (*polyListArrayPtr)[9] = 8;
  (*polyListArrayPtr)[10] = 9;
  (*polyListArrayPtr)[11] = 4;
  (*polyListArrayPtr)[12] = 2;
  (*polyListArrayPtr)[13] = 11;
  (*polyListArrayPtr)[14] = 10;
  (*polyListArrayPtr)[15] = 3;
}

static void CreateIncompatibleVerticesArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  Float32Array* verticesArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, name, {2}, {3}, parentId);
  (*verticesArrayPtr)[0] = -0.707107;
  (*verticesArrayPtr)[1] = -0.707107;
  (*verticesArrayPtr)[2] = 0;
  (*verticesArrayPtr)[3] = -0.599278;
  (*verticesArrayPtr)[4] = -0.800541;
  (*verticesArrayPtr)[5] = 0;
}

TEST_CASE("SimplnxCore::CreateGeometry: Valid Execution", "[SimplnxCore][CreateGeometry]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateGeometryFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const std::string geometryName = "[Geometry Test]";
  const DataPath geometryPath({geometryName});

  const std::string vertexAmName = INodeGeometry0D::k_VertexAttributeMatrixName;
  const std::string edgeAmName = INodeGeometry1D::k_EdgeAttributeMatrixName;
  const std::string faceAmName = INodeGeometry2D::k_FaceAttributeMatrixName;
  const std::string cellAmName = IGridGeometry::k_CellAttributeMatrixName;

  const std::string vertexListName = "Shared Vertex List";
  const std::string edgeListName = "Edge List";
  const std::string triangleListName = "Triangle List";
  const std::string quadListName = "Quadrilateral List";
  const std::string tetListName = "Tetrahedral List";
  const std::string tetVertexListName = "Tet Vertices List";
  const std::string hexListName = "Hexahedral List";
  const std::string hexVertexListName = "Hex Vertices List";
  const std::string incompatibleVertexListName = "Incompatible Vertices List";
  const DataPath srcVerticesPath({vertexListName});
  const DataPath srcEdgesPath({edgeListName});
  const DataPath srcTrianglesPath({triangleListName});
  const DataPath srcQuadsPath({quadListName});
  const DataPath srcTetsPath({tetListName});
  const DataPath srcHexsPath({hexListName});
  const DataPath srcTetVerticesPath({tetVertexListName});
  const DataPath srcHexVerticesPath({hexVertexListName});
  const DataPath incompatibleVerticesPath({incompatibleVertexListName});

  const std::string xBoundsName = "xBounds";
  const std::string yBoundsName = "yBounds";
  const std::string zBoundsName = "zBounds";
  const DataPath xBoundsPath({xBoundsName});
  const DataPath yBoundsPath({yBoundsName});
  const DataPath zBoundsPath({zBoundsName});

  CreateVerticesArray(dataStructure, vertexListName, 0);
  CreateEdgesArray(dataStructure, edgeListName, 0);
  CreateTrianglesArray(dataStructure, triangleListName, 0);
  CreateQuadsArray(dataStructure, quadListName, 0);
  CreateTetAndVerticesArrays(dataStructure, tetVertexListName, tetListName, 0);
  CreateHexAndVerticesArrays(dataStructure, hexVertexListName, hexListName, 0);
  CreateIncompatibleVerticesArray(dataStructure, incompatibleVertexListName, 0);

  SECTION("Image Geometry Copy")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_ImageGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    std::vector<uint64> imageDims = {40, 60, 80};
    std::vector<float32> imageSpacing = {0.10F, 2.0F, 33.0F};
    std::vector<float32> imageOrigin = {0.0F, 22.0F, 77.0F};
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>(imageDims));
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(imageOrigin));
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(imageSpacing));
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(cellAmName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* const createdGeomPtr = dataStructure.getDataAs<ImageGeom>(geometryPath);
    REQUIRE(createdGeomPtr != nullptr);
    auto* const cellAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(cellAmName));
    REQUIRE(cellAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getCellData() != nullptr);
  }

  SECTION("RectGrid Geometry Copy")
  {
    Float32Array* xBoundsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, xBoundsName, {14}, {1}, 0);
    (*xBoundsArrayPtr)[0] = 0;
    (*xBoundsArrayPtr)[1] = 1;
    (*xBoundsArrayPtr)[2] = 2;
    (*xBoundsArrayPtr)[3] = 3;
    (*xBoundsArrayPtr)[4] = 4;
    (*xBoundsArrayPtr)[5] = 5;
    (*xBoundsArrayPtr)[6] = 6;
    (*xBoundsArrayPtr)[7] = 7;
    (*xBoundsArrayPtr)[8] = 8;
    (*xBoundsArrayPtr)[9] = 9;
    (*xBoundsArrayPtr)[10] = 10;
    (*xBoundsArrayPtr)[11] = 11;
    (*xBoundsArrayPtr)[12] = 12;
    (*xBoundsArrayPtr)[13] = 14;
    Float32Array* yBoundsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, yBoundsName, {14}, {1}, 0);
    (*yBoundsArrayPtr)[0] = 0;
    (*yBoundsArrayPtr)[1] = 2;
    (*yBoundsArrayPtr)[2] = 4;
    (*yBoundsArrayPtr)[3] = 6;
    (*yBoundsArrayPtr)[4] = 8;
    (*yBoundsArrayPtr)[5] = 10;
    (*yBoundsArrayPtr)[6] = 12;
    (*yBoundsArrayPtr)[7] = 14;
    (*yBoundsArrayPtr)[8] = 16;
    (*yBoundsArrayPtr)[9] = 18;
    (*yBoundsArrayPtr)[10] = 20;
    (*yBoundsArrayPtr)[11] = 22;
    (*yBoundsArrayPtr)[12] = 24;
    (*yBoundsArrayPtr)[13] = 26;
    Float32Array* zBoundsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, zBoundsName, {14}, {1}, 0);
    (*zBoundsArrayPtr)[0] = 0;
    (*zBoundsArrayPtr)[1] = 5;
    (*zBoundsArrayPtr)[2] = 10;
    (*zBoundsArrayPtr)[3] = 15;
    (*zBoundsArrayPtr)[4] = 20;
    (*zBoundsArrayPtr)[5] = 25;
    (*zBoundsArrayPtr)[6] = 30;
    (*zBoundsArrayPtr)[7] = 35;
    (*zBoundsArrayPtr)[8] = 40;
    (*zBoundsArrayPtr)[9] = 45;
    (*zBoundsArrayPtr)[10] = 50;
    (*zBoundsArrayPtr)[11] = 55;
    (*zBoundsArrayPtr)[12] = 60;
    (*zBoundsArrayPtr)[13] = 65;

    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_RectGridGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(xBoundsPath));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(yBoundsPath));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(zBoundsPath));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(cellAmName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* const createdGeomPtr = dataStructure.getDataAs<RectGridGeom>(geometryPath);
    REQUIRE(createdGeomPtr != nullptr);
    const auto dims = createdGeomPtr->getDimensions();
    REQUIRE(dims == SizeVec3{13, 13, 13});
    auto* const cellAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(cellAmName));
    REQUIRE(cellAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getCellData() != nullptr);
    auto* const destXBoundsPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(xBoundsName));
    REQUIRE(destXBoundsPtr != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, xBoundsPath, geometryPath.createChildPath(xBoundsName));
    auto* const destYBoundsPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(yBoundsName));
    REQUIRE(destYBoundsPtr != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, yBoundsPath, geometryPath.createChildPath(yBoundsName));
    auto* const destZBoundsPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(zBoundsName));
    REQUIRE(destZBoundsPtr != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, zBoundsPath, geometryPath.createChildPath(zBoundsName));
  }

  SECTION("Vertex Geometry Copy")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_VertexGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(srcVerticesPath));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(vertexAmName));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* const createdGeomPtr = dataStructure.getDataAs<VertexGeom>(geometryPath);
    REQUIRE(createdGeomPtr != nullptr);
    auto* const vertexAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertexAttributeMatrix() != nullptr);
    auto* const destVerticesPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVerticesPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcVerticesPath, geometryPath.createChildPath(vertexListName));
  }

  SECTION("Edge Geometry Copy")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_EdgeGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(srcVerticesPath));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(vertexAmName));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(srcEdgesPath));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(edgeAmName));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* const createdGeomPtr = dataStructure.getDataAs<EdgeGeom>(geometryPath);
    REQUIRE(createdGeomPtr != nullptr);

    auto* const vertexAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertexAttributeMatrix() != nullptr);
    auto* const destVerticesPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVerticesPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcVerticesPath, geometryPath.createChildPath(vertexListName));

    auto* const edgeAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(edgeAmName));
    REQUIRE(edgeAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getEdgeAttributeMatrix() != nullptr);
    auto* const destEdgesPtr = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(edgeListName));
    REQUIRE(destEdgesPtr != nullptr);
    REQUIRE(createdGeomPtr->getEdges() != nullptr);
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStructure, srcEdgesPath, geometryPath.createChildPath(edgeListName));
  }

  SECTION("Triangle Geometry Copy")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_TriangleGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(srcVerticesPath));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(vertexAmName));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(srcTrianglesPath));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(faceAmName));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* const createdGeomPtr = dataStructure.getDataAs<TriangleGeom>(geometryPath);
    REQUIRE(createdGeomPtr != nullptr);

    auto* const vertexAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertexAttributeMatrix() != nullptr);
    auto* const destVerticesPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVerticesPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcVerticesPath, geometryPath.createChildPath(vertexListName));

    auto* const faceAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(faceAmName));
    REQUIRE(faceAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getFaceAttributeMatrix() != nullptr);
    auto* const destFacesPtr = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(triangleListName));
    REQUIRE(destFacesPtr != nullptr);
    REQUIRE(createdGeomPtr->getFaces() != nullptr);
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStructure, srcTrianglesPath, geometryPath.createChildPath(triangleListName));
  }

  SECTION("Quad Geometry Copy")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_QuadGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(srcVerticesPath));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(vertexAmName));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(srcQuadsPath));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(faceAmName));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* const createdGeomPtr = dataStructure.getDataAs<QuadGeom>(geometryPath);
    REQUIRE(createdGeomPtr != nullptr);

    auto* const vertexAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertexAttributeMatrix() != nullptr);
    auto* const destVerticesPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVerticesPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcVerticesPath, geometryPath.createChildPath(vertexListName));

    auto* const quadAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(faceAmName));
    REQUIRE(quadAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getFaceAttributeMatrix() != nullptr);
    auto* const destFacesPtr = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(quadListName));
    REQUIRE(destFacesPtr != nullptr);
    REQUIRE(createdGeomPtr->getFaces() != nullptr);
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStructure, srcQuadsPath, geometryPath.createChildPath(quadListName));
  }

  SECTION("Tetrahedral Geometry Copy")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_TetGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(srcTetVerticesPath));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(vertexAmName));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(srcTetsPath));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(cellAmName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* const createdGeomPtr = dataStructure.getDataAs<TetrahedralGeom>(geometryPath);
    REQUIRE(createdGeomPtr != nullptr);

    auto* const vertexAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertexAttributeMatrix() != nullptr);
    auto* const destVerticesPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(tetVertexListName));
    REQUIRE(destVerticesPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcTetVerticesPath, geometryPath.createChildPath(tetVertexListName));

    auto* const tetAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(cellAmName));
    REQUIRE(tetAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getPolyhedraAttributeMatrix() != nullptr);
    auto* const destCellsPtr = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(tetListName));
    REQUIRE(destCellsPtr != nullptr);
    REQUIRE(createdGeomPtr->getPolyhedra() != nullptr);
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStructure, srcTetsPath, geometryPath.createChildPath(tetListName));
  }

  SECTION("Hexahedral Geometry Copy")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_HexGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(srcHexVerticesPath));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(vertexAmName));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(srcHexsPath));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(cellAmName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* const createdGeomPtr = dataStructure.getDataAs<HexahedralGeom>(geometryPath);
    REQUIRE(createdGeomPtr != nullptr);

    auto* const vertexAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertexAttributeMatrix() != nullptr);
    auto* const destVerticesPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(hexVertexListName));
    REQUIRE(destVerticesPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcHexVerticesPath, geometryPath.createChildPath(hexVertexListName));

    auto* const hexAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(cellAmName));
    REQUIRE(hexAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getPolyhedraAttributeMatrix() != nullptr);
    auto* const destCellsPtr = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(hexListName));
    REQUIRE(destCellsPtr != nullptr);
    REQUIRE(createdGeomPtr->getPolyhedra() != nullptr);
    UnitTest::CompareArrays<IGeometry::MeshIndexType>(dataStructure, srcHexsPath, geometryPath.createChildPath(hexListName));
  }

  SECTION("Triangle Geometry Move")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_TriangleGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_MoveArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(srcVerticesPath));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(vertexAmName));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(srcTrianglesPath));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(faceAmName));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* const createdGeomPtr = dataStructure.getDataAs<TriangleGeom>(geometryPath);
    REQUIRE(createdGeomPtr != nullptr);

    auto* const vertexAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertexAttributeMatrix() != nullptr);
    auto* const destVerticesPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVerticesPtr != nullptr);
    REQUIRE(createdGeomPtr->getVertices() != nullptr);

    auto* const faceAmPtr = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(faceAmName));
    REQUIRE(faceAmPtr != nullptr);
    REQUIRE(createdGeomPtr->getFaceAttributeMatrix() != nullptr);
    auto* const destFacesPtr = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(triangleListName));
    REQUIRE(destFacesPtr != nullptr);
    REQUIRE(createdGeomPtr->getFaces() != nullptr);

    REQUIRE(dataStructure.getData(srcVerticesPath) == nullptr);
    REQUIRE(dataStructure.getData(srcTrianglesPath) == nullptr);
  }

  SECTION("Tetrahedral Geometry Incompatible Arrays")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_TetGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(true));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(incompatibleVerticesPath));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(vertexAmName));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(srcTetsPath));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(cellAmName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  }

  SECTION("RectGrid Geometry Invalid Grid Bounds Resolution")
  {
    Float32Array* xBoundsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, xBoundsName, {14}, {1}, 0);
    (*xBoundsArrayPtr)[0] = 0;
    (*xBoundsArrayPtr)[1] = 1;
    (*xBoundsArrayPtr)[2] = 2;
    (*xBoundsArrayPtr)[3] = 3;
    (*xBoundsArrayPtr)[4] = 4;
    (*xBoundsArrayPtr)[5] = 5;
    (*xBoundsArrayPtr)[6] = 6;
    (*xBoundsArrayPtr)[7] = 7;
    (*xBoundsArrayPtr)[8] = 8;
    (*xBoundsArrayPtr)[9] = 9;
    (*xBoundsArrayPtr)[10] = 14;
    (*xBoundsArrayPtr)[11] = 13;
    (*xBoundsArrayPtr)[12] = 12;
    (*xBoundsArrayPtr)[13] = 11;
    Float32Array* yBoundsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, yBoundsName, {14}, {1}, 0);
    (*yBoundsArrayPtr)[0] = 0;
    (*yBoundsArrayPtr)[1] = 2;
    (*yBoundsArrayPtr)[2] = 4;
    (*yBoundsArrayPtr)[3] = 6;
    (*yBoundsArrayPtr)[4] = 8;
    (*yBoundsArrayPtr)[5] = 10;
    (*yBoundsArrayPtr)[6] = 12;
    (*yBoundsArrayPtr)[7] = 14;
    (*yBoundsArrayPtr)[8] = 16;
    (*yBoundsArrayPtr)[9] = 18;
    (*yBoundsArrayPtr)[10] = 20;
    (*yBoundsArrayPtr)[11] = 22;
    (*yBoundsArrayPtr)[12] = 24;
    (*yBoundsArrayPtr)[13] = 26;
    Float32Array* zBoundsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, zBoundsName, {14}, {1}, 0);
    (*zBoundsArrayPtr)[0] = 0;
    (*zBoundsArrayPtr)[1] = 5;
    (*zBoundsArrayPtr)[2] = 10;
    (*zBoundsArrayPtr)[3] = 15;
    (*zBoundsArrayPtr)[4] = 20;
    (*zBoundsArrayPtr)[5] = 25;
    (*zBoundsArrayPtr)[6] = 30;
    (*zBoundsArrayPtr)[7] = 35;
    (*zBoundsArrayPtr)[8] = 40;
    (*zBoundsArrayPtr)[9] = 45;
    (*zBoundsArrayPtr)[10] = 50;
    (*zBoundsArrayPtr)[11] = 55;
    (*zBoundsArrayPtr)[12] = 60;
    (*zBoundsArrayPtr)[13] = 65;

    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_RectGridGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(true));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64ParameterType::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>());
    // RectilinearGrid Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(xBoundsPath));
    args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(yBoundsPath));
    args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(zBoundsPath));
    // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Edge Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_EdgeListPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Triangle Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TriangleListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_QuadrilateralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Triangle & Quadrilateral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(""));
    // Tetrahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_TetrahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_HexahedralListPath_Key, std::make_any<DataPath>(DataPath{}));
    // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(cellAmName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateGeometry: RectGrid preflight with EmptyDataStore bounds", "[SimplnxCore][CreateGeometry]")
{
  const auto arrayHandling = GENERATE(CreateGeometryFilter::k_CopyArray, CreateGeometryFilter::k_MoveArray, CreateGeometryFilter::k_ReferenceArray);
  CAPTURE(arrayHandling);
  DataStructure dataStructure;
  const auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
  REQUIRE(inputGroupPtr != nullptr);
  for(const auto* namePtr : {"X", "Y", "Z"})
  {
    auto storeResult = EmptyDataStore<float32>::Create(ShapeType{3}, ShapeType{1}, "");
    SIMPLNX_RESULT_REQUIRE_VALID(storeResult);
    const auto* boundsPtr = Float32Array::Create(dataStructure, namePtr, std::move(storeResult.value()), inputGroupPtr->getId());
    REQUIRE(boundsPtr != nullptr);
    REQUIRE(boundsPtr->getDataStoreRef().getStoreType() == IDataStore::StoreType::Empty);
  }

  const CreateGeometryFilter filter;
  Arguments args;
  const DataPath geometryPath({"RectGrid"});
  args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_RectGridGeometry));
  args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(arrayHandling));
  args.insertOrAssign(CreateGeometryFilter::k_XBoundsPath_Key, std::make_any<DataPath>(DataPath({"Input", "X"})));
  args.insertOrAssign(CreateGeometryFilter::k_YBoundsPath_Key, std::make_any<DataPath>(DataPath({"Input", "Y"})));
  args.insertOrAssign(CreateGeometryFilter::k_ZBoundsPath_Key, std::make_any<DataPath>(DataPath({"Input", "Z"})));
  args.insertOrAssign(CreateGeometryFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("Cell Data"));

  Result<> actionsResult;
  if(arrayHandling == CreateGeometryFilter::k_ReferenceArray)
  {
    // Reference is supported by the action but is not an enabled filter parameter choice.
    const CreateRectGridGeometryAction action(geometryPath, DataPath({"Input", "X"}), DataPath({"Input", "Y"}), DataPath({"Input", "Z"}), "Cell Data", ArrayHandlingType::Reference);
    REQUIRE_NOTHROW(actionsResult = action.apply(dataStructure, IDataAction::Mode::Preflight));
  }
  else
  {
    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE_NOTHROW(actionsResult = preflightResult.outputActions.value().applyAll(dataStructure, IDataAction::Mode::Preflight));
  }
  SIMPLNX_RESULT_REQUIRE_VALID(actionsResult);
  const auto* geometryPtr = dataStructure.getDataAs<RectGridGeom>(geometryPath);
  REQUIRE(geometryPtr != nullptr);
  REQUIRE(geometryPtr->getDimensions() == SizeVec3{2, 2, 2});
  REQUIRE(geometryPtr->getXBounds() != nullptr);
  REQUIRE(geometryPtr->getYBounds() != nullptr);
  REQUIRE(geometryPtr->getZBounds() != nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CreateGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CreateGeometryFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<CreateGeometryFilter>::uuid);

      const Arguments args = pipelineFilterPtr->getArguments();
      // Fixture uses GeometryType=0 (Image); only common params + image-specific CellAttributeMatrixName are populated.
      CHECK(args.value<ChoicesParameter::ValueType>(CreateGeometryFilter::k_GeometryType_Key) == 0);
      CHECK(args.value<bool>(CreateGeometryFilter::k_WarningsAsErrors_Key) == true);
      CHECK(args.value<ChoicesParameter::ValueType>(CreateGeometryFilter::k_ArrayHandling_Key) == 0);
      CHECK(args.value<DataPath>(CreateGeometryFilter::k_GeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}

TEST_CASE("SimplnxCore::CreateGeometryFilter: Numeric domain regression", "[SimplnxCore][CreateGeometryFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1, 2, 3);
  CAPTURE(row);
  DataStructure dataStructure;
  CreateGeometryFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, VectorUInt64ParameterType::ValueType{2, 2, 2});
  args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, DataPath({"Created"}));
  args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, ChoicesParameter::ValueType{0});
  const float32 nan = std::numeric_limits<float32>::quiet_NaN();
  const std::vector<VectorFloat32Parameter::ValueType> spacings = {{0, 1, 1}, {-1, 1, 1}, {nan, 1, 1}, {1, 1, 1}};
  args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, spacings[row]);
  args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, VectorFloat32Parameter::ValueType{0, row == 3 ? std::numeric_limits<float32>::infinity() : 0, 0});
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors()[0].code == (row == 3 ? -86008 : -86007));
  CHECK(result.outputActions.errors()[0].message.find(row == 3 ? "Origin" : "Spacing") != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find(row == 3 ? "Y" : "X") != std::string::npos);
}

TEST_CASE("SimplnxCore::CreateGeometryFilter: Origin X domain regression", "[SimplnxCore][CreateGeometryFilter][NumericDomainRed]")
{
  DataStructure dataStructure;
  CreateGeometryFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, VectorUInt64ParameterType::ValueType{2, 2, 2});
  args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, ChoicesParameter::ValueType{0});
  args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, DataPath({"Created"}));
  args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, VectorFloat32Parameter::ValueType{std::numeric_limits<float32>::infinity(), 0, 0});
  auto pre = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(pre.outputActions);
  CHECK(pre.outputActions.errors()[0].code == -86008);
  CHECK(pre.outputActions.errors()[0].message.find("Origin") != std::string::npos);
  CHECK(pre.outputActions.errors()[0].message.find('X') != std::string::npos);
}

TEST_CASE("SimplnxCore::CreateGeometryFilter: Inactive image vectors", "[SimplnxCore][CreateGeometryFilter]")
{
  DataStructure dataStructure;
  auto* verticesPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {4}, {3});
  REQUIRE(verticesPtr != nullptr);
  verticesPtr->fill(1);
  CreateGeometryFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, VectorUInt64ParameterType::ValueType{2, 2, 2});
  args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, ChoicesParameter::ValueType{2});
  args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, DataPath({"Created"}));
  args.insertOrAssign(CreateGeometryFilter::k_VertexListPath_Key, DataPath({"Vertices"}));
  args.insertOrAssign(CreateGeometryFilter::k_Spacing_Key, VectorFloat32Parameter::ValueType{0, 0, 0});
  args.insertOrAssign(CreateGeometryFilter::k_Origin_Key, VectorFloat32Parameter::ValueType{std::numeric_limits<float32>::quiet_NaN(), 0, 0});
  auto pre = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
}
