import simplnx as nx

ds = nx.DataStructure()

SIZE = 42
PATH = nx.DataPath(['foo'])

action = nx.CreateNeighborListAction(nx.DataType.int32, SIZE, PATH)

assert action.apply(ds, nx.IDataAction.Mode.Execute)

nl: nx.Int32NeighborList = ds[PATH]
assert nl.get_number_of_lists() == SIZE

INDEX = 2

assert nl.get_list(INDEX) == []
assert nl.get_list_size(INDEX) == 0

VALUE = 4

nl.add_entry(INDEX, VALUE)

assert nl.get_list(INDEX) == [VALUE]
assert nl.get_list_size(INDEX) == 1
