import collections
import numpy as np

def robdd2xbar(bdd_file):
    # read ROBDD nodes from file and build DAG
    with open(bdd_file, 'r') as f:
        lines = f.readlines()

    bdd_dag = lines[2:]
    bdd_dict = {}
    for node in bdd_dag:
        node = list(map(int,node.strip('\n').split()))
        bdd_dict[node[0]] = node[1:]

    # map ROBDD to crossbar
    bdd_dict = collections.OrderedDict(sorted(bdd_dict.items(), reverse=True)) # sort dict on descending order of keys

    xbar = np.zeros((1000,1000), dtype=int) # declare blank crossbar of size 1000x1000
    current_row = 0 # track row being mapped to
    current_col = 0 # track column being mapped to
    current_var = bdd_dict[list(bdd_dict.keys())[0]][0] # track variable being mapped
    edge_dict_aux = {}

    # obtain index of memristors in crossbar
    for key in bdd_dict:
        tmp_var = bdd_dict[key][0]
        lo_edge = bdd_dict[key][1]
        hi_edge = bdd_dict[key][2]
        edge_list = []
        if hi_edge != 0:
            edge_list.append(((tmp_var+1)*2, current_row, current_col))
            current_row += 1
        if lo_edge != 0:
            edge_list.append(((tmp_var+1)*2+1, current_row, current_col))
            current_row += 1
        edge_dict_aux[key] = edge_list
        current_col += 1

    edge_dict_aux[1] = [(1, current_row, current_col)]
    edge_dict = collections.OrderedDict(sorted(edge_dict_aux.items(), reverse=True))

    one_list = []

    # obtain index of ON memristors (edges) in crossbar
    for key in bdd_dict:
        tmp_var = bdd_dict[key][0]
        lo_edge = bdd_dict[key][1]
        hi_edge = bdd_dict[key][2]
        if hi_edge != 0:
            parent_row = edge_dict[key][0][1]
            child_col = edge_dict[hi_edge][0][2]
            one_list.append((parent_row, child_col))
        if lo_edge != 0:
            parent_row = edge_dict[key][1][1]
            child_col = edge_dict[lo_edge][0][2]
            one_list.append((parent_row, child_col))

    # map memristor indices to crossbar
    for key in edge_dict:
        tmp_node = edge_dict[key]
        for tmp_memr in tmp_node:
            memr_val, memr_row, memr_col = tmp_memr
            xbar[memr_row][memr_col] = memr_val
    for edge in one_list:
        edge_row, edge_col = edge
        xbar[edge_row][edge_col] = 1

    # truncate crossbar as filled
    xbar = xbar[:current_row+1, :]
    xbar = xbar[:, :current_col+1]

    return xbar
################################################################################

# load ROBDD specifications from file
bdd_file = "adder-4.bdd"
print(robdd2xbar(bdd_file))
