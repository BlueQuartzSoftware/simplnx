import simplnx as nx

ds = nx.DataStructure()

SIZE = 42
PATH = nx.DataPath(['foo'])

action = nx.CreateNeighborListAction(nx.DataType.int32, SIZE, PATH)

assert action.apply(ds, nx.IDataAction.Mode.Execute)

nl: nx.Int32NeighborList = ds[PATH]
assert nl.get_number_of_lists() == SIZE

INDEX = 2

# get_list returns a copy
assert nl.get_list(INDEX) == []
assert nl.get_list_size(INDEX) == 0

VALUE = 4

nl.add_entry(INDEX, VALUE)

assert nl.get_list(INDEX) == [VALUE]
assert nl.get_list_size(INDEX) == 1

for i in range(3):
  nl.add_entry(1, i)

for grain_id in range(nl.get_number_of_lists()):
  print(f'grain_id={grain_id}')
  for value in nl.get_list(grain_id):
    print(value)
