#include "SimplnxCore/Filters/CreateGeometryFilter.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

void CreateVerticesArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  Float32Array* verticesArray = UnitTest::CreateTestDataArray<float32>(dataStructure, name, {144}, {3}, parentId);
  (*verticesArray)[0] = -0.707107;
  (*verticesArray)[1] = -0.707107;
  (*verticesArray)[2] = 0;
  (*verticesArray)[3] = -0.599278;
  (*verticesArray)[4] = -0.800541;
  (*verticesArray)[5] = 0;
  (*verticesArray)[6] = -0.479249;
  (*verticesArray)[7] = -0.877679;
  (*verticesArray)[8] = 0;
  (*verticesArray)[9] = -0.349464;
  (*verticesArray)[10] = -0.93695;
  (*verticesArray)[11] = 0;
  (*verticesArray)[12] = -0.212565;
  (*verticesArray)[13] = -0.977147;
  (*verticesArray)[14] = 0;
  (*verticesArray)[15] = -0.0713392;
  (*verticesArray)[16] = -0.997452;
  (*verticesArray)[17] = 0;
  (*verticesArray)[18] = 0.0713392;
  (*verticesArray)[19] = -0.997452;
  (*verticesArray)[20] = 0;
  (*verticesArray)[21] = 0.212565;
  (*verticesArray)[22] = -0.977147;
  (*verticesArray)[23] = 0;
  (*verticesArray)[24] = 0.349464;
  (*verticesArray)[25] = -0.93695;
  (*verticesArray)[26] = 0;
  (*verticesArray)[27] = 0.479249;
  (*verticesArray)[28] = -0.877679;
  (*verticesArray)[29] = 0;
  (*verticesArray)[30] = 0.599277;
  (*verticesArray)[31] = -0.800541;
  (*verticesArray)[32] = 0;
  (*verticesArray)[33] = 0.707107;
  (*verticesArray)[34] = -0.707107;
  (*verticesArray)[35] = 0;
  (*verticesArray)[36] = -0.800541;
  (*verticesArray)[37] = -0.599278;
  (*verticesArray)[38] = 0;
  (*verticesArray)[39] = -0.667352;
  (*verticesArray)[40] = -0.667352;
  (*verticesArray)[41] = 0.330579;
  (*verticesArray)[42] = -0.541329;
  (*verticesArray)[43] = -0.773098;
  (*verticesArray)[44] = 0.330579;
  (*verticesArray)[45] = -0.398858;
  (*verticesArray)[46] = -0.855354;
  (*verticesArray)[47] = 0.330579;
  (*verticesArray)[48] = -0.244268;
  (*verticesArray)[49] = -0.91162;
  (*verticesArray)[50] = 0.330579;
  (*verticesArray)[51] = -0.0822558;
  (*verticesArray)[52] = -0.940187;
  (*verticesArray)[53] = 0.330579;
  (*verticesArray)[54] = 0.0822557;
  (*verticesArray)[55] = -0.940187;
  (*verticesArray)[56] = 0.330579;
  (*verticesArray)[57] = 0.244268;
  (*verticesArray)[58] = -0.91162;
  (*verticesArray)[59] = 0.330579;
  (*verticesArray)[60] = 0.398858;
  (*verticesArray)[61] = -0.855354;
  (*verticesArray)[62] = 0.330579;
  (*verticesArray)[63] = 0.541329;
  (*verticesArray)[64] = -0.773098;
  (*verticesArray)[65] = 0.330579;
  (*verticesArray)[66] = 0.667352;
  (*verticesArray)[67] = -0.667352;
  (*verticesArray)[68] = 0.330579;
  (*verticesArray)[69] = 0.800541;
  (*verticesArray)[70] = -0.599278;
  (*verticesArray)[71] = 0;
  (*verticesArray)[72] = -0.877679;
  (*verticesArray)[73] = -0.479249;
  (*verticesArray)[74] = 0;
  (*verticesArray)[75] = -0.773098;
  (*verticesArray)[76] = -0.541329;
  (*verticesArray)[77] = 0.330579;
  (*verticesArray)[78] = -0.568298;
  (*verticesArray)[79] = -0.568298;
  (*verticesArray)[80] = 0.595041;
  (*verticesArray)[81] = -0.427592;
  (*verticesArray)[82] = -0.680508;
  (*verticesArray)[83] = 0.595041;
  (*verticesArray)[84] = -0.265444;
  (*verticesArray)[85] = -0.758594;
  (*verticesArray)[86] = 0.595041;
  (*verticesArray)[87] = -0.0899854;
  (*verticesArray)[88] = -0.798642;
  (*verticesArray)[89] = 0.595041;
  (*verticesArray)[90] = 0.0899853;
  (*verticesArray)[91] = -0.798642;
  (*verticesArray)[92] = 0.595041;
  (*verticesArray)[93] = 0.265444;
  (*verticesArray)[94] = -0.758594;
  (*verticesArray)[95] = 0.595041;
  (*verticesArray)[96] = 0.427592;
  (*verticesArray)[97] = -0.680508;
  (*verticesArray)[98] = 0.595041;
  (*verticesArray)[99] = 0.568298;
  (*verticesArray)[100] = -0.568298;
  (*verticesArray)[101] = 0.595041;
  (*verticesArray)[102] = 0.773098;
  (*verticesArray)[103] = -0.541329;
  (*verticesArray)[104] = 0.330579;
  (*verticesArray)[105] = 0.877679;
  (*verticesArray)[106] = -0.479249;
  (*verticesArray)[107] = 0;
  (*verticesArray)[108] = -0.93695;
  (*verticesArray)[109] = -0.349464;
  (*verticesArray)[110] = 0;
  (*verticesArray)[111] = -0.855354;
  (*verticesArray)[112] = -0.398858;
  (*verticesArray)[113] = 0.330579;
  (*verticesArray)[114] = -0.680508;
  (*verticesArray)[115] = -0.427592;
  (*verticesArray)[116] = 0.595041;
  (*verticesArray)[117] = -0.430427;
  (*verticesArray)[118] = -0.430427;
  (*verticesArray)[119] = 0.793388;
  (*verticesArray)[120] = -0.276351;
  (*verticesArray)[121] = -0.54237;
  (*verticesArray)[122] = 0.793388;
  (*verticesArray)[123] = -0.0952242;
  (*verticesArray)[124] = -0.601221;
  (*verticesArray)[125] = 0.793388;
  (*verticesArray)[126] = 0.0952241;
  (*verticesArray)[127] = -0.601221;
  (*verticesArray)[128] = 0.793388;
  (*verticesArray)[129] = 0.276351;
  (*verticesArray)[130] = -0.54237;
  (*verticesArray)[131] = 0.793388;
  (*verticesArray)[132] = 0.430427;
  (*verticesArray)[133] = -0.430427;
  (*verticesArray)[134] = 0.793388;
  (*verticesArray)[135] = 0.680508;
  (*verticesArray)[136] = -0.427592;
  (*verticesArray)[137] = 0.595041;
  (*verticesArray)[138] = 0.855354;
  (*verticesArray)[139] = -0.398858;
  (*verticesArray)[140] = 0.330579;
  (*verticesArray)[141] = 0.93695;
  (*verticesArray)[142] = -0.349464;
  (*verticesArray)[143] = 0;
  (*verticesArray)[144] = -0.977147;
  (*verticesArray)[145] = -0.212565;
  (*verticesArray)[146] = 0;
  (*verticesArray)[147] = -0.91162;
  (*verticesArray)[148] = -0.244268;
  (*verticesArray)[149] = 0.330579;
  (*verticesArray)[150] = -0.758594;
  (*verticesArray)[151] = -0.265444;
  (*verticesArray)[152] = 0.595041;
  (*verticesArray)[153] = -0.54237;
  (*verticesArray)[154] = -0.276351;
  (*verticesArray)[155] = 0.793388;
  (*verticesArray)[156] = -0.267608;
  (*verticesArray)[157] = -0.267608;
  (*verticesArray)[158] = 0.92562;
  (*verticesArray)[159] = -0.0979513;
  (*verticesArray)[160] = -0.365559;
  (*verticesArray)[161] = 0.92562;
  (*verticesArray)[162] = 0.0979512;
  (*verticesArray)[163] = -0.365559;
  (*verticesArray)[164] = 0.92562;
  (*verticesArray)[165] = 0.267608;
  (*verticesArray)[166] = -0.267608;
  (*verticesArray)[167] = 0.92562;
  (*verticesArray)[168] = 0.54237;
  (*verticesArray)[169] = -0.276351;
  (*verticesArray)[170] = 0.793388;
  (*verticesArray)[171] = 0.758594;
  (*verticesArray)[172] = -0.265444;
  (*verticesArray)[173] = 0.595041;
  (*verticesArray)[174] = 0.91162;
  (*verticesArray)[175] = -0.244268;
  (*verticesArray)[176] = 0.330579;
  (*verticesArray)[177] = 0.977147;
  (*verticesArray)[178] = -0.212565;
  (*verticesArray)[179] = 0;
  (*verticesArray)[180] = -0.997452;
  (*verticesArray)[181] = -0.0713392;
  (*verticesArray)[182] = 0;
  (*verticesArray)[183] = -0.940187;
  (*verticesArray)[184] = -0.0822558;
  (*verticesArray)[185] = 0.330579;
  (*verticesArray)[186] = -0.798642;
  (*verticesArray)[187] = -0.0899854;
  (*verticesArray)[188] = 0.595041;
  (*verticesArray)[189] = -0.601221;
  (*verticesArray)[190] = -0.0952242;
  (*verticesArray)[191] = 0.793388;
  (*verticesArray)[192] = -0.365559;
  (*verticesArray)[193] = -0.0979513;
  (*verticesArray)[194] = 0.92562;
  (*verticesArray)[195] = -0.0907211;
  (*verticesArray)[196] = -0.0907211;
  (*verticesArray)[197] = 0.991736;
  (*verticesArray)[198] = 0.0907211;
  (*verticesArray)[199] = -0.0907212;
  (*verticesArray)[200] = 0.991736;
  (*verticesArray)[201] = 0.365559;
  (*verticesArray)[202] = -0.0979513;
  (*verticesArray)[203] = 0.92562;
  (*verticesArray)[204] = 0.601221;
  (*verticesArray)[205] = -0.0952242;
  (*verticesArray)[206] = 0.793388;
  (*verticesArray)[207] = 0.798642;
  (*verticesArray)[208] = -0.0899854;
  (*verticesArray)[209] = 0.595041;
  (*verticesArray)[210] = 0.940187;
  (*verticesArray)[211] = -0.0822558;
  (*verticesArray)[212] = 0.330579;
  (*verticesArray)[213] = 0.997452;
  (*verticesArray)[214] = -0.0713392;
  (*verticesArray)[215] = 0;
  (*verticesArray)[216] = -0.997452;
  (*verticesArray)[217] = 0.0713392;
  (*verticesArray)[218] = 0;
  (*verticesArray)[219] = -0.940187;
  (*verticesArray)[220] = 0.0822557;
  (*verticesArray)[221] = 0.330579;
  (*verticesArray)[222] = -0.798642;
  (*verticesArray)[223] = 0.0899853;
  (*verticesArray)[224] = 0.595041;
  (*verticesArray)[225] = -0.601221;
  (*verticesArray)[226] = 0.0952241;
  (*verticesArray)[227] = 0.793388;
  (*verticesArray)[228] = -0.365559;
  (*verticesArray)[229] = 0.0979512;
  (*verticesArray)[230] = 0.92562;
  (*verticesArray)[231] = -0.0907212;
  (*verticesArray)[232] = 0.0907211;
  (*verticesArray)[233] = 0.991736;
  (*verticesArray)[234] = 0.090721;
  (*verticesArray)[235] = 0.090721;
  (*verticesArray)[236] = 0.991736;
  (*verticesArray)[237] = 0.365559;
  (*verticesArray)[238] = 0.0979512;
  (*verticesArray)[239] = 0.92562;
  (*verticesArray)[240] = 0.601221;
  (*verticesArray)[241] = 0.0952241;
  (*verticesArray)[242] = 0.793388;
  (*verticesArray)[243] = 0.798642;
  (*verticesArray)[244] = 0.0899853;
  (*verticesArray)[245] = 0.595041;
  (*verticesArray)[246] = 0.940187;
  (*verticesArray)[247] = 0.0822557;
  (*verticesArray)[248] = 0.330579;
  (*verticesArray)[249] = 0.997452;
  (*verticesArray)[250] = 0.0713392;
  (*verticesArray)[251] = 0;
  (*verticesArray)[252] = -0.977147;
  (*verticesArray)[253] = 0.212565;
  (*verticesArray)[254] = 0;
  (*verticesArray)[255] = -0.91162;
  (*verticesArray)[256] = 0.244268;
  (*verticesArray)[257] = 0.330579;
  (*verticesArray)[258] = -0.758594;
  (*verticesArray)[259] = 0.265444;
  (*verticesArray)[260] = 0.595041;
  (*verticesArray)[261] = -0.54237;
  (*verticesArray)[262] = 0.276351;
  (*verticesArray)[263] = 0.793388;
  (*verticesArray)[264] = -0.267608;
  (*verticesArray)[265] = 0.267608;
  (*verticesArray)[266] = 0.92562;
  (*verticesArray)[267] = -0.0979513;
  (*verticesArray)[268] = 0.365559;
  (*verticesArray)[269] = 0.92562;
  (*verticesArray)[270] = 0.0979512;
  (*verticesArray)[271] = 0.365559;
  (*verticesArray)[272] = 0.92562;
  (*verticesArray)[273] = 0.267608;
  (*verticesArray)[274] = 0.267608;
  (*verticesArray)[275] = 0.92562;
  (*verticesArray)[276] = 0.54237;
  (*verticesArray)[277] = 0.276351;
  (*verticesArray)[278] = 0.793388;
  (*verticesArray)[279] = 0.758594;
  (*verticesArray)[280] = 0.265444;
  (*verticesArray)[281] = 0.595041;
  (*verticesArray)[282] = 0.91162;
  (*verticesArray)[283] = 0.244268;
  (*verticesArray)[284] = 0.330579;
  (*verticesArray)[285] = 0.977147;
  (*verticesArray)[286] = 0.212565;
  (*verticesArray)[287] = 0;
  (*verticesArray)[288] = -0.93695;
  (*verticesArray)[289] = 0.349464;
  (*verticesArray)[290] = 0;
  (*verticesArray)[291] = -0.855354;
  (*verticesArray)[292] = 0.398858;
  (*verticesArray)[293] = 0.330579;
  (*verticesArray)[294] = -0.680508;
  (*verticesArray)[295] = 0.427592;
  (*verticesArray)[296] = 0.595041;
  (*verticesArray)[297] = -0.430427;
  (*verticesArray)[298] = 0.430427;
  (*verticesArray)[299] = 0.793388;
  (*verticesArray)[300] = -0.276351;
  (*verticesArray)[301] = 0.54237;
  (*verticesArray)[302] = 0.793388;
  (*verticesArray)[303] = -0.0952242;
  (*verticesArray)[304] = 0.601221;
  (*verticesArray)[305] = 0.793388;
  (*verticesArray)[306] = 0.0952241;
  (*verticesArray)[307] = 0.601221;
  (*verticesArray)[308] = 0.793388;
  (*verticesArray)[309] = 0.276351;
  (*verticesArray)[310] = 0.54237;
  (*verticesArray)[311] = 0.793388;
  (*verticesArray)[312] = 0.430427;
  (*verticesArray)[313] = 0.430427;
  (*verticesArray)[314] = 0.793388;
  (*verticesArray)[315] = 0.680508;
  (*verticesArray)[316] = 0.427592;
  (*verticesArray)[317] = 0.595041;
  (*verticesArray)[318] = 0.855354;
  (*verticesArray)[319] = 0.398858;
  (*verticesArray)[320] = 0.330579;
  (*verticesArray)[321] = 0.93695;
  (*verticesArray)[322] = 0.349464;
  (*verticesArray)[323] = 0;
  (*verticesArray)[324] = -0.877679;
  (*verticesArray)[325] = 0.479249;
  (*verticesArray)[326] = 0;
  (*verticesArray)[327] = -0.773098;
  (*verticesArray)[328] = 0.541329;
  (*verticesArray)[329] = 0.330579;
  (*verticesArray)[330] = -0.568298;
  (*verticesArray)[331] = 0.568298;
  (*verticesArray)[332] = 0.595041;
  (*verticesArray)[333] = -0.427592;
  (*verticesArray)[334] = 0.680508;
  (*verticesArray)[335] = 0.595041;
  (*verticesArray)[336] = -0.265444;
  (*verticesArray)[337] = 0.758594;
  (*verticesArray)[338] = 0.595041;
  (*verticesArray)[339] = -0.0899854;
  (*verticesArray)[340] = 0.798642;
  (*verticesArray)[341] = 0.595041;
  (*verticesArray)[342] = 0.0899853;
  (*verticesArray)[343] = 0.798642;
  (*verticesArray)[344] = 0.595041;
  (*verticesArray)[345] = 0.265444;
  (*verticesArray)[346] = 0.758594;
  (*verticesArray)[347] = 0.595041;
  (*verticesArray)[348] = 0.427592;
  (*verticesArray)[349] = 0.680508;
  (*verticesArray)[350] = 0.595041;
  (*verticesArray)[351] = 0.568298;
  (*verticesArray)[352] = 0.568298;
  (*verticesArray)[353] = 0.595041;
  (*verticesArray)[354] = 0.773098;
  (*verticesArray)[355] = 0.541329;
  (*verticesArray)[356] = 0.330579;
  (*verticesArray)[357] = 0.877679;
  (*verticesArray)[358] = 0.479249;
  (*verticesArray)[359] = 0;
  (*verticesArray)[360] = -0.800541;
  (*verticesArray)[361] = 0.599277;
  (*verticesArray)[362] = 0;
  (*verticesArray)[363] = -0.667352;
  (*verticesArray)[364] = 0.667352;
  (*verticesArray)[365] = 0.330579;
  (*verticesArray)[366] = -0.541329;
  (*verticesArray)[367] = 0.773098;
  (*verticesArray)[368] = 0.330579;
  (*verticesArray)[369] = -0.398858;
  (*verticesArray)[370] = 0.855354;
  (*verticesArray)[371] = 0.330579;
  (*verticesArray)[372] = -0.244268;
  (*verticesArray)[373] = 0.91162;
  (*verticesArray)[374] = 0.330579;
  (*verticesArray)[375] = -0.0822558;
  (*verticesArray)[376] = 0.940187;
  (*verticesArray)[377] = 0.330579;
  (*verticesArray)[378] = 0.0822557;
  (*verticesArray)[379] = 0.940187;
  (*verticesArray)[380] = 0.330579;
  (*verticesArray)[381] = 0.244268;
  (*verticesArray)[382] = 0.91162;
  (*verticesArray)[383] = 0.330579;
  (*verticesArray)[384] = 0.398858;
  (*verticesArray)[385] = 0.855354;
  (*verticesArray)[386] = 0.330579;
  (*verticesArray)[387] = 0.541329;
  (*verticesArray)[388] = 0.773098;
  (*verticesArray)[389] = 0.330579;
  (*verticesArray)[390] = 0.667352;
  (*verticesArray)[391] = 0.667352;
  (*verticesArray)[392] = 0.330579;
  (*verticesArray)[393] = 0.800541;
  (*verticesArray)[394] = 0.599277;
  (*verticesArray)[395] = 0;
  (*verticesArray)[396] = -0.707107;
  (*verticesArray)[397] = 0.707107;
  (*verticesArray)[398] = 0;
  (*verticesArray)[399] = -0.599278;
  (*verticesArray)[400] = 0.800541;
  (*verticesArray)[401] = 0;
  (*verticesArray)[402] = -0.479249;
  (*verticesArray)[403] = 0.877679;
  (*verticesArray)[404] = 0;
  (*verticesArray)[405] = -0.349464;
  (*verticesArray)[406] = 0.93695;
  (*verticesArray)[407] = 0;
  (*verticesArray)[408] = -0.212565;
  (*verticesArray)[409] = 0.977147;
  (*verticesArray)[410] = 0;
  (*verticesArray)[411] = -0.0713392;
  (*verticesArray)[412] = 0.997452;
  (*verticesArray)[413] = 0;
  (*verticesArray)[414] = 0.0713392;
  (*verticesArray)[415] = 0.997452;
  (*verticesArray)[416] = 0;
  (*verticesArray)[417] = 0.212565;
  (*verticesArray)[418] = 0.977147;
  (*verticesArray)[419] = 0;
  (*verticesArray)[420] = 0.349464;
  (*verticesArray)[421] = 0.93695;
  (*verticesArray)[422] = 0;
  (*verticesArray)[423] = 0.479249;
  (*verticesArray)[424] = 0.877679;
  (*verticesArray)[425] = 0;
  (*verticesArray)[426] = 0.599277;
  (*verticesArray)[427] = 0.800541;
  (*verticesArray)[428] = 0;
  (*verticesArray)[429] = 0.707107;
  (*verticesArray)[430] = 0.707107;
  (*verticesArray)[431] = 0;
}

void CreateEdgesArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  IGeometry::MeshIndexArrayType* edgesListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, name, {264}, {2}, parentId);
  (*edgesListArray)[0] = 12;
  (*edgesListArray)[1] = 0;
  (*edgesListArray)[2] = 0;
  (*edgesListArray)[3] = 1;
  (*edgesListArray)[4] = 13;
  (*edgesListArray)[5] = 1;
  (*edgesListArray)[6] = 1;
  (*edgesListArray)[7] = 2;
  (*edgesListArray)[8] = 14;
  (*edgesListArray)[9] = 2;
  (*edgesListArray)[10] = 2;
  (*edgesListArray)[11] = 3;
  (*edgesListArray)[12] = 15;
  (*edgesListArray)[13] = 3;
  (*edgesListArray)[14] = 3;
  (*edgesListArray)[15] = 4;
  (*edgesListArray)[16] = 16;
  (*edgesListArray)[17] = 4;
  (*edgesListArray)[18] = 4;
  (*edgesListArray)[19] = 5;
  (*edgesListArray)[20] = 17;
  (*edgesListArray)[21] = 5;
  (*edgesListArray)[22] = 5;
  (*edgesListArray)[23] = 6;
  (*edgesListArray)[24] = 18;
  (*edgesListArray)[25] = 6;
  (*edgesListArray)[26] = 6;
  (*edgesListArray)[27] = 7;
  (*edgesListArray)[28] = 19;
  (*edgesListArray)[29] = 7;
  (*edgesListArray)[30] = 7;
  (*edgesListArray)[31] = 8;
  (*edgesListArray)[32] = 20;
  (*edgesListArray)[33] = 8;
  (*edgesListArray)[34] = 8;
  (*edgesListArray)[35] = 9;
  (*edgesListArray)[36] = 21;
  (*edgesListArray)[37] = 9;
  (*edgesListArray)[38] = 9;
  (*edgesListArray)[39] = 10;
  (*edgesListArray)[40] = 22;
  (*edgesListArray)[41] = 10;
  (*edgesListArray)[42] = 10;
  (*edgesListArray)[43] = 11;
  (*edgesListArray)[44] = 11;
  (*edgesListArray)[45] = 23;
  (*edgesListArray)[46] = 24;
  (*edgesListArray)[47] = 12;
  (*edgesListArray)[48] = 12;
  (*edgesListArray)[49] = 13;
  (*edgesListArray)[50] = 25;
  (*edgesListArray)[51] = 13;
  (*edgesListArray)[52] = 13;
  (*edgesListArray)[53] = 14;
  (*edgesListArray)[54] = 26;
  (*edgesListArray)[55] = 14;
  (*edgesListArray)[56] = 14;
  (*edgesListArray)[57] = 15;
  (*edgesListArray)[58] = 27;
  (*edgesListArray)[59] = 15;
  (*edgesListArray)[60] = 15;
  (*edgesListArray)[61] = 16;
  (*edgesListArray)[62] = 28;
  (*edgesListArray)[63] = 16;
  (*edgesListArray)[64] = 16;
  (*edgesListArray)[65] = 17;
  (*edgesListArray)[66] = 29;
  (*edgesListArray)[67] = 17;
  (*edgesListArray)[68] = 17;
  (*edgesListArray)[69] = 18;
  (*edgesListArray)[70] = 30;
  (*edgesListArray)[71] = 18;
  (*edgesListArray)[72] = 18;
  (*edgesListArray)[73] = 19;
  (*edgesListArray)[74] = 31;
  (*edgesListArray)[75] = 19;
  (*edgesListArray)[76] = 19;
  (*edgesListArray)[77] = 20;
  (*edgesListArray)[78] = 32;
  (*edgesListArray)[79] = 20;
  (*edgesListArray)[80] = 20;
  (*edgesListArray)[81] = 21;
  (*edgesListArray)[82] = 33;
  (*edgesListArray)[83] = 21;
  (*edgesListArray)[84] = 21;
  (*edgesListArray)[85] = 22;
  (*edgesListArray)[86] = 34;
  (*edgesListArray)[87] = 22;
  (*edgesListArray)[88] = 22;
  (*edgesListArray)[89] = 23;
  (*edgesListArray)[90] = 23;
  (*edgesListArray)[91] = 35;
  (*edgesListArray)[92] = 36;
  (*edgesListArray)[93] = 24;
  (*edgesListArray)[94] = 24;
  (*edgesListArray)[95] = 25;
  (*edgesListArray)[96] = 37;
  (*edgesListArray)[97] = 25;
  (*edgesListArray)[98] = 25;
  (*edgesListArray)[99] = 26;
  (*edgesListArray)[100] = 38;
  (*edgesListArray)[101] = 26;
  (*edgesListArray)[102] = 26;
  (*edgesListArray)[103] = 27;
  (*edgesListArray)[104] = 39;
  (*edgesListArray)[105] = 27;
  (*edgesListArray)[106] = 27;
  (*edgesListArray)[107] = 28;
  (*edgesListArray)[108] = 40;
  (*edgesListArray)[109] = 28;
  (*edgesListArray)[110] = 28;
  (*edgesListArray)[111] = 29;
  (*edgesListArray)[112] = 41;
  (*edgesListArray)[113] = 29;
  (*edgesListArray)[114] = 29;
  (*edgesListArray)[115] = 30;
  (*edgesListArray)[116] = 42;
  (*edgesListArray)[117] = 30;
  (*edgesListArray)[118] = 30;
  (*edgesListArray)[119] = 31;
  (*edgesListArray)[120] = 43;
  (*edgesListArray)[121] = 31;
  (*edgesListArray)[122] = 31;
  (*edgesListArray)[123] = 32;
  (*edgesListArray)[124] = 44;
  (*edgesListArray)[125] = 32;
  (*edgesListArray)[126] = 32;
  (*edgesListArray)[127] = 33;
  (*edgesListArray)[128] = 45;
  (*edgesListArray)[129] = 33;
  (*edgesListArray)[130] = 33;
  (*edgesListArray)[131] = 34;
  (*edgesListArray)[132] = 46;
  (*edgesListArray)[133] = 34;
  (*edgesListArray)[134] = 34;
  (*edgesListArray)[135] = 35;
  (*edgesListArray)[136] = 35;
  (*edgesListArray)[137] = 47;
  (*edgesListArray)[138] = 48;
  (*edgesListArray)[139] = 36;
  (*edgesListArray)[140] = 36;
  (*edgesListArray)[141] = 37;
  (*edgesListArray)[142] = 49;
  (*edgesListArray)[143] = 37;
  (*edgesListArray)[144] = 37;
  (*edgesListArray)[145] = 38;
  (*edgesListArray)[146] = 50;
  (*edgesListArray)[147] = 38;
  (*edgesListArray)[148] = 38;
  (*edgesListArray)[149] = 39;
  (*edgesListArray)[150] = 51;
  (*edgesListArray)[151] = 39;
  (*edgesListArray)[152] = 39;
  (*edgesListArray)[153] = 40;
  (*edgesListArray)[154] = 52;
  (*edgesListArray)[155] = 40;
  (*edgesListArray)[156] = 40;
  (*edgesListArray)[157] = 41;
  (*edgesListArray)[158] = 53;
  (*edgesListArray)[159] = 41;
  (*edgesListArray)[160] = 41;
  (*edgesListArray)[161] = 42;
  (*edgesListArray)[162] = 54;
  (*edgesListArray)[163] = 42;
  (*edgesListArray)[164] = 42;
  (*edgesListArray)[165] = 43;
  (*edgesListArray)[166] = 55;
  (*edgesListArray)[167] = 43;
  (*edgesListArray)[168] = 43;
  (*edgesListArray)[169] = 44;
  (*edgesListArray)[170] = 56;
  (*edgesListArray)[171] = 44;
  (*edgesListArray)[172] = 44;
  (*edgesListArray)[173] = 45;
  (*edgesListArray)[174] = 57;
  (*edgesListArray)[175] = 45;
  (*edgesListArray)[176] = 45;
  (*edgesListArray)[177] = 46;
  (*edgesListArray)[178] = 58;
  (*edgesListArray)[179] = 46;
  (*edgesListArray)[180] = 46;
  (*edgesListArray)[181] = 47;
  (*edgesListArray)[182] = 47;
  (*edgesListArray)[183] = 59;
  (*edgesListArray)[184] = 60;
  (*edgesListArray)[185] = 48;
  (*edgesListArray)[186] = 48;
  (*edgesListArray)[187] = 49;
  (*edgesListArray)[188] = 61;
  (*edgesListArray)[189] = 49;
  (*edgesListArray)[190] = 49;
  (*edgesListArray)[191] = 50;
  (*edgesListArray)[192] = 62;
  (*edgesListArray)[193] = 50;
  (*edgesListArray)[194] = 50;
  (*edgesListArray)[195] = 51;
  (*edgesListArray)[196] = 63;
  (*edgesListArray)[197] = 51;
  (*edgesListArray)[198] = 51;
  (*edgesListArray)[199] = 52;
  (*edgesListArray)[200] = 64;
  (*edgesListArray)[201] = 52;
  (*edgesListArray)[202] = 52;
  (*edgesListArray)[203] = 53;
  (*edgesListArray)[204] = 65;
  (*edgesListArray)[205] = 53;
  (*edgesListArray)[206] = 53;
  (*edgesListArray)[207] = 54;
  (*edgesListArray)[208] = 66;
  (*edgesListArray)[209] = 54;
  (*edgesListArray)[210] = 54;
  (*edgesListArray)[211] = 55;
  (*edgesListArray)[212] = 67;
  (*edgesListArray)[213] = 55;
  (*edgesListArray)[214] = 55;
  (*edgesListArray)[215] = 56;
  (*edgesListArray)[216] = 68;
  (*edgesListArray)[217] = 56;
  (*edgesListArray)[218] = 56;
  (*edgesListArray)[219] = 57;
  (*edgesListArray)[220] = 69;
  (*edgesListArray)[221] = 57;
  (*edgesListArray)[222] = 57;
  (*edgesListArray)[223] = 58;
  (*edgesListArray)[224] = 70;
  (*edgesListArray)[225] = 58;
  (*edgesListArray)[226] = 58;
  (*edgesListArray)[227] = 59;
  (*edgesListArray)[228] = 59;
  (*edgesListArray)[229] = 71;
  (*edgesListArray)[230] = 72;
  (*edgesListArray)[231] = 60;
  (*edgesListArray)[232] = 60;
  (*edgesListArray)[233] = 61;
  (*edgesListArray)[234] = 73;
  (*edgesListArray)[235] = 61;
  (*edgesListArray)[236] = 61;
  (*edgesListArray)[237] = 62;
  (*edgesListArray)[238] = 74;
  (*edgesListArray)[239] = 62;
  (*edgesListArray)[240] = 62;
  (*edgesListArray)[241] = 63;
  (*edgesListArray)[242] = 75;
  (*edgesListArray)[243] = 63;
  (*edgesListArray)[244] = 63;
  (*edgesListArray)[245] = 64;
  (*edgesListArray)[246] = 76;
  (*edgesListArray)[247] = 64;
  (*edgesListArray)[248] = 64;
  (*edgesListArray)[249] = 65;
  (*edgesListArray)[250] = 77;
  (*edgesListArray)[251] = 65;
  (*edgesListArray)[252] = 65;
  (*edgesListArray)[253] = 66;
  (*edgesListArray)[254] = 78;
  (*edgesListArray)[255] = 66;
  (*edgesListArray)[256] = 66;
  (*edgesListArray)[257] = 67;
  (*edgesListArray)[258] = 79;
  (*edgesListArray)[259] = 67;
  (*edgesListArray)[260] = 67;
  (*edgesListArray)[261] = 68;
  (*edgesListArray)[262] = 80;
  (*edgesListArray)[263] = 68;
  (*edgesListArray)[264] = 68;
  (*edgesListArray)[265] = 69;
  (*edgesListArray)[266] = 81;
  (*edgesListArray)[267] = 69;
  (*edgesListArray)[268] = 69;
  (*edgesListArray)[269] = 70;
  (*edgesListArray)[270] = 82;
  (*edgesListArray)[271] = 70;
  (*edgesListArray)[272] = 70;
  (*edgesListArray)[273] = 71;
  (*edgesListArray)[274] = 71;
  (*edgesListArray)[275] = 83;
  (*edgesListArray)[276] = 84;
  (*edgesListArray)[277] = 72;
  (*edgesListArray)[278] = 72;
  (*edgesListArray)[279] = 73;
  (*edgesListArray)[280] = 85;
  (*edgesListArray)[281] = 73;
  (*edgesListArray)[282] = 73;
  (*edgesListArray)[283] = 74;
  (*edgesListArray)[284] = 86;
  (*edgesListArray)[285] = 74;
  (*edgesListArray)[286] = 74;
  (*edgesListArray)[287] = 75;
  (*edgesListArray)[288] = 87;
  (*edgesListArray)[289] = 75;
  (*edgesListArray)[290] = 75;
  (*edgesListArray)[291] = 76;
  (*edgesListArray)[292] = 88;
  (*edgesListArray)[293] = 76;
  (*edgesListArray)[294] = 76;
  (*edgesListArray)[295] = 77;
  (*edgesListArray)[296] = 89;
  (*edgesListArray)[297] = 77;
  (*edgesListArray)[298] = 77;
  (*edgesListArray)[299] = 78;
  (*edgesListArray)[300] = 90;
  (*edgesListArray)[301] = 78;
  (*edgesListArray)[302] = 78;
  (*edgesListArray)[303] = 79;
  (*edgesListArray)[304] = 91;
  (*edgesListArray)[305] = 79;
  (*edgesListArray)[306] = 79;
  (*edgesListArray)[307] = 80;
  (*edgesListArray)[308] = 92;
  (*edgesListArray)[309] = 80;
  (*edgesListArray)[310] = 80;
  (*edgesListArray)[311] = 81;
  (*edgesListArray)[312] = 93;
  (*edgesListArray)[313] = 81;
  (*edgesListArray)[314] = 81;
  (*edgesListArray)[315] = 82;
  (*edgesListArray)[316] = 94;
  (*edgesListArray)[317] = 82;
  (*edgesListArray)[318] = 82;
  (*edgesListArray)[319] = 83;
  (*edgesListArray)[320] = 83;
  (*edgesListArray)[321] = 95;
  (*edgesListArray)[322] = 96;
  (*edgesListArray)[323] = 84;
  (*edgesListArray)[324] = 84;
  (*edgesListArray)[325] = 85;
  (*edgesListArray)[326] = 97;
  (*edgesListArray)[327] = 85;
  (*edgesListArray)[328] = 85;
  (*edgesListArray)[329] = 86;
  (*edgesListArray)[330] = 98;
  (*edgesListArray)[331] = 86;
  (*edgesListArray)[332] = 86;
  (*edgesListArray)[333] = 87;
  (*edgesListArray)[334] = 99;
  (*edgesListArray)[335] = 87;
  (*edgesListArray)[336] = 87;
  (*edgesListArray)[337] = 88;
  (*edgesListArray)[338] = 100;
  (*edgesListArray)[339] = 88;
  (*edgesListArray)[340] = 88;
  (*edgesListArray)[341] = 89;
  (*edgesListArray)[342] = 101;
  (*edgesListArray)[343] = 89;
  (*edgesListArray)[344] = 89;
  (*edgesListArray)[345] = 90;
  (*edgesListArray)[346] = 102;
  (*edgesListArray)[347] = 90;
  (*edgesListArray)[348] = 90;
  (*edgesListArray)[349] = 91;
  (*edgesListArray)[350] = 103;
  (*edgesListArray)[351] = 91;
  (*edgesListArray)[352] = 91;
  (*edgesListArray)[353] = 92;
  (*edgesListArray)[354] = 104;
  (*edgesListArray)[355] = 92;
  (*edgesListArray)[356] = 92;
  (*edgesListArray)[357] = 93;
  (*edgesListArray)[358] = 105;
  (*edgesListArray)[359] = 93;
  (*edgesListArray)[360] = 93;
  (*edgesListArray)[361] = 94;
  (*edgesListArray)[362] = 106;
  (*edgesListArray)[363] = 94;
  (*edgesListArray)[364] = 94;
  (*edgesListArray)[365] = 95;
  (*edgesListArray)[366] = 95;
  (*edgesListArray)[367] = 107;
  (*edgesListArray)[368] = 108;
  (*edgesListArray)[369] = 96;
  (*edgesListArray)[370] = 96;
  (*edgesListArray)[371] = 97;
  (*edgesListArray)[372] = 109;
  (*edgesListArray)[373] = 97;
  (*edgesListArray)[374] = 97;
  (*edgesListArray)[375] = 98;
  (*edgesListArray)[376] = 110;
  (*edgesListArray)[377] = 98;
  (*edgesListArray)[378] = 98;
  (*edgesListArray)[379] = 99;
  (*edgesListArray)[380] = 111;
  (*edgesListArray)[381] = 99;
  (*edgesListArray)[382] = 99;
  (*edgesListArray)[383] = 100;
  (*edgesListArray)[384] = 112;
  (*edgesListArray)[385] = 100;
  (*edgesListArray)[386] = 100;
  (*edgesListArray)[387] = 101;
  (*edgesListArray)[388] = 113;
  (*edgesListArray)[389] = 101;
  (*edgesListArray)[390] = 101;
  (*edgesListArray)[391] = 102;
  (*edgesListArray)[392] = 114;
  (*edgesListArray)[393] = 102;
  (*edgesListArray)[394] = 102;
  (*edgesListArray)[395] = 103;
  (*edgesListArray)[396] = 115;
  (*edgesListArray)[397] = 103;
  (*edgesListArray)[398] = 103;
  (*edgesListArray)[399] = 104;
  (*edgesListArray)[400] = 116;
  (*edgesListArray)[401] = 104;
  (*edgesListArray)[402] = 104;
  (*edgesListArray)[403] = 105;
  (*edgesListArray)[404] = 117;
  (*edgesListArray)[405] = 105;
  (*edgesListArray)[406] = 105;
  (*edgesListArray)[407] = 106;
  (*edgesListArray)[408] = 118;
  (*edgesListArray)[409] = 106;
  (*edgesListArray)[410] = 106;
  (*edgesListArray)[411] = 107;
  (*edgesListArray)[412] = 107;
  (*edgesListArray)[413] = 119;
  (*edgesListArray)[414] = 120;
  (*edgesListArray)[415] = 108;
  (*edgesListArray)[416] = 108;
  (*edgesListArray)[417] = 109;
  (*edgesListArray)[418] = 121;
  (*edgesListArray)[419] = 109;
  (*edgesListArray)[420] = 109;
  (*edgesListArray)[421] = 110;
  (*edgesListArray)[422] = 122;
  (*edgesListArray)[423] = 110;
  (*edgesListArray)[424] = 110;
  (*edgesListArray)[425] = 111;
  (*edgesListArray)[426] = 123;
  (*edgesListArray)[427] = 111;
  (*edgesListArray)[428] = 111;
  (*edgesListArray)[429] = 112;
  (*edgesListArray)[430] = 124;
  (*edgesListArray)[431] = 112;
  (*edgesListArray)[432] = 112;
  (*edgesListArray)[433] = 113;
  (*edgesListArray)[434] = 125;
  (*edgesListArray)[435] = 113;
  (*edgesListArray)[436] = 113;
  (*edgesListArray)[437] = 114;
  (*edgesListArray)[438] = 126;
  (*edgesListArray)[439] = 114;
  (*edgesListArray)[440] = 114;
  (*edgesListArray)[441] = 115;
  (*edgesListArray)[442] = 127;
  (*edgesListArray)[443] = 115;
  (*edgesListArray)[444] = 115;
  (*edgesListArray)[445] = 116;
  (*edgesListArray)[446] = 128;
  (*edgesListArray)[447] = 116;
  (*edgesListArray)[448] = 116;
  (*edgesListArray)[449] = 117;
  (*edgesListArray)[450] = 129;
  (*edgesListArray)[451] = 117;
  (*edgesListArray)[452] = 117;
  (*edgesListArray)[453] = 118;
  (*edgesListArray)[454] = 130;
  (*edgesListArray)[455] = 118;
  (*edgesListArray)[456] = 118;
  (*edgesListArray)[457] = 119;
  (*edgesListArray)[458] = 119;
  (*edgesListArray)[459] = 131;
  (*edgesListArray)[460] = 132;
  (*edgesListArray)[461] = 120;
  (*edgesListArray)[462] = 120;
  (*edgesListArray)[463] = 121;
  (*edgesListArray)[464] = 133;
  (*edgesListArray)[465] = 132;
  (*edgesListArray)[466] = 133;
  (*edgesListArray)[467] = 121;
  (*edgesListArray)[468] = 121;
  (*edgesListArray)[469] = 122;
  (*edgesListArray)[470] = 134;
  (*edgesListArray)[471] = 133;
  (*edgesListArray)[472] = 134;
  (*edgesListArray)[473] = 122;
  (*edgesListArray)[474] = 122;
  (*edgesListArray)[475] = 123;
  (*edgesListArray)[476] = 135;
  (*edgesListArray)[477] = 134;
  (*edgesListArray)[478] = 135;
  (*edgesListArray)[479] = 123;
  (*edgesListArray)[480] = 123;
  (*edgesListArray)[481] = 124;
  (*edgesListArray)[482] = 136;
  (*edgesListArray)[483] = 135;
  (*edgesListArray)[484] = 136;
  (*edgesListArray)[485] = 124;
  (*edgesListArray)[486] = 124;
  (*edgesListArray)[487] = 125;
  (*edgesListArray)[488] = 137;
  (*edgesListArray)[489] = 136;
  (*edgesListArray)[490] = 137;
  (*edgesListArray)[491] = 125;
  (*edgesListArray)[492] = 125;
  (*edgesListArray)[493] = 126;
  (*edgesListArray)[494] = 138;
  (*edgesListArray)[495] = 137;
  (*edgesListArray)[496] = 138;
  (*edgesListArray)[497] = 126;
  (*edgesListArray)[498] = 126;
  (*edgesListArray)[499] = 127;
  (*edgesListArray)[500] = 139;
  (*edgesListArray)[501] = 138;
  (*edgesListArray)[502] = 139;
  (*edgesListArray)[503] = 127;
  (*edgesListArray)[504] = 127;
  (*edgesListArray)[505] = 128;
  (*edgesListArray)[506] = 140;
  (*edgesListArray)[507] = 139;
  (*edgesListArray)[508] = 140;
  (*edgesListArray)[509] = 128;
  (*edgesListArray)[510] = 128;
  (*edgesListArray)[511] = 129;
  (*edgesListArray)[512] = 141;
  (*edgesListArray)[513] = 140;
  (*edgesListArray)[514] = 141;
  (*edgesListArray)[515] = 129;
  (*edgesListArray)[516] = 129;
  (*edgesListArray)[517] = 130;
  (*edgesListArray)[518] = 142;
  (*edgesListArray)[519] = 141;
  (*edgesListArray)[520] = 142;
  (*edgesListArray)[521] = 130;
  (*edgesListArray)[522] = 130;
  (*edgesListArray)[523] = 131;
  (*edgesListArray)[524] = 131;
  (*edgesListArray)[525] = 143;
  (*edgesListArray)[526] = 143;
  (*edgesListArray)[527] = 142;
}

void CreateTrianglesArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  IGeometry::MeshIndexArrayType* trianglesListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, name, {242}, {3}, parentId);
  (*trianglesListArray)[0] = 0;
  (*trianglesListArray)[1] = 1;
  (*trianglesListArray)[2] = 13;
  (*trianglesListArray)[3] = 0;
  (*trianglesListArray)[4] = 13;
  (*trianglesListArray)[5] = 12;
  (*trianglesListArray)[6] = 1;
  (*trianglesListArray)[7] = 2;
  (*trianglesListArray)[8] = 14;
  (*trianglesListArray)[9] = 1;
  (*trianglesListArray)[10] = 14;
  (*trianglesListArray)[11] = 13;
  (*trianglesListArray)[12] = 2;
  (*trianglesListArray)[13] = 3;
  (*trianglesListArray)[14] = 15;
  (*trianglesListArray)[15] = 2;
  (*trianglesListArray)[16] = 15;
  (*trianglesListArray)[17] = 14;
  (*trianglesListArray)[18] = 3;
  (*trianglesListArray)[19] = 4;
  (*trianglesListArray)[20] = 16;
  (*trianglesListArray)[21] = 3;
  (*trianglesListArray)[22] = 16;
  (*trianglesListArray)[23] = 15;
  (*trianglesListArray)[24] = 4;
  (*trianglesListArray)[25] = 5;
  (*trianglesListArray)[26] = 17;
  (*trianglesListArray)[27] = 4;
  (*trianglesListArray)[28] = 17;
  (*trianglesListArray)[29] = 16;
  (*trianglesListArray)[30] = 5;
  (*trianglesListArray)[31] = 6;
  (*trianglesListArray)[32] = 18;
  (*trianglesListArray)[33] = 5;
  (*trianglesListArray)[34] = 18;
  (*trianglesListArray)[35] = 17;
  (*trianglesListArray)[36] = 6;
  (*trianglesListArray)[37] = 7;
  (*trianglesListArray)[38] = 19;
  (*trianglesListArray)[39] = 6;
  (*trianglesListArray)[40] = 19;
  (*trianglesListArray)[41] = 18;
  (*trianglesListArray)[42] = 7;
  (*trianglesListArray)[43] = 8;
  (*trianglesListArray)[44] = 20;
  (*trianglesListArray)[45] = 7;
  (*trianglesListArray)[46] = 20;
  (*trianglesListArray)[47] = 19;
  (*trianglesListArray)[48] = 8;
  (*trianglesListArray)[49] = 9;
  (*trianglesListArray)[50] = 21;
  (*trianglesListArray)[51] = 8;
  (*trianglesListArray)[52] = 21;
  (*trianglesListArray)[53] = 20;
  (*trianglesListArray)[54] = 9;
  (*trianglesListArray)[55] = 10;
  (*trianglesListArray)[56] = 22;
  (*trianglesListArray)[57] = 9;
  (*trianglesListArray)[58] = 22;
  (*trianglesListArray)[59] = 21;
  (*trianglesListArray)[60] = 10;
  (*trianglesListArray)[61] = 11;
  (*trianglesListArray)[62] = 23;
  (*trianglesListArray)[63] = 10;
  (*trianglesListArray)[64] = 23;
  (*trianglesListArray)[65] = 22;
  (*trianglesListArray)[66] = 12;
  (*trianglesListArray)[67] = 13;
  (*trianglesListArray)[68] = 25;
  (*trianglesListArray)[69] = 12;
  (*trianglesListArray)[70] = 25;
  (*trianglesListArray)[71] = 24;
  (*trianglesListArray)[72] = 13;
  (*trianglesListArray)[73] = 14;
  (*trianglesListArray)[74] = 26;
  (*trianglesListArray)[75] = 13;
  (*trianglesListArray)[76] = 26;
  (*trianglesListArray)[77] = 25;
  (*trianglesListArray)[78] = 14;
  (*trianglesListArray)[79] = 15;
  (*trianglesListArray)[80] = 27;
  (*trianglesListArray)[81] = 14;
  (*trianglesListArray)[82] = 27;
  (*trianglesListArray)[83] = 26;
  (*trianglesListArray)[84] = 15;
  (*trianglesListArray)[85] = 16;
  (*trianglesListArray)[86] = 28;
  (*trianglesListArray)[87] = 15;
  (*trianglesListArray)[88] = 28;
  (*trianglesListArray)[89] = 27;
  (*trianglesListArray)[90] = 16;
  (*trianglesListArray)[91] = 17;
  (*trianglesListArray)[92] = 29;
  (*trianglesListArray)[93] = 16;
  (*trianglesListArray)[94] = 29;
  (*trianglesListArray)[95] = 28;
  (*trianglesListArray)[96] = 17;
  (*trianglesListArray)[97] = 18;
  (*trianglesListArray)[98] = 30;
  (*trianglesListArray)[99] = 17;
  (*trianglesListArray)[100] = 30;
  (*trianglesListArray)[101] = 29;
  (*trianglesListArray)[102] = 18;
  (*trianglesListArray)[103] = 19;
  (*trianglesListArray)[104] = 31;
  (*trianglesListArray)[105] = 18;
  (*trianglesListArray)[106] = 31;
  (*trianglesListArray)[107] = 30;
  (*trianglesListArray)[108] = 19;
  (*trianglesListArray)[109] = 20;
  (*trianglesListArray)[110] = 32;
  (*trianglesListArray)[111] = 19;
  (*trianglesListArray)[112] = 32;
  (*trianglesListArray)[113] = 31;
  (*trianglesListArray)[114] = 20;
  (*trianglesListArray)[115] = 21;
  (*trianglesListArray)[116] = 33;
  (*trianglesListArray)[117] = 20;
  (*trianglesListArray)[118] = 33;
  (*trianglesListArray)[119] = 32;
  (*trianglesListArray)[120] = 21;
  (*trianglesListArray)[121] = 22;
  (*trianglesListArray)[122] = 34;
  (*trianglesListArray)[123] = 21;
  (*trianglesListArray)[124] = 34;
  (*trianglesListArray)[125] = 33;
  (*trianglesListArray)[126] = 22;
  (*trianglesListArray)[127] = 23;
  (*trianglesListArray)[128] = 35;
  (*trianglesListArray)[129] = 22;
  (*trianglesListArray)[130] = 35;
  (*trianglesListArray)[131] = 34;
  (*trianglesListArray)[132] = 24;
  (*trianglesListArray)[133] = 25;
  (*trianglesListArray)[134] = 37;
  (*trianglesListArray)[135] = 24;
  (*trianglesListArray)[136] = 37;
  (*trianglesListArray)[137] = 36;
  (*trianglesListArray)[138] = 25;
  (*trianglesListArray)[139] = 26;
  (*trianglesListArray)[140] = 38;
  (*trianglesListArray)[141] = 25;
  (*trianglesListArray)[142] = 38;
  (*trianglesListArray)[143] = 37;
  (*trianglesListArray)[144] = 26;
  (*trianglesListArray)[145] = 27;
  (*trianglesListArray)[146] = 39;
  (*trianglesListArray)[147] = 26;
  (*trianglesListArray)[148] = 39;
  (*trianglesListArray)[149] = 38;
  (*trianglesListArray)[150] = 27;
  (*trianglesListArray)[151] = 28;
  (*trianglesListArray)[152] = 40;
  (*trianglesListArray)[153] = 27;
  (*trianglesListArray)[154] = 40;
  (*trianglesListArray)[155] = 39;
  (*trianglesListArray)[156] = 28;
  (*trianglesListArray)[157] = 29;
  (*trianglesListArray)[158] = 41;
  (*trianglesListArray)[159] = 28;
  (*trianglesListArray)[160] = 41;
  (*trianglesListArray)[161] = 40;
  (*trianglesListArray)[162] = 29;
  (*trianglesListArray)[163] = 30;
  (*trianglesListArray)[164] = 42;
  (*trianglesListArray)[165] = 29;
  (*trianglesListArray)[166] = 42;
  (*trianglesListArray)[167] = 41;
  (*trianglesListArray)[168] = 30;
  (*trianglesListArray)[169] = 31;
  (*trianglesListArray)[170] = 43;
  (*trianglesListArray)[171] = 30;
  (*trianglesListArray)[172] = 43;
  (*trianglesListArray)[173] = 42;
  (*trianglesListArray)[174] = 31;
  (*trianglesListArray)[175] = 32;
  (*trianglesListArray)[176] = 44;
  (*trianglesListArray)[177] = 31;
  (*trianglesListArray)[178] = 44;
  (*trianglesListArray)[179] = 43;
  (*trianglesListArray)[180] = 32;
  (*trianglesListArray)[181] = 33;
  (*trianglesListArray)[182] = 45;
  (*trianglesListArray)[183] = 32;
  (*trianglesListArray)[184] = 45;
  (*trianglesListArray)[185] = 44;
  (*trianglesListArray)[186] = 33;
  (*trianglesListArray)[187] = 34;
  (*trianglesListArray)[188] = 46;
  (*trianglesListArray)[189] = 33;
  (*trianglesListArray)[190] = 46;
  (*trianglesListArray)[191] = 45;
  (*trianglesListArray)[192] = 34;
  (*trianglesListArray)[193] = 35;
  (*trianglesListArray)[194] = 47;
  (*trianglesListArray)[195] = 34;
  (*trianglesListArray)[196] = 47;
  (*trianglesListArray)[197] = 46;
  (*trianglesListArray)[198] = 36;
  (*trianglesListArray)[199] = 37;
  (*trianglesListArray)[200] = 49;
  (*trianglesListArray)[201] = 36;
  (*trianglesListArray)[202] = 49;
  (*trianglesListArray)[203] = 48;
  (*trianglesListArray)[204] = 37;
  (*trianglesListArray)[205] = 38;
  (*trianglesListArray)[206] = 50;
  (*trianglesListArray)[207] = 37;
  (*trianglesListArray)[208] = 50;
  (*trianglesListArray)[209] = 49;
  (*trianglesListArray)[210] = 38;
  (*trianglesListArray)[211] = 39;
  (*trianglesListArray)[212] = 51;
  (*trianglesListArray)[213] = 38;
  (*trianglesListArray)[214] = 51;
  (*trianglesListArray)[215] = 50;
  (*trianglesListArray)[216] = 39;
  (*trianglesListArray)[217] = 40;
  (*trianglesListArray)[218] = 52;
  (*trianglesListArray)[219] = 39;
  (*trianglesListArray)[220] = 52;
  (*trianglesListArray)[221] = 51;
  (*trianglesListArray)[222] = 40;
  (*trianglesListArray)[223] = 41;
  (*trianglesListArray)[224] = 53;
  (*trianglesListArray)[225] = 40;
  (*trianglesListArray)[226] = 53;
  (*trianglesListArray)[227] = 52;
  (*trianglesListArray)[228] = 41;
  (*trianglesListArray)[229] = 42;
  (*trianglesListArray)[230] = 54;
  (*trianglesListArray)[231] = 41;
  (*trianglesListArray)[232] = 54;
  (*trianglesListArray)[233] = 53;
  (*trianglesListArray)[234] = 42;
  (*trianglesListArray)[235] = 43;
  (*trianglesListArray)[236] = 55;
  (*trianglesListArray)[237] = 42;
  (*trianglesListArray)[238] = 55;
  (*trianglesListArray)[239] = 54;
  (*trianglesListArray)[240] = 43;
  (*trianglesListArray)[241] = 44;
  (*trianglesListArray)[242] = 56;
  (*trianglesListArray)[243] = 43;
  (*trianglesListArray)[244] = 56;
  (*trianglesListArray)[245] = 55;
  (*trianglesListArray)[246] = 44;
  (*trianglesListArray)[247] = 45;
  (*trianglesListArray)[248] = 57;
  (*trianglesListArray)[249] = 44;
  (*trianglesListArray)[250] = 57;
  (*trianglesListArray)[251] = 56;
  (*trianglesListArray)[252] = 45;
  (*trianglesListArray)[253] = 46;
  (*trianglesListArray)[254] = 58;
  (*trianglesListArray)[255] = 45;
  (*trianglesListArray)[256] = 58;
  (*trianglesListArray)[257] = 57;
  (*trianglesListArray)[258] = 46;
  (*trianglesListArray)[259] = 47;
  (*trianglesListArray)[260] = 59;
  (*trianglesListArray)[261] = 46;
  (*trianglesListArray)[262] = 59;
  (*trianglesListArray)[263] = 58;
  (*trianglesListArray)[264] = 48;
  (*trianglesListArray)[265] = 49;
  (*trianglesListArray)[266] = 61;
  (*trianglesListArray)[267] = 48;
  (*trianglesListArray)[268] = 61;
  (*trianglesListArray)[269] = 60;
  (*trianglesListArray)[270] = 49;
  (*trianglesListArray)[271] = 50;
  (*trianglesListArray)[272] = 62;
  (*trianglesListArray)[273] = 49;
  (*trianglesListArray)[274] = 62;
  (*trianglesListArray)[275] = 61;
  (*trianglesListArray)[276] = 50;
  (*trianglesListArray)[277] = 51;
  (*trianglesListArray)[278] = 63;
  (*trianglesListArray)[279] = 50;
  (*trianglesListArray)[280] = 63;
  (*trianglesListArray)[281] = 62;
  (*trianglesListArray)[282] = 51;
  (*trianglesListArray)[283] = 52;
  (*trianglesListArray)[284] = 64;
  (*trianglesListArray)[285] = 51;
  (*trianglesListArray)[286] = 64;
  (*trianglesListArray)[287] = 63;
  (*trianglesListArray)[288] = 52;
  (*trianglesListArray)[289] = 53;
  (*trianglesListArray)[290] = 65;
  (*trianglesListArray)[291] = 52;
  (*trianglesListArray)[292] = 65;
  (*trianglesListArray)[293] = 64;
  (*trianglesListArray)[294] = 53;
  (*trianglesListArray)[295] = 54;
  (*trianglesListArray)[296] = 66;
  (*trianglesListArray)[297] = 53;
  (*trianglesListArray)[298] = 66;
  (*trianglesListArray)[299] = 65;
  (*trianglesListArray)[300] = 54;
  (*trianglesListArray)[301] = 55;
  (*trianglesListArray)[302] = 67;
  (*trianglesListArray)[303] = 54;
  (*trianglesListArray)[304] = 67;
  (*trianglesListArray)[305] = 66;
  (*trianglesListArray)[306] = 55;
  (*trianglesListArray)[307] = 56;
  (*trianglesListArray)[308] = 68;
  (*trianglesListArray)[309] = 55;
  (*trianglesListArray)[310] = 68;
  (*trianglesListArray)[311] = 67;
  (*trianglesListArray)[312] = 56;
  (*trianglesListArray)[313] = 57;
  (*trianglesListArray)[314] = 69;
  (*trianglesListArray)[315] = 56;
  (*trianglesListArray)[316] = 69;
  (*trianglesListArray)[317] = 68;
  (*trianglesListArray)[318] = 57;
  (*trianglesListArray)[319] = 58;
  (*trianglesListArray)[320] = 70;
  (*trianglesListArray)[321] = 57;
  (*trianglesListArray)[322] = 70;
  (*trianglesListArray)[323] = 69;
  (*trianglesListArray)[324] = 58;
  (*trianglesListArray)[325] = 59;
  (*trianglesListArray)[326] = 71;
  (*trianglesListArray)[327] = 58;
  (*trianglesListArray)[328] = 71;
  (*trianglesListArray)[329] = 70;
  (*trianglesListArray)[330] = 60;
  (*trianglesListArray)[331] = 61;
  (*trianglesListArray)[332] = 73;
  (*trianglesListArray)[333] = 60;
  (*trianglesListArray)[334] = 73;
  (*trianglesListArray)[335] = 72;
  (*trianglesListArray)[336] = 61;
  (*trianglesListArray)[337] = 62;
  (*trianglesListArray)[338] = 74;
  (*trianglesListArray)[339] = 61;
  (*trianglesListArray)[340] = 74;
  (*trianglesListArray)[341] = 73;
  (*trianglesListArray)[342] = 62;
  (*trianglesListArray)[343] = 63;
  (*trianglesListArray)[344] = 75;
  (*trianglesListArray)[345] = 62;
  (*trianglesListArray)[346] = 75;
  (*trianglesListArray)[347] = 74;
  (*trianglesListArray)[348] = 63;
  (*trianglesListArray)[349] = 64;
  (*trianglesListArray)[350] = 76;
  (*trianglesListArray)[351] = 63;
  (*trianglesListArray)[352] = 76;
  (*trianglesListArray)[353] = 75;
  (*trianglesListArray)[354] = 64;
  (*trianglesListArray)[355] = 65;
  (*trianglesListArray)[356] = 77;
  (*trianglesListArray)[357] = 64;
  (*trianglesListArray)[358] = 77;
  (*trianglesListArray)[359] = 76;
  (*trianglesListArray)[360] = 65;
  (*trianglesListArray)[361] = 66;
  (*trianglesListArray)[362] = 78;
  (*trianglesListArray)[363] = 65;
  (*trianglesListArray)[364] = 78;
  (*trianglesListArray)[365] = 77;
  (*trianglesListArray)[366] = 66;
  (*trianglesListArray)[367] = 67;
  (*trianglesListArray)[368] = 79;
  (*trianglesListArray)[369] = 66;
  (*trianglesListArray)[370] = 79;
  (*trianglesListArray)[371] = 78;
  (*trianglesListArray)[372] = 67;
  (*trianglesListArray)[373] = 68;
  (*trianglesListArray)[374] = 80;
  (*trianglesListArray)[375] = 67;
  (*trianglesListArray)[376] = 80;
  (*trianglesListArray)[377] = 79;
  (*trianglesListArray)[378] = 68;
  (*trianglesListArray)[379] = 69;
  (*trianglesListArray)[380] = 81;
  (*trianglesListArray)[381] = 68;
  (*trianglesListArray)[382] = 81;
  (*trianglesListArray)[383] = 80;
  (*trianglesListArray)[384] = 69;
  (*trianglesListArray)[385] = 70;
  (*trianglesListArray)[386] = 82;
  (*trianglesListArray)[387] = 69;
  (*trianglesListArray)[388] = 82;
  (*trianglesListArray)[389] = 81;
  (*trianglesListArray)[390] = 70;
  (*trianglesListArray)[391] = 71;
  (*trianglesListArray)[392] = 83;
  (*trianglesListArray)[393] = 70;
  (*trianglesListArray)[394] = 83;
  (*trianglesListArray)[395] = 82;
  (*trianglesListArray)[396] = 72;
  (*trianglesListArray)[397] = 73;
  (*trianglesListArray)[398] = 85;
  (*trianglesListArray)[399] = 72;
  (*trianglesListArray)[400] = 85;
  (*trianglesListArray)[401] = 84;
  (*trianglesListArray)[402] = 73;
  (*trianglesListArray)[403] = 74;
  (*trianglesListArray)[404] = 86;
  (*trianglesListArray)[405] = 73;
  (*trianglesListArray)[406] = 86;
  (*trianglesListArray)[407] = 85;
  (*trianglesListArray)[408] = 74;
  (*trianglesListArray)[409] = 75;
  (*trianglesListArray)[410] = 87;
  (*trianglesListArray)[411] = 74;
  (*trianglesListArray)[412] = 87;
  (*trianglesListArray)[413] = 86;
  (*trianglesListArray)[414] = 75;
  (*trianglesListArray)[415] = 76;
  (*trianglesListArray)[416] = 88;
  (*trianglesListArray)[417] = 75;
  (*trianglesListArray)[418] = 88;
  (*trianglesListArray)[419] = 87;
  (*trianglesListArray)[420] = 76;
  (*trianglesListArray)[421] = 77;
  (*trianglesListArray)[422] = 89;
  (*trianglesListArray)[423] = 76;
  (*trianglesListArray)[424] = 89;
  (*trianglesListArray)[425] = 88;
  (*trianglesListArray)[426] = 77;
  (*trianglesListArray)[427] = 78;
  (*trianglesListArray)[428] = 90;
  (*trianglesListArray)[429] = 77;
  (*trianglesListArray)[430] = 90;
  (*trianglesListArray)[431] = 89;
  (*trianglesListArray)[432] = 78;
  (*trianglesListArray)[433] = 79;
  (*trianglesListArray)[434] = 91;
  (*trianglesListArray)[435] = 78;
  (*trianglesListArray)[436] = 91;
  (*trianglesListArray)[437] = 90;
  (*trianglesListArray)[438] = 79;
  (*trianglesListArray)[439] = 80;
  (*trianglesListArray)[440] = 92;
  (*trianglesListArray)[441] = 79;
  (*trianglesListArray)[442] = 92;
  (*trianglesListArray)[443] = 91;
  (*trianglesListArray)[444] = 80;
  (*trianglesListArray)[445] = 81;
  (*trianglesListArray)[446] = 93;
  (*trianglesListArray)[447] = 80;
  (*trianglesListArray)[448] = 93;
  (*trianglesListArray)[449] = 92;
  (*trianglesListArray)[450] = 81;
  (*trianglesListArray)[451] = 82;
  (*trianglesListArray)[452] = 94;
  (*trianglesListArray)[453] = 81;
  (*trianglesListArray)[454] = 94;
  (*trianglesListArray)[455] = 93;
  (*trianglesListArray)[456] = 82;
  (*trianglesListArray)[457] = 83;
  (*trianglesListArray)[458] = 95;
  (*trianglesListArray)[459] = 82;
  (*trianglesListArray)[460] = 95;
  (*trianglesListArray)[461] = 94;
  (*trianglesListArray)[462] = 84;
  (*trianglesListArray)[463] = 85;
  (*trianglesListArray)[464] = 97;
  (*trianglesListArray)[465] = 84;
  (*trianglesListArray)[466] = 97;
  (*trianglesListArray)[467] = 96;
  (*trianglesListArray)[468] = 85;
  (*trianglesListArray)[469] = 86;
  (*trianglesListArray)[470] = 98;
  (*trianglesListArray)[471] = 85;
  (*trianglesListArray)[472] = 98;
  (*trianglesListArray)[473] = 97;
  (*trianglesListArray)[474] = 86;
  (*trianglesListArray)[475] = 87;
  (*trianglesListArray)[476] = 99;
  (*trianglesListArray)[477] = 86;
  (*trianglesListArray)[478] = 99;
  (*trianglesListArray)[479] = 98;
  (*trianglesListArray)[480] = 87;
  (*trianglesListArray)[481] = 88;
  (*trianglesListArray)[482] = 100;
  (*trianglesListArray)[483] = 87;
  (*trianglesListArray)[484] = 100;
  (*trianglesListArray)[485] = 99;
  (*trianglesListArray)[486] = 88;
  (*trianglesListArray)[487] = 89;
  (*trianglesListArray)[488] = 101;
  (*trianglesListArray)[489] = 88;
  (*trianglesListArray)[490] = 101;
  (*trianglesListArray)[491] = 100;
  (*trianglesListArray)[492] = 89;
  (*trianglesListArray)[493] = 90;
  (*trianglesListArray)[494] = 102;
  (*trianglesListArray)[495] = 89;
  (*trianglesListArray)[496] = 102;
  (*trianglesListArray)[497] = 101;
  (*trianglesListArray)[498] = 90;
  (*trianglesListArray)[499] = 91;
  (*trianglesListArray)[500] = 103;
  (*trianglesListArray)[501] = 90;
  (*trianglesListArray)[502] = 103;
  (*trianglesListArray)[503] = 102;
  (*trianglesListArray)[504] = 91;
  (*trianglesListArray)[505] = 92;
  (*trianglesListArray)[506] = 104;
  (*trianglesListArray)[507] = 91;
  (*trianglesListArray)[508] = 104;
  (*trianglesListArray)[509] = 103;
  (*trianglesListArray)[510] = 92;
  (*trianglesListArray)[511] = 93;
  (*trianglesListArray)[512] = 105;
  (*trianglesListArray)[513] = 92;
  (*trianglesListArray)[514] = 105;
  (*trianglesListArray)[515] = 104;
  (*trianglesListArray)[516] = 93;
  (*trianglesListArray)[517] = 94;
  (*trianglesListArray)[518] = 106;
  (*trianglesListArray)[519] = 93;
  (*trianglesListArray)[520] = 106;
  (*trianglesListArray)[521] = 105;
  (*trianglesListArray)[522] = 94;
  (*trianglesListArray)[523] = 95;
  (*trianglesListArray)[524] = 107;
  (*trianglesListArray)[525] = 94;
  (*trianglesListArray)[526] = 107;
  (*trianglesListArray)[527] = 106;
  (*trianglesListArray)[528] = 96;
  (*trianglesListArray)[529] = 97;
  (*trianglesListArray)[530] = 109;
  (*trianglesListArray)[531] = 96;
  (*trianglesListArray)[532] = 109;
  (*trianglesListArray)[533] = 108;
  (*trianglesListArray)[534] = 97;
  (*trianglesListArray)[535] = 98;
  (*trianglesListArray)[536] = 110;
  (*trianglesListArray)[537] = 97;
  (*trianglesListArray)[538] = 110;
  (*trianglesListArray)[539] = 109;
  (*trianglesListArray)[540] = 98;
  (*trianglesListArray)[541] = 99;
  (*trianglesListArray)[542] = 111;
  (*trianglesListArray)[543] = 98;
  (*trianglesListArray)[544] = 111;
  (*trianglesListArray)[545] = 110;
  (*trianglesListArray)[546] = 99;
  (*trianglesListArray)[547] = 100;
  (*trianglesListArray)[548] = 112;
  (*trianglesListArray)[549] = 99;
  (*trianglesListArray)[550] = 112;
  (*trianglesListArray)[551] = 111;
  (*trianglesListArray)[552] = 100;
  (*trianglesListArray)[553] = 101;
  (*trianglesListArray)[554] = 113;
  (*trianglesListArray)[555] = 100;
  (*trianglesListArray)[556] = 113;
  (*trianglesListArray)[557] = 112;
  (*trianglesListArray)[558] = 101;
  (*trianglesListArray)[559] = 102;
  (*trianglesListArray)[560] = 114;
  (*trianglesListArray)[561] = 101;
  (*trianglesListArray)[562] = 114;
  (*trianglesListArray)[563] = 113;
  (*trianglesListArray)[564] = 102;
  (*trianglesListArray)[565] = 103;
  (*trianglesListArray)[566] = 115;
  (*trianglesListArray)[567] = 102;
  (*trianglesListArray)[568] = 115;
  (*trianglesListArray)[569] = 114;
  (*trianglesListArray)[570] = 103;
  (*trianglesListArray)[571] = 104;
  (*trianglesListArray)[572] = 116;
  (*trianglesListArray)[573] = 103;
  (*trianglesListArray)[574] = 116;
  (*trianglesListArray)[575] = 115;
  (*trianglesListArray)[576] = 104;
  (*trianglesListArray)[577] = 105;
  (*trianglesListArray)[578] = 117;
  (*trianglesListArray)[579] = 104;
  (*trianglesListArray)[580] = 117;
  (*trianglesListArray)[581] = 116;
  (*trianglesListArray)[582] = 105;
  (*trianglesListArray)[583] = 106;
  (*trianglesListArray)[584] = 118;
  (*trianglesListArray)[585] = 105;
  (*trianglesListArray)[586] = 118;
  (*trianglesListArray)[587] = 117;
  (*trianglesListArray)[588] = 106;
  (*trianglesListArray)[589] = 107;
  (*trianglesListArray)[590] = 119;
  (*trianglesListArray)[591] = 106;
  (*trianglesListArray)[592] = 119;
  (*trianglesListArray)[593] = 118;
  (*trianglesListArray)[594] = 108;
  (*trianglesListArray)[595] = 109;
  (*trianglesListArray)[596] = 121;
  (*trianglesListArray)[597] = 108;
  (*trianglesListArray)[598] = 121;
  (*trianglesListArray)[599] = 120;
  (*trianglesListArray)[600] = 109;
  (*trianglesListArray)[601] = 110;
  (*trianglesListArray)[602] = 122;
  (*trianglesListArray)[603] = 109;
  (*trianglesListArray)[604] = 122;
  (*trianglesListArray)[605] = 121;
  (*trianglesListArray)[606] = 110;
  (*trianglesListArray)[607] = 111;
  (*trianglesListArray)[608] = 123;
  (*trianglesListArray)[609] = 110;
  (*trianglesListArray)[610] = 123;
  (*trianglesListArray)[611] = 122;
  (*trianglesListArray)[612] = 111;
  (*trianglesListArray)[613] = 112;
  (*trianglesListArray)[614] = 124;
  (*trianglesListArray)[615] = 111;
  (*trianglesListArray)[616] = 124;
  (*trianglesListArray)[617] = 123;
  (*trianglesListArray)[618] = 112;
  (*trianglesListArray)[619] = 113;
  (*trianglesListArray)[620] = 125;
  (*trianglesListArray)[621] = 112;
  (*trianglesListArray)[622] = 125;
  (*trianglesListArray)[623] = 124;
  (*trianglesListArray)[624] = 113;
  (*trianglesListArray)[625] = 114;
  (*trianglesListArray)[626] = 126;
  (*trianglesListArray)[627] = 113;
  (*trianglesListArray)[628] = 126;
  (*trianglesListArray)[629] = 125;
  (*trianglesListArray)[630] = 114;
  (*trianglesListArray)[631] = 115;
  (*trianglesListArray)[632] = 127;
  (*trianglesListArray)[633] = 114;
  (*trianglesListArray)[634] = 127;
  (*trianglesListArray)[635] = 126;
  (*trianglesListArray)[636] = 115;
  (*trianglesListArray)[637] = 116;
  (*trianglesListArray)[638] = 128;
  (*trianglesListArray)[639] = 115;
  (*trianglesListArray)[640] = 128;
  (*trianglesListArray)[641] = 127;
  (*trianglesListArray)[642] = 116;
  (*trianglesListArray)[643] = 117;
  (*trianglesListArray)[644] = 129;
  (*trianglesListArray)[645] = 116;
  (*trianglesListArray)[646] = 129;
  (*trianglesListArray)[647] = 128;
  (*trianglesListArray)[648] = 117;
  (*trianglesListArray)[649] = 118;
  (*trianglesListArray)[650] = 130;
  (*trianglesListArray)[651] = 117;
  (*trianglesListArray)[652] = 130;
  (*trianglesListArray)[653] = 129;
  (*trianglesListArray)[654] = 118;
  (*trianglesListArray)[655] = 119;
  (*trianglesListArray)[656] = 131;
  (*trianglesListArray)[657] = 118;
  (*trianglesListArray)[658] = 131;
  (*trianglesListArray)[659] = 130;
  (*trianglesListArray)[660] = 120;
  (*trianglesListArray)[661] = 121;
  (*trianglesListArray)[662] = 133;
  (*trianglesListArray)[663] = 120;
  (*trianglesListArray)[664] = 133;
  (*trianglesListArray)[665] = 132;
  (*trianglesListArray)[666] = 121;
  (*trianglesListArray)[667] = 122;
  (*trianglesListArray)[668] = 134;
  (*trianglesListArray)[669] = 121;
  (*trianglesListArray)[670] = 134;
  (*trianglesListArray)[671] = 133;
  (*trianglesListArray)[672] = 122;
  (*trianglesListArray)[673] = 123;
  (*trianglesListArray)[674] = 135;
  (*trianglesListArray)[675] = 122;
  (*trianglesListArray)[676] = 135;
  (*trianglesListArray)[677] = 134;
  (*trianglesListArray)[678] = 123;
  (*trianglesListArray)[679] = 124;
  (*trianglesListArray)[680] = 136;
  (*trianglesListArray)[681] = 123;
  (*trianglesListArray)[682] = 136;
  (*trianglesListArray)[683] = 135;
  (*trianglesListArray)[684] = 124;
  (*trianglesListArray)[685] = 125;
  (*trianglesListArray)[686] = 137;
  (*trianglesListArray)[687] = 124;
  (*trianglesListArray)[688] = 137;
  (*trianglesListArray)[689] = 136;
  (*trianglesListArray)[690] = 125;
  (*trianglesListArray)[691] = 126;
  (*trianglesListArray)[692] = 138;
  (*trianglesListArray)[693] = 125;
  (*trianglesListArray)[694] = 138;
  (*trianglesListArray)[695] = 137;
  (*trianglesListArray)[696] = 126;
  (*trianglesListArray)[697] = 127;
  (*trianglesListArray)[698] = 139;
  (*trianglesListArray)[699] = 126;
  (*trianglesListArray)[700] = 139;
  (*trianglesListArray)[701] = 138;
  (*trianglesListArray)[702] = 127;
  (*trianglesListArray)[703] = 128;
  (*trianglesListArray)[704] = 140;
  (*trianglesListArray)[705] = 127;
  (*trianglesListArray)[706] = 140;
  (*trianglesListArray)[707] = 139;
  (*trianglesListArray)[708] = 128;
  (*trianglesListArray)[709] = 129;
  (*trianglesListArray)[710] = 141;
  (*trianglesListArray)[711] = 128;
  (*trianglesListArray)[712] = 141;
  (*trianglesListArray)[713] = 140;
  (*trianglesListArray)[714] = 129;
  (*trianglesListArray)[715] = 130;
  (*trianglesListArray)[716] = 142;
  (*trianglesListArray)[717] = 129;
  (*trianglesListArray)[718] = 142;
  (*trianglesListArray)[719] = 141;
  (*trianglesListArray)[720] = 130;
  (*trianglesListArray)[721] = 131;
  (*trianglesListArray)[722] = 143;
  (*trianglesListArray)[723] = 130;
  (*trianglesListArray)[724] = 143;
  (*trianglesListArray)[725] = 142;
}

void CreateQuadsArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  IGeometry::MeshIndexArrayType* quadsListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, name, {121}, {4}, parentId);
  (*quadsListArray)[0] = 0;
  (*quadsListArray)[1] = 1;
  (*quadsListArray)[2] = 13;
  (*quadsListArray)[3] = 12;
  (*quadsListArray)[4] = 1;
  (*quadsListArray)[5] = 2;
  (*quadsListArray)[6] = 14;
  (*quadsListArray)[7] = 13;
  (*quadsListArray)[8] = 2;
  (*quadsListArray)[9] = 3;
  (*quadsListArray)[10] = 15;
  (*quadsListArray)[11] = 14;
  (*quadsListArray)[12] = 3;
  (*quadsListArray)[13] = 4;
  (*quadsListArray)[14] = 16;
  (*quadsListArray)[15] = 15;
  (*quadsListArray)[16] = 4;
  (*quadsListArray)[17] = 5;
  (*quadsListArray)[18] = 17;
  (*quadsListArray)[19] = 16;
  (*quadsListArray)[20] = 5;
  (*quadsListArray)[21] = 6;
  (*quadsListArray)[22] = 18;
  (*quadsListArray)[23] = 17;
  (*quadsListArray)[24] = 6;
  (*quadsListArray)[25] = 7;
  (*quadsListArray)[26] = 19;
  (*quadsListArray)[27] = 18;
  (*quadsListArray)[28] = 7;
  (*quadsListArray)[29] = 8;
  (*quadsListArray)[30] = 20;
  (*quadsListArray)[31] = 19;
  (*quadsListArray)[32] = 8;
  (*quadsListArray)[33] = 9;
  (*quadsListArray)[34] = 21;
  (*quadsListArray)[35] = 20;
  (*quadsListArray)[36] = 9;
  (*quadsListArray)[37] = 10;
  (*quadsListArray)[38] = 22;
  (*quadsListArray)[39] = 21;
  (*quadsListArray)[40] = 10;
  (*quadsListArray)[41] = 11;
  (*quadsListArray)[42] = 23;
  (*quadsListArray)[43] = 22;
  (*quadsListArray)[44] = 12;
  (*quadsListArray)[45] = 13;
  (*quadsListArray)[46] = 25;
  (*quadsListArray)[47] = 24;
  (*quadsListArray)[48] = 13;
  (*quadsListArray)[49] = 14;
  (*quadsListArray)[50] = 26;
  (*quadsListArray)[51] = 25;
  (*quadsListArray)[52] = 14;
  (*quadsListArray)[53] = 15;
  (*quadsListArray)[54] = 27;
  (*quadsListArray)[55] = 26;
  (*quadsListArray)[56] = 15;
  (*quadsListArray)[57] = 16;
  (*quadsListArray)[58] = 28;
  (*quadsListArray)[59] = 27;
  (*quadsListArray)[60] = 16;
  (*quadsListArray)[61] = 17;
  (*quadsListArray)[62] = 29;
  (*quadsListArray)[63] = 28;
  (*quadsListArray)[64] = 17;
  (*quadsListArray)[65] = 18;
  (*quadsListArray)[66] = 30;
  (*quadsListArray)[67] = 29;
  (*quadsListArray)[68] = 18;
  (*quadsListArray)[69] = 19;
  (*quadsListArray)[70] = 31;
  (*quadsListArray)[71] = 30;
  (*quadsListArray)[72] = 19;
  (*quadsListArray)[73] = 20;
  (*quadsListArray)[74] = 32;
  (*quadsListArray)[75] = 31;
  (*quadsListArray)[76] = 20;
  (*quadsListArray)[77] = 21;
  (*quadsListArray)[78] = 33;
  (*quadsListArray)[79] = 32;
  (*quadsListArray)[80] = 21;
  (*quadsListArray)[81] = 22;
  (*quadsListArray)[82] = 34;
  (*quadsListArray)[83] = 33;
  (*quadsListArray)[84] = 22;
  (*quadsListArray)[85] = 23;
  (*quadsListArray)[86] = 35;
  (*quadsListArray)[87] = 34;
  (*quadsListArray)[88] = 24;
  (*quadsListArray)[89] = 25;
  (*quadsListArray)[90] = 37;
  (*quadsListArray)[91] = 36;
  (*quadsListArray)[92] = 25;
  (*quadsListArray)[93] = 26;
  (*quadsListArray)[94] = 38;
  (*quadsListArray)[95] = 37;
  (*quadsListArray)[96] = 26;
  (*quadsListArray)[97] = 27;
  (*quadsListArray)[98] = 39;
  (*quadsListArray)[99] = 38;
  (*quadsListArray)[100] = 27;
  (*quadsListArray)[101] = 28;
  (*quadsListArray)[102] = 40;
  (*quadsListArray)[103] = 39;
  (*quadsListArray)[104] = 28;
  (*quadsListArray)[105] = 29;
  (*quadsListArray)[106] = 41;
  (*quadsListArray)[107] = 40;
  (*quadsListArray)[108] = 29;
  (*quadsListArray)[109] = 30;
  (*quadsListArray)[110] = 42;
  (*quadsListArray)[111] = 41;
  (*quadsListArray)[112] = 30;
  (*quadsListArray)[113] = 31;
  (*quadsListArray)[114] = 43;
  (*quadsListArray)[115] = 42;
  (*quadsListArray)[116] = 31;
  (*quadsListArray)[117] = 32;
  (*quadsListArray)[118] = 44;
  (*quadsListArray)[119] = 43;
  (*quadsListArray)[120] = 32;
  (*quadsListArray)[121] = 33;
  (*quadsListArray)[122] = 45;
  (*quadsListArray)[123] = 44;
  (*quadsListArray)[124] = 33;
  (*quadsListArray)[125] = 34;
  (*quadsListArray)[126] = 46;
  (*quadsListArray)[127] = 45;
  (*quadsListArray)[128] = 34;
  (*quadsListArray)[129] = 35;
  (*quadsListArray)[130] = 47;
  (*quadsListArray)[131] = 46;
  (*quadsListArray)[132] = 36;
  (*quadsListArray)[133] = 37;
  (*quadsListArray)[134] = 49;
  (*quadsListArray)[135] = 48;
  (*quadsListArray)[136] = 37;
  (*quadsListArray)[137] = 38;
  (*quadsListArray)[138] = 50;
  (*quadsListArray)[139] = 49;
  (*quadsListArray)[140] = 38;
  (*quadsListArray)[141] = 39;
  (*quadsListArray)[142] = 51;
  (*quadsListArray)[143] = 50;
  (*quadsListArray)[144] = 39;
  (*quadsListArray)[145] = 40;
  (*quadsListArray)[146] = 52;
  (*quadsListArray)[147] = 51;
  (*quadsListArray)[148] = 40;
  (*quadsListArray)[149] = 41;
  (*quadsListArray)[150] = 53;
  (*quadsListArray)[151] = 52;
  (*quadsListArray)[152] = 41;
  (*quadsListArray)[153] = 42;
  (*quadsListArray)[154] = 54;
  (*quadsListArray)[155] = 53;
  (*quadsListArray)[156] = 42;
  (*quadsListArray)[157] = 43;
  (*quadsListArray)[158] = 55;
  (*quadsListArray)[159] = 54;
  (*quadsListArray)[160] = 43;
  (*quadsListArray)[161] = 44;
  (*quadsListArray)[162] = 56;
  (*quadsListArray)[163] = 55;
  (*quadsListArray)[164] = 44;
  (*quadsListArray)[165] = 45;
  (*quadsListArray)[166] = 57;
  (*quadsListArray)[167] = 56;
  (*quadsListArray)[168] = 45;
  (*quadsListArray)[169] = 46;
  (*quadsListArray)[170] = 58;
  (*quadsListArray)[171] = 57;
  (*quadsListArray)[172] = 46;
  (*quadsListArray)[173] = 47;
  (*quadsListArray)[174] = 59;
  (*quadsListArray)[175] = 58;
  (*quadsListArray)[176] = 48;
  (*quadsListArray)[177] = 49;
  (*quadsListArray)[178] = 61;
  (*quadsListArray)[179] = 60;
  (*quadsListArray)[180] = 49;
  (*quadsListArray)[181] = 50;
  (*quadsListArray)[182] = 62;
  (*quadsListArray)[183] = 61;
  (*quadsListArray)[184] = 50;
  (*quadsListArray)[185] = 51;
  (*quadsListArray)[186] = 63;
  (*quadsListArray)[187] = 62;
  (*quadsListArray)[188] = 51;
  (*quadsListArray)[189] = 52;
  (*quadsListArray)[190] = 64;
  (*quadsListArray)[191] = 63;
  (*quadsListArray)[192] = 52;
  (*quadsListArray)[193] = 53;
  (*quadsListArray)[194] = 65;
  (*quadsListArray)[195] = 64;
  (*quadsListArray)[196] = 53;
  (*quadsListArray)[197] = 54;
  (*quadsListArray)[198] = 66;
  (*quadsListArray)[199] = 65;
  (*quadsListArray)[200] = 54;
  (*quadsListArray)[201] = 55;
  (*quadsListArray)[202] = 67;
  (*quadsListArray)[203] = 66;
  (*quadsListArray)[204] = 55;
  (*quadsListArray)[205] = 56;
  (*quadsListArray)[206] = 68;
  (*quadsListArray)[207] = 67;
  (*quadsListArray)[208] = 56;
  (*quadsListArray)[209] = 57;
  (*quadsListArray)[210] = 69;
  (*quadsListArray)[211] = 68;
  (*quadsListArray)[212] = 57;
  (*quadsListArray)[213] = 58;
  (*quadsListArray)[214] = 70;
  (*quadsListArray)[215] = 69;
  (*quadsListArray)[216] = 58;
  (*quadsListArray)[217] = 59;
  (*quadsListArray)[218] = 71;
  (*quadsListArray)[219] = 70;
  (*quadsListArray)[220] = 60;
  (*quadsListArray)[221] = 61;
  (*quadsListArray)[222] = 73;
  (*quadsListArray)[223] = 72;
  (*quadsListArray)[224] = 61;
  (*quadsListArray)[225] = 62;
  (*quadsListArray)[226] = 74;
  (*quadsListArray)[227] = 73;
  (*quadsListArray)[228] = 62;
  (*quadsListArray)[229] = 63;
  (*quadsListArray)[230] = 75;
  (*quadsListArray)[231] = 74;
  (*quadsListArray)[232] = 63;
  (*quadsListArray)[233] = 64;
  (*quadsListArray)[234] = 76;
  (*quadsListArray)[235] = 75;
  (*quadsListArray)[236] = 64;
  (*quadsListArray)[237] = 65;
  (*quadsListArray)[238] = 77;
  (*quadsListArray)[239] = 76;
  (*quadsListArray)[240] = 65;
  (*quadsListArray)[241] = 66;
  (*quadsListArray)[242] = 78;
  (*quadsListArray)[243] = 77;
  (*quadsListArray)[244] = 66;
  (*quadsListArray)[245] = 67;
  (*quadsListArray)[246] = 79;
  (*quadsListArray)[247] = 78;
  (*quadsListArray)[248] = 67;
  (*quadsListArray)[249] = 68;
  (*quadsListArray)[250] = 80;
  (*quadsListArray)[251] = 79;
  (*quadsListArray)[252] = 68;
  (*quadsListArray)[253] = 69;
  (*quadsListArray)[254] = 81;
  (*quadsListArray)[255] = 80;
  (*quadsListArray)[256] = 69;
  (*quadsListArray)[257] = 70;
  (*quadsListArray)[258] = 82;
  (*quadsListArray)[259] = 81;
  (*quadsListArray)[260] = 70;
  (*quadsListArray)[261] = 71;
  (*quadsListArray)[262] = 83;
  (*quadsListArray)[263] = 82;
  (*quadsListArray)[264] = 72;
  (*quadsListArray)[265] = 73;
  (*quadsListArray)[266] = 85;
  (*quadsListArray)[267] = 84;
  (*quadsListArray)[268] = 73;
  (*quadsListArray)[269] = 74;
  (*quadsListArray)[270] = 86;
  (*quadsListArray)[271] = 85;
  (*quadsListArray)[272] = 74;
  (*quadsListArray)[273] = 75;
  (*quadsListArray)[274] = 87;
  (*quadsListArray)[275] = 86;
  (*quadsListArray)[276] = 75;
  (*quadsListArray)[277] = 76;
  (*quadsListArray)[278] = 88;
  (*quadsListArray)[279] = 87;
  (*quadsListArray)[280] = 76;
  (*quadsListArray)[281] = 77;
  (*quadsListArray)[282] = 89;
  (*quadsListArray)[283] = 88;
  (*quadsListArray)[284] = 77;
  (*quadsListArray)[285] = 78;
  (*quadsListArray)[286] = 90;
  (*quadsListArray)[287] = 89;
  (*quadsListArray)[288] = 78;
  (*quadsListArray)[289] = 79;
  (*quadsListArray)[290] = 91;
  (*quadsListArray)[291] = 90;
  (*quadsListArray)[292] = 79;
  (*quadsListArray)[293] = 80;
  (*quadsListArray)[294] = 92;
  (*quadsListArray)[295] = 91;
  (*quadsListArray)[296] = 80;
  (*quadsListArray)[297] = 81;
  (*quadsListArray)[298] = 93;
  (*quadsListArray)[299] = 92;
  (*quadsListArray)[300] = 81;
  (*quadsListArray)[301] = 82;
  (*quadsListArray)[302] = 94;
  (*quadsListArray)[303] = 93;
  (*quadsListArray)[304] = 82;
  (*quadsListArray)[305] = 83;
  (*quadsListArray)[306] = 95;
  (*quadsListArray)[307] = 94;
  (*quadsListArray)[308] = 84;
  (*quadsListArray)[309] = 85;
  (*quadsListArray)[310] = 97;
  (*quadsListArray)[311] = 96;
  (*quadsListArray)[312] = 85;
  (*quadsListArray)[313] = 86;
  (*quadsListArray)[314] = 98;
  (*quadsListArray)[315] = 97;
  (*quadsListArray)[316] = 86;
  (*quadsListArray)[317] = 87;
  (*quadsListArray)[318] = 99;
  (*quadsListArray)[319] = 98;
  (*quadsListArray)[320] = 87;
  (*quadsListArray)[321] = 88;
  (*quadsListArray)[322] = 100;
  (*quadsListArray)[323] = 99;
  (*quadsListArray)[324] = 88;
  (*quadsListArray)[325] = 89;
  (*quadsListArray)[326] = 101;
  (*quadsListArray)[327] = 100;
  (*quadsListArray)[328] = 89;
  (*quadsListArray)[329] = 90;
  (*quadsListArray)[330] = 102;
  (*quadsListArray)[331] = 101;
  (*quadsListArray)[332] = 90;
  (*quadsListArray)[333] = 91;
  (*quadsListArray)[334] = 103;
  (*quadsListArray)[335] = 102;
  (*quadsListArray)[336] = 91;
  (*quadsListArray)[337] = 92;
  (*quadsListArray)[338] = 104;
  (*quadsListArray)[339] = 103;
  (*quadsListArray)[340] = 92;
  (*quadsListArray)[341] = 93;
  (*quadsListArray)[342] = 105;
  (*quadsListArray)[343] = 104;
  (*quadsListArray)[344] = 93;
  (*quadsListArray)[345] = 94;
  (*quadsListArray)[346] = 106;
  (*quadsListArray)[347] = 105;
  (*quadsListArray)[348] = 94;
  (*quadsListArray)[349] = 95;
  (*quadsListArray)[350] = 107;
  (*quadsListArray)[351] = 106;
  (*quadsListArray)[352] = 96;
  (*quadsListArray)[353] = 97;
  (*quadsListArray)[354] = 109;
  (*quadsListArray)[355] = 108;
  (*quadsListArray)[356] = 97;
  (*quadsListArray)[357] = 98;
  (*quadsListArray)[358] = 110;
  (*quadsListArray)[359] = 109;
  (*quadsListArray)[360] = 98;
  (*quadsListArray)[361] = 99;
  (*quadsListArray)[362] = 111;
  (*quadsListArray)[363] = 110;
  (*quadsListArray)[364] = 99;
  (*quadsListArray)[365] = 100;
  (*quadsListArray)[366] = 112;
  (*quadsListArray)[367] = 111;
  (*quadsListArray)[368] = 100;
  (*quadsListArray)[369] = 101;
  (*quadsListArray)[370] = 113;
  (*quadsListArray)[371] = 112;
  (*quadsListArray)[372] = 101;
  (*quadsListArray)[373] = 102;
  (*quadsListArray)[374] = 114;
  (*quadsListArray)[375] = 113;
  (*quadsListArray)[376] = 102;
  (*quadsListArray)[377] = 103;
  (*quadsListArray)[378] = 115;
  (*quadsListArray)[379] = 114;
  (*quadsListArray)[380] = 103;
  (*quadsListArray)[381] = 104;
  (*quadsListArray)[382] = 116;
  (*quadsListArray)[383] = 115;
  (*quadsListArray)[384] = 104;
  (*quadsListArray)[385] = 105;
  (*quadsListArray)[386] = 117;
  (*quadsListArray)[387] = 116;
  (*quadsListArray)[388] = 105;
  (*quadsListArray)[389] = 106;
  (*quadsListArray)[390] = 118;
  (*quadsListArray)[391] = 117;
  (*quadsListArray)[392] = 106;
  (*quadsListArray)[393] = 107;
  (*quadsListArray)[394] = 119;
  (*quadsListArray)[395] = 118;
  (*quadsListArray)[396] = 108;
  (*quadsListArray)[397] = 109;
  (*quadsListArray)[398] = 121;
  (*quadsListArray)[399] = 120;
  (*quadsListArray)[400] = 109;
  (*quadsListArray)[401] = 110;
  (*quadsListArray)[402] = 122;
  (*quadsListArray)[403] = 121;
  (*quadsListArray)[404] = 110;
  (*quadsListArray)[405] = 111;
  (*quadsListArray)[406] = 123;
  (*quadsListArray)[407] = 122;
  (*quadsListArray)[408] = 111;
  (*quadsListArray)[409] = 112;
  (*quadsListArray)[410] = 124;
  (*quadsListArray)[411] = 123;
  (*quadsListArray)[412] = 112;
  (*quadsListArray)[413] = 113;
  (*quadsListArray)[414] = 125;
  (*quadsListArray)[415] = 124;
  (*quadsListArray)[416] = 113;
  (*quadsListArray)[417] = 114;
  (*quadsListArray)[418] = 126;
  (*quadsListArray)[419] = 125;
  (*quadsListArray)[420] = 114;
  (*quadsListArray)[421] = 115;
  (*quadsListArray)[422] = 127;
  (*quadsListArray)[423] = 126;
  (*quadsListArray)[424] = 115;
  (*quadsListArray)[425] = 116;
  (*quadsListArray)[426] = 128;
  (*quadsListArray)[427] = 127;
  (*quadsListArray)[428] = 116;
  (*quadsListArray)[429] = 117;
  (*quadsListArray)[430] = 129;
  (*quadsListArray)[431] = 128;
  (*quadsListArray)[432] = 117;
  (*quadsListArray)[433] = 118;
  (*quadsListArray)[434] = 130;
  (*quadsListArray)[435] = 129;
  (*quadsListArray)[436] = 118;
  (*quadsListArray)[437] = 119;
  (*quadsListArray)[438] = 131;
  (*quadsListArray)[439] = 130;
  (*quadsListArray)[440] = 120;
  (*quadsListArray)[441] = 121;
  (*quadsListArray)[442] = 133;
  (*quadsListArray)[443] = 132;
  (*quadsListArray)[444] = 121;
  (*quadsListArray)[445] = 122;
  (*quadsListArray)[446] = 134;
  (*quadsListArray)[447] = 133;
  (*quadsListArray)[448] = 122;
  (*quadsListArray)[449] = 123;
  (*quadsListArray)[450] = 135;
  (*quadsListArray)[451] = 134;
  (*quadsListArray)[452] = 123;
  (*quadsListArray)[453] = 124;
  (*quadsListArray)[454] = 136;
  (*quadsListArray)[455] = 135;
  (*quadsListArray)[456] = 124;
  (*quadsListArray)[457] = 125;
  (*quadsListArray)[458] = 137;
  (*quadsListArray)[459] = 136;
  (*quadsListArray)[460] = 125;
  (*quadsListArray)[461] = 126;
  (*quadsListArray)[462] = 138;
  (*quadsListArray)[463] = 137;
  (*quadsListArray)[464] = 126;
  (*quadsListArray)[465] = 127;
  (*quadsListArray)[466] = 139;
  (*quadsListArray)[467] = 138;
  (*quadsListArray)[468] = 127;
  (*quadsListArray)[469] = 128;
  (*quadsListArray)[470] = 140;
  (*quadsListArray)[471] = 139;
  (*quadsListArray)[472] = 128;
  (*quadsListArray)[473] = 129;
  (*quadsListArray)[474] = 141;
  (*quadsListArray)[475] = 140;
  (*quadsListArray)[476] = 129;
  (*quadsListArray)[477] = 130;
  (*quadsListArray)[478] = 142;
  (*quadsListArray)[479] = 141;
  (*quadsListArray)[480] = 130;
  (*quadsListArray)[481] = 131;
  (*quadsListArray)[482] = 143;
  (*quadsListArray)[483] = 142;
}

