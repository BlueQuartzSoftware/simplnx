#include "DREAM3DReviewPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{073798a1-1fb4-5e3c-81f6-e426f60e347a}").value(), Uuid::FromString("6bf380da-8016-52af-bd54-f6a729ccd201").value()},
    {Uuid::FromString("{07b1048e-d6d4-56d0-8cc5-132ac79bdf60}").value(), Uuid::FromString("40505a78-f73d-57e6-95dd-accbf18098da").value()},
    {Uuid::FromString("{10b319ca-6b2f-5118-9f4e-d19ed481cd1f}").value(), Uuid::FromString("38d0d24f-e90a-573e-8aac-0bd483b02c0f").value()},
    {Uuid::FromString("{119861c5-e303-537e-b210-2e62936222e9}").value(), Uuid::FromString("ee34ef95-aa04-5ad3-8232-5783a880d279").value()},
    {Uuid::FromString("{1422a4d7-ecd3-5e84-8883-8d2c8c551675}").value(), Uuid::FromString("8af8ab11-a4d3-5045-99e3-a6d0f231576b").value()},
    {Uuid::FromString("{14ad5792-a186-5e96-8e9e-4d623919dabc}").value(), Uuid::FromString("c2547a4a-b476-56d8-a2e5-853c6e8c71c0").value()},
    {Uuid::FromString("{14f85d76-2400-57b8-9650-438563a8b8eb}").value(), Uuid::FromString("23fd021e-b649-587a-89d3-a93f76e207d6").value()},
    {Uuid::FromString("{1cf52f08-a11a-5870-a38c-ea8958071bd8}").value(), Uuid::FromString("f4f7719b-d079-5fcc-b081-ca332b0bd57d").value()},
    {Uuid::FromString("{222307a4-67fd-5cb5-a12e-d80f9fb970ae}").value(), Uuid::FromString("c6d42b50-5860-52ed-bde4-0f7bebb8623b").value()},
    {Uuid::FromString("{247ea39f-cb50-5dbb-bb9d-ecde12377fed}").value(), Uuid::FromString("559d7a0e-3bfa-5e71-9255-86eaa90845ad").value()},
    {Uuid::FromString("{28977b7a-5b88-5145-abcd-d1c933f7d975}").value(), Uuid::FromString("1b641c73-29a0-5761-85c1-a2d04075425c").value()},
    {Uuid::FromString("{3062fc2c-76b2-5c50-92b7-edbbb424c41d}").value(), Uuid::FromString("6fae2634-52bf-5fc4-b309-1f0d5cf355c8").value()},
    {Uuid::FromString("{3192d494-d1ec-5ee7-a345-e9963f02aaab}").value(), Uuid::FromString("632cae91-0b7d-562f-bd5a-f60af2b55ad5").value()},
    {Uuid::FromString("{351c38c9-330d-599c-8632-19d74868be86}").value(), Uuid::FromString("507d1945-c3e2-5a16-ac6c-4aa4721d676d").value()},
    {Uuid::FromString("{379ccc67-16dd-530a-984f-177db2314bac}").value(), Uuid::FromString("ea6c86f7-1866-543f-958c-b33c264e84ef").value()},
    {Uuid::FromString("{38fde424-0292-5c42-b3b4-18d80c95524d}").value(), Uuid::FromString("fe5d3f37-0dbc-5493-bc96-b78d18116b89").value()},
    {Uuid::FromString("{39652621-3cc4-5a72-92f3-e56c516d2b18}").value(), Uuid::FromString("6b0cfafe-84b6-58d3-b0e9-7927fcb1ed6b").value()},
    {Uuid::FromString("{3dd5d1cb-d39d-5bb6-aab5-4f64c0892e24}").value(), Uuid::FromString("3dd5d1cb-d39d-5bb6-aab5-4f64c0892e24").value()},
    {Uuid::FromString("{4178c7f9-5f90-5e95-8cf1-a67ca2a98a60}").value(), Uuid::FromString("0fe57d46-5a7d-5f92-b126-7aa2ff178538").value()},
    {Uuid::FromString("{4b551c15-418d-5081-be3f-d3aeb62408e5}").value(), Uuid::FromString("55c08183-543b-5c6a-b5a4-3d6829fac71f").value()},
    {Uuid::FromString("{52a069b4-6a46-5810-b0ec-e0693c636034}").value(), Uuid::FromString("1531f0cc-82c4-5efb-8b0f-44be9cfe2836").value()},
    {Uuid::FromString("{5a2b714e-bae9-5213-8171-d1e190609e2d}").value(), Uuid::FromString("dac4155e-bf33-54cf-9401-f9282a4c5c41").value()},
    {Uuid::FromString("{5d0cd577-3e3e-57b8-a36d-b215b834251f}").value(), Uuid::FromString("d54322fa-ee14-51fe-bbb3-b68c0265e67c").value()},
    {Uuid::FromString("{5d4c38fe-af0a-5cb3-b7fa-fb9e57b2097f}").value(), Uuid::FromString("29c6512f-5928-5eb4-8ec0-729b34ce97a4").value()},
    {Uuid::FromString("{5fa10d81-94b4-582b-833f-8eabe659069e}").value(), Uuid::FromString("7fc4e5da-66f6-5592-b7df-2428dab87c32").value()},
    {Uuid::FromString("{60b75e1c-4c65-5d86-8cb0-8b8086193d23}").value(), Uuid::FromString("f853a936-fd16-54dd-a156-8e81cc9d5f39").value()},
    {Uuid::FromString("{620a3022-0f92-5d07-b725-b22604874bbf}").value(), Uuid::FromString("161f56bb-721b-57f6-9a81-fe34c72ebcfe").value()},
    {Uuid::FromString("{676cca06-40ab-56fb-bb15-8f31c6fa8c3e}").value(), Uuid::FromString("26455785-2a9e-5c6d-8a59-a55f203beb28").value()},
    {Uuid::FromString("{6c8fb24b-5b12-551c-ba6d-ae2fa7724764}").value(), Uuid::FromString("2ad37842-acdc-5c7a-a90e-c9389883e32b").value()},
    {Uuid::FromString("{6cc3148c-0bad-53b4-9568-ee1971cadb00}").value(), Uuid::FromString("1eaa74db-9218-5528-989e-ded856c7ec7e").value()},
    {Uuid::FromString("{6f1abe57-ca7b-57ce-b03a-8c6f06fdc633}").value(), Uuid::FromString("93a4c3fa-566f-5023-82c7-891b02883dcf").value()},
    {Uuid::FromString("{71d46128-1d2d-58fd-9924-1714695768c3}").value(), Uuid::FromString("35ce6b9a-7ff8-597a-8b21-0323c24caa1e").value()},
    {Uuid::FromString("{738c8da9-45d0-53dd-aa54-3f3a337b70d7}").value(), Uuid::FromString("eaca6168-4ece-5409-b91a-29bcb9529957").value()},
    {Uuid::FromString("{73ee33b6-7622-5004-8b88-4d145514fb6a}").value(), Uuid::FromString("1c469a65-3326-53dc-92aa-a96ccaf2fd77").value()},
    {Uuid::FromString("{76ec2a58-a0cd-5548-b248-5a5eb08a1581}").value(), Uuid::FromString("bd5a4546-b75b-5731-8417-76509805f32c").value()},
    {Uuid::FromString("{81e94b15-efb6-5bae-9ab1-c74099136174}").value(), Uuid::FromString("f22a5799-bd82-56cd-b7b1-0c5eb757d362").value()},
    {Uuid::FromString("{85654f78-04a8-50ed-9ae1-25dfbd0539b3}").value(), Uuid::FromString("03e87afb-8e05-5942-807e-a76318d0790d").value()},
    {Uuid::FromString("{879e1eb8-40dc-5a5b-abe5-7e0baa77ed73}").value(), Uuid::FromString("879e1eb8-40dc-5a5b-abe5-7e0baa77ed73").value()},
    {Uuid::FromString("{8b2fa51e-3bad-58ec-9fbf-b03b065e67fc}").value(), Uuid::FromString("8b2fa51e-3bad-58ec-9fbf-b03b065e67fc").value()},
    {Uuid::FromString("{8c584519-15c3-5010-b5ed-a2ac626591a1}").value(), Uuid::FromString("27ea7655-f344-5381-a408-be18db576bce").value()},
    {Uuid::FromString("{8ef88380-ece9-5f8e-a12d-d149d0856752}").value(), Uuid::FromString("d35f6a05-30b8-544c-96d4-8bea25363dc3").value()},
    {Uuid::FromString("{9df4c18a-f51b-5698-8067-530d37f57c61}").value(), Uuid::FromString("5fa55214-a1e9-5e36-ba95-aa1968c8db8f").value()},
    {Uuid::FromString("{9fe34deb-99e1-5f3a-a9cc-e90c655b47ee}").value(), Uuid::FromString("2db3204e-a4ce-5f37-a928-65744ba06173").value()},
    {Uuid::FromString("{a250a228-3b6b-5b37-a6e4-8687484f04c4}").value(), Uuid::FromString("9488d377-c163-5752-87e4-10d607773d12").value()},
    {Uuid::FromString("{ab8f6892-2b57-5ec6-88b7-01610d80c32c}").value(), Uuid::FromString("21be3d9e-06cf-57f8-9534-1626ecc87896").value()},
    {Uuid::FromString("{b56a04de-0ca0-509d-809f-52219fca9c98}").value(), Uuid::FromString("061f74ad-9173-523a-a774-c84e07d380a0").value()},
    {Uuid::FromString("{b6b1ba7c-14aa-5c6f-9436-8c46face6053}").value(), Uuid::FromString("b74ef6ab-2a3d-533a-bd11-49aaae7a48b9").value()},
    {Uuid::FromString("{bf35f515-294b-55ed-8c69-211b7e69cb56}").value(), Uuid::FromString("e66b25f3-8158-54b4-bccc-0d058aff2b01").value()},
    {Uuid::FromString("{c2d4f1e8-2b04-5d82-b90f-2191e8f4262e}").value(), Uuid::FromString("4b6b4a2d-add0-526a-9d8a-a17e1cd23cba").value()},
    {Uuid::FromString("{c681caf4-22f2-5885-bbc9-a0476abc72eb}").value(), Uuid::FromString("2a09affe-1f88-5cbc-9251-64c85ee82203").value()},
    {Uuid::FromString("{c85277c7-9f02-5891-8141-04523658737d}").value(), Uuid::FromString("7dce1d58-7445-5ddd-8d80-043078b5e696").value()},
    {Uuid::FromString("{caed6d53-6846-523f-a837-0692b3a81570}").value(), Uuid::FromString("d5d7625a-2344-551d-8834-b838e5531866").value()},
    {Uuid::FromString("{ce1ee404-0336-536c-8aad-f9641c9458be}").value(), Uuid::FromString("dd198eed-2023-5cb0-9284-e46a17e46987").value()},
    {Uuid::FromString("{d87ff37c-120d-577d-a955-571c8280fa8e}").value(), Uuid::FromString("5ab6ea3d-5fe7-5f14-b766-67745306bd11").value()},
    {Uuid::FromString("{e15ec84b-1e02-53a6-a830-59e0813775a1}").value(), Uuid::FromString("49f80399-addb-5883-8237-c41982c05034").value()},
    {Uuid::FromString("{e4d6fda0-1143-56cc-9d00-c09a94e2f501}").value(), Uuid::FromString("e4d6fda0-1143-56cc-9d00-c09a94e2f501").value()},
    {Uuid::FromString("{ebdfe707-0c9c-5552-89f6-6ee4a1e0891b}").value(), Uuid::FromString("723ac8c2-afc1-5f32-9f11-97e06bdc05fb").value()},
    {Uuid::FromString("{ec163736-39c8-5c69-9a56-61940a337c07}").value(), Uuid::FromString("d8758182-4a77-5635-b4f9-db42323a508f").value()},
    {Uuid::FromString("{ef37f78b-bc9a-5884-b372-d882df6abe74}").value(), Uuid::FromString("c957c0a7-5c44-53e1-ba9c-48290daefd75").value()},
    {Uuid::FromString("{f2259481-5011-5f22-9fcb-c92fb6f8be10}").value(), Uuid::FromString("b694c528-2a9b-5383-ab9c-f17360e81b7a").value()},
    {Uuid::FromString("{f7486aa6-3049-5be7-8511-ae772b70c90b}").value(), Uuid::FromString("1205ab95-ceef-536a-aa38-a40d7654e5a6").value()},
    {Uuid::FromString("{f84d4d69-9ea5-54b6-a71c-df76d76d50cf}").value(), Uuid::FromString("746d10be-6b34-57af-9596-d75b27e1dc85").value()},
    {Uuid::FromString("{fab669ad-66c6-5a39-bdb7-fc47b94311ed}").value(), Uuid::FromString("0a0cff3a-6ec2-57af-9120-40dbee90c9d2").value()},
    {Uuid::FromString("{fcdde553-36b4-5731-bc88-fc499806cb4e}").value(), Uuid::FromString("1bab3f41-fdf6-5278-9b17-5db0a142392f").value()},
    {Uuid::FromString("{fd6da27d-0f2c-5c35-802f-7d6ce1ad0ca1}").value(), Uuid::FromString("f6e4b3ef-581c-55cb-8908-f44fe043bbc5").value()},
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("302815ff-93a3-55c4-85b8-99b05690e2e6");
} // namespace

DREAM3DReviewPlugin::DREAM3DReviewPlugin()
: AbstractPlugin(k_ID, "DREAM3DReview", "<<--Description was not read-->>", "Open-Source")
{
  registerFilters();
}

DREAM3DReviewPlugin::~DREAM3DReviewPlugin() = default;

std::vector<complex::H5::IDataFactory*> DREAM3DReviewPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(DREAM3DReviewPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "DREAM3DReview/plugin_filter_registration.h"
