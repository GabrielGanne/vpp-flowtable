#!/bin/env python
from __future__ import print_function

import vpp_papi as vpp 

if_1_name = 'TenGigabitEthernet83/0/0'
if_2_name = 'TenGigabitEthernet83/0/1'
if_3_name = 'netmap-vale00:pm'

r = vpp.connect('papi')

if_1_sw_if_index = vpp.sw_interface_dump(1, if_1_name)[0].sw_if_index
if_2_sw_if_index = vpp.sw_interface_dump(1, if_2_name)[0].sw_if_index
if_3_sw_if_index = vpp.sw_interface_dump(1, if_3_name)[0].sw_if_index

# add flowtable as available classifier next nodes
r = vpp.add_node_next("l2-input-classify", "flowtable-process")
print(r)
ft_idx = r.node_index;

# add, table_index, nbuckets, memory_size, skip_n_vectors, match_n_vectors, next_table_index, miss_next_index, mask
cl0 = vpp.classify_add_del_table(1, 0xffffffff, 64, 1024*1024, 0, 1, 0xffffffff, ft_idx, '')
print(cl0)

# input -> 1, output -> 0
r = vpp.classify_set_interface_l2_tables(if_1_sw_if_index, cl0.new_table_index, 0xffffffff, 0xffffffff, 1)
print(r)

r = vpp.disconnect()
exit(r)