void CreateTetAndVerticesArrays(DataStructure& dataStructure, const std::string& verticesName, const std::string& tetsName, DataObject::IdType parentId)
{
  Float32Array* vertListArray = UnitTest::CreateTestDataArray<float32>(dataStructure, verticesName, {5}, {3}, parentId);
  (*vertListArray)[0] = -1;
  (*vertListArray)[1] = 0.5;
  (*vertListArray)[2] = 0;
  (*vertListArray)[3] = 0;
  (*vertListArray)[4] = 0;
  (*vertListArray)[5] = 0;
  (*vertListArray)[6] = 0;
  (*vertListArray)[7] = 1;
  (*vertListArray)[8] = 0;
  (*vertListArray)[9] = -0.5;
  (*vertListArray)[10] = 0.5;
  (*vertListArray)[11] = 1;
  (*vertListArray)[12] = 1;
  (*vertListArray)[13] = 0.5;
  (*vertListArray)[14] = 0;
  IGeometry::MeshIndexArrayType* polyListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, tetsName, {2}, {4}, parentId);
  (*polyListArray)[0] = 0;
  (*polyListArray)[1] = 1;
  (*polyListArray)[2] = 2;
  (*polyListArray)[3] = 3;
  (*polyListArray)[4] = 1;
  (*polyListArray)[5] = 4;
  (*polyListArray)[6] = 2;
  (*polyListArray)[7] = 3;
}

void CreateHexAndVerticesArrays(DataStructure& dataStructure, const std::string& verticesName, const std::string& hexsName, DataObject::IdType parentId)
{
  Float32Array* vertListArray = UnitTest::CreateTestDataArray<float32>(dataStructure, verticesName, {12}, {3}, parentId);
  (*vertListArray)[0] = -1;
  (*vertListArray)[1] = 1;
  (*vertListArray)[2] = 1;
  (*vertListArray)[3] = -1;
  (*vertListArray)[4] = -1;
  (*vertListArray)[5] = 1;
  (*vertListArray)[6] = 0;
  (*vertListArray)[7] = -1;
  (*vertListArray)[8] = 1;
  (*vertListArray)[9] = 0;
  (*vertListArray)[10] = 1;
  (*vertListArray)[11] = 1;
  (*vertListArray)[12] = 0;
  (*vertListArray)[13] = 1;
  (*vertListArray)[14] = -1;
  (*vertListArray)[15] = -1;
  (*vertListArray)[16] = 1;
  (*vertListArray)[17] = -1;
  (*vertListArray)[18] = -1;
  (*vertListArray)[19] = -1;
  (*vertListArray)[20] = -1;
  (*vertListArray)[21] = 0;
  (*vertListArray)[22] = -1;
  (*vertListArray)[23] = -1;
  (*vertListArray)[24] = 1;
  (*vertListArray)[25] = -1;
  (*vertListArray)[26] = -1;
  (*vertListArray)[27] = 1;
  (*vertListArray)[28] = 1;
  (*vertListArray)[29] = -1;
  (*vertListArray)[30] = 1;
  (*vertListArray)[31] = 1;
  (*vertListArray)[32] = 1;
  (*vertListArray)[33] = 1;
  (*vertListArray)[34] = -1;
  (*vertListArray)[35] = 1;
  IGeometry::MeshIndexArrayType* polyListArray = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, hexsName, {2}, {8}, parentId);
  (*polyListArray)[0] = 6;
  (*polyListArray)[1] = 7;
  (*polyListArray)[2] = 4;
  (*polyListArray)[3] = 5;
  (*polyListArray)[4] = 1;
  (*polyListArray)[5] = 2;
  (*polyListArray)[6] = 3;
  (*polyListArray)[7] = 0;
  (*polyListArray)[8] = 7;
  (*polyListArray)[9] = 8;
  (*polyListArray)[10] = 9;
  (*polyListArray)[11] = 4;
  (*polyListArray)[12] = 2;
  (*polyListArray)[13] = 11;
  (*polyListArray)[14] = 10;
  (*polyListArray)[15] = 3;
}

void CreateIncompatibleVerticesArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  Float32Array* verticesArray = UnitTest::CreateTestDataArray<float32>(dataStructure, name, {2}, {3}, parentId);
  (*verticesArray)[0] = -0.707107;
  (*verticesArray)[1] = -0.707107;
  (*verticesArray)[2] = 0;
  (*verticesArray)[3] = -0.599278;
  (*verticesArray)[4] = -0.800541;
  (*verticesArray)[5] = 0;
}

TEST_CASE("SimplnxCore::CreateGeometry: Valid Execution", "[SimplnxCore][CreateGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateGeometryFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const std::string geometryName = "[Geometry Test]";
  const DataPath geometryPath({geometryName});

  const std::string vertexAmName = INodeGeometry0D::k_VertexDataName;
  const std::string edgeAmName = INodeGeometry1D::k_EdgeDataName;
  const std::string faceAmName = INodeGeometry2D::k_FaceDataName;
  const std::string cellAmName = IGridGeometry::k_CellDataName;

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
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(imageDims));
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

    const auto createdGeom = dataStructure.getDataAs<ImageGeom>(geometryPath);
    REQUIRE(createdGeom != nullptr);
    const auto cellAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(cellAmName));
    REQUIRE(cellAm != nullptr);
    REQUIRE(createdGeom->getCellData() != nullptr);
  }

  SECTION("RectGrid Geometry Copy")
  {
    Float32Array* xBoundsArray = UnitTest::CreateTestDataArray<float32>(dataStructure, xBoundsName, {14}, {1}, 0);
    (*xBoundsArray)[0] = 0;
    (*xBoundsArray)[1] = 1;
    (*xBoundsArray)[2] = 2;
    (*xBoundsArray)[3] = 3;
    (*xBoundsArray)[4] = 4;
    (*xBoundsArray)[5] = 5;
    (*xBoundsArray)[6] = 6;
    (*xBoundsArray)[7] = 7;
    (*xBoundsArray)[8] = 8;
    (*xBoundsArray)[9] = 9;
    (*xBoundsArray)[10] = 10;
    (*xBoundsArray)[11] = 11;
    (*xBoundsArray)[12] = 12;
    (*xBoundsArray)[13] = 14;
    Float32Array* yBoundsArray = UnitTest::CreateTestDataArray<float32>(dataStructure, yBoundsName, {14}, {1}, 0);
    (*yBoundsArray)[0] = 0;
    (*yBoundsArray)[1] = 2;
    (*yBoundsArray)[2] = 4;
    (*yBoundsArray)[3] = 6;
    (*yBoundsArray)[4] = 8;
    (*yBoundsArray)[5] = 10;
    (*yBoundsArray)[6] = 12;
    (*yBoundsArray)[7] = 14;
    (*yBoundsArray)[8] = 16;
    (*yBoundsArray)[9] = 18;
    (*yBoundsArray)[10] = 20;
    (*yBoundsArray)[11] = 22;
    (*yBoundsArray)[12] = 24;
    (*yBoundsArray)[13] = 26;
    Float32Array* zBoundsArray = UnitTest::CreateTestDataArray<float32>(dataStructure, zBoundsName, {14}, {1}, 0);
    (*zBoundsArray)[0] = 0;
    (*zBoundsArray)[1] = 5;
    (*zBoundsArray)[2] = 10;
    (*zBoundsArray)[3] = 15;
    (*zBoundsArray)[4] = 20;
    (*zBoundsArray)[5] = 25;
    (*zBoundsArray)[6] = 30;
    (*zBoundsArray)[7] = 35;
    (*zBoundsArray)[8] = 40;
    (*zBoundsArray)[9] = 45;
    (*zBoundsArray)[10] = 50;
    (*zBoundsArray)[11] = 55;
    (*zBoundsArray)[12] = 60;
    (*zBoundsArray)[13] = 65;

    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_RectGridGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(false));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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

    const auto createdGeom = dataStructure.getDataAs<RectGridGeom>(geometryPath);
    REQUIRE(createdGeom != nullptr);
    const auto dims = createdGeom->getDimensions();
    REQUIRE(dims == SizeVec3{13, 13, 13});
    const auto cellAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(cellAmName));
    REQUIRE(cellAm != nullptr);
    REQUIRE(createdGeom->getCellData() != nullptr);
    const auto destXBounds = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(xBoundsName));
    REQUIRE(destXBounds != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, xBoundsPath, geometryPath.createChildPath(xBoundsName));
    const auto destYBounds = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(yBoundsName));
    REQUIRE(destYBounds != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, yBoundsPath, geometryPath.createChildPath(yBoundsName));
    const auto destZBounds = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(zBoundsName));
    REQUIRE(destZBounds != nullptr);
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
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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

    const auto createdGeom = dataStructure.getDataAs<VertexGeom>(geometryPath);
    REQUIRE(createdGeom != nullptr);
    const auto vertexAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAm != nullptr);
    REQUIRE(createdGeom->getVertexAttributeMatrix() != nullptr);
    const auto destVertices = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVertices != nullptr);
    REQUIRE(createdGeom->getVertices() != nullptr);
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
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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

    const auto createdGeom = dataStructure.getDataAs<EdgeGeom>(geometryPath);
    REQUIRE(createdGeom != nullptr);

    const auto vertexAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAm != nullptr);
    REQUIRE(createdGeom->getVertexAttributeMatrix() != nullptr);
    const auto destVertices = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVertices != nullptr);
    REQUIRE(createdGeom->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcVerticesPath, geometryPath.createChildPath(vertexListName));

    const auto edgeAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(edgeAmName));
    REQUIRE(edgeAm != nullptr);
    REQUIRE(createdGeom->getEdgeAttributeMatrix() != nullptr);
    const auto destEdges = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(edgeListName));
    REQUIRE(destEdges != nullptr);
    REQUIRE(createdGeom->getEdges() != nullptr);
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
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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

    const auto createdGeom = dataStructure.getDataAs<TriangleGeom>(geometryPath);
    REQUIRE(createdGeom != nullptr);

    const auto vertexAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAm != nullptr);
    REQUIRE(createdGeom->getVertexAttributeMatrix() != nullptr);
    const auto destVertices = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVertices != nullptr);
    REQUIRE(createdGeom->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcVerticesPath, geometryPath.createChildPath(vertexListName));

    const auto faceAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(faceAmName));
    REQUIRE(faceAm != nullptr);
    REQUIRE(createdGeom->getFaceAttributeMatrix() != nullptr);
    const auto destFaces = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(triangleListName));
    REQUIRE(destFaces != nullptr);
    REQUIRE(createdGeom->getFaces() != nullptr);
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
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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

    const auto createdGeom = dataStructure.getDataAs<QuadGeom>(geometryPath);
    REQUIRE(createdGeom != nullptr);

    const auto vertexAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAm != nullptr);
    REQUIRE(createdGeom->getVertexAttributeMatrix() != nullptr);
    const auto destVertices = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVertices != nullptr);
    REQUIRE(createdGeom->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcVerticesPath, geometryPath.createChildPath(vertexListName));

    const auto quadAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(faceAmName));
    REQUIRE(quadAm != nullptr);
    REQUIRE(createdGeom->getFaceAttributeMatrix() != nullptr);
    const auto destFaces = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(quadListName));
    REQUIRE(destFaces != nullptr);
    REQUIRE(createdGeom->getFaces() != nullptr);
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
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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

    const auto createdGeom = dataStructure.getDataAs<TetrahedralGeom>(geometryPath);
    REQUIRE(createdGeom != nullptr);

    const auto vertexAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAm != nullptr);
    REQUIRE(createdGeom->getVertexAttributeMatrix() != nullptr);
    const auto destVertices = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(tetVertexListName));
    REQUIRE(destVertices != nullptr);
    REQUIRE(createdGeom->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcTetVerticesPath, geometryPath.createChildPath(tetVertexListName));

    const auto tetAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(cellAmName));
    REQUIRE(tetAm != nullptr);
    REQUIRE(createdGeom->getPolyhedraAttributeMatrix() != nullptr);
    const auto destCells = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(tetListName));
    REQUIRE(destCells != nullptr);
    REQUIRE(createdGeom->getPolyhedra() != nullptr);
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
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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

    const auto createdGeom = dataStructure.getDataAs<HexahedralGeom>(geometryPath);
    REQUIRE(createdGeom != nullptr);

    const auto vertexAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAm != nullptr);
    REQUIRE(createdGeom->getVertexAttributeMatrix() != nullptr);
    const auto destVertices = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(hexVertexListName));
    REQUIRE(destVertices != nullptr);
    REQUIRE(createdGeom->getVertices() != nullptr);
    UnitTest::CompareArrays<float32>(dataStructure, srcHexVerticesPath, geometryPath.createChildPath(hexVertexListName));

    const auto hexAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(cellAmName));
    REQUIRE(hexAm != nullptr);
    REQUIRE(createdGeom->getPolyhedraAttributeMatrix() != nullptr);
    const auto destCells = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(hexListName));
    REQUIRE(destCells != nullptr);
    REQUIRE(createdGeom->getPolyhedra() != nullptr);
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
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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

    const auto createdGeom = dataStructure.getDataAs<TriangleGeom>(geometryPath);
    REQUIRE(createdGeom != nullptr);

    const auto vertexAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(vertexAmName));
    REQUIRE(vertexAm != nullptr);
    REQUIRE(createdGeom->getVertexAttributeMatrix() != nullptr);
    const auto destVertices = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath(vertexListName));
    REQUIRE(destVertices != nullptr);
    REQUIRE(createdGeom->getVertices() != nullptr);

    const auto faceAm = dataStructure.getDataAs<AttributeMatrix>(geometryPath.createChildPath(faceAmName));
    REQUIRE(faceAm != nullptr);
    REQUIRE(createdGeom->getFaceAttributeMatrix() != nullptr);
    const auto destFaces = dataStructure.getDataAs<IGeometry::MeshIndexArrayType>(geometryPath.createChildPath(triangleListName));
    REQUIRE(destFaces != nullptr);
    REQUIRE(createdGeom->getFaces() != nullptr);

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
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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
    Float32Array* xBoundsArray = UnitTest::CreateTestDataArray<float32>(dataStructure, xBoundsName, {14}, {1}, 0);
    (*xBoundsArray)[0] = 0;
    (*xBoundsArray)[1] = 1;
    (*xBoundsArray)[2] = 2;
    (*xBoundsArray)[3] = 3;
    (*xBoundsArray)[4] = 4;
    (*xBoundsArray)[5] = 5;
    (*xBoundsArray)[6] = 6;
    (*xBoundsArray)[7] = 7;
    (*xBoundsArray)[8] = 8;
    (*xBoundsArray)[9] = 9;
    (*xBoundsArray)[10] = 14;
    (*xBoundsArray)[11] = 13;
    (*xBoundsArray)[12] = 12;
    (*xBoundsArray)[13] = 11;
    Float32Array* yBoundsArray = UnitTest::CreateTestDataArray<float32>(dataStructure, yBoundsName, {14}, {1}, 0);
    (*yBoundsArray)[0] = 0;
    (*yBoundsArray)[1] = 2;
    (*yBoundsArray)[2] = 4;
    (*yBoundsArray)[3] = 6;
    (*yBoundsArray)[4] = 8;
    (*yBoundsArray)[5] = 10;
    (*yBoundsArray)[6] = 12;
    (*yBoundsArray)[7] = 14;
    (*yBoundsArray)[8] = 16;
    (*yBoundsArray)[9] = 18;
    (*yBoundsArray)[10] = 20;
    (*yBoundsArray)[11] = 22;
    (*yBoundsArray)[12] = 24;
    (*yBoundsArray)[13] = 26;
    Float32Array* zBoundsArray = UnitTest::CreateTestDataArray<float32>(dataStructure, zBoundsName, {14}, {1}, 0);
    (*zBoundsArray)[0] = 0;
    (*zBoundsArray)[1] = 5;
    (*zBoundsArray)[2] = 10;
    (*zBoundsArray)[3] = 15;
    (*zBoundsArray)[4] = 20;
    (*zBoundsArray)[5] = 25;
    (*zBoundsArray)[6] = 30;
    (*zBoundsArray)[7] = 35;
    (*zBoundsArray)[8] = 40;
    (*zBoundsArray)[9] = 45;
    (*zBoundsArray)[10] = 50;
    (*zBoundsArray)[11] = 55;
    (*zBoundsArray)[12] = 60;
    (*zBoundsArray)[13] = 65;

    // Create default Parameters for the filter.
    args.insertOrAssign(CreateGeometryFilter::k_GeometryType_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_RectGridGeometry));
    args.insertOrAssign(CreateGeometryFilter::k_GeometryPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CreateGeometryFilter::k_WarningsAsErrors_Key, std::make_any<bool>(true));
    args.insertOrAssign(CreateGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(CreateGeometryFilter::k_CopyArray));
    // Image Geometry parameters
    args.insertOrAssign(CreateGeometryFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>());
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
}
