#!/bin/env python
from __future__ import print_function

import vpp_papi as vpp 

from vpp_papi.portmirroring import *
import vpp_papi.portmirroring as pm

if_1_name = 'tap-0'
if_2_name = 'tap-1'
if_3_name = 'tap-2'

r = vpp.connect('papi')

if_1_sw_if_index = vpp.sw_interface_dump(1, if_1_name)[0].sw_if_index
if_2_sw_if_index = vpp.sw_interface_dump(1, if_2_name)[0].sw_if_index
if_3_sw_if_index = vpp.sw_interface_dump(1, if_3_name)[0].sw_if_index

# add port-mirroring as available classifier next nodes
r = vpp.add_node_next("l2-input-classify", "pm-in-hit")
print(r)
pm_in_hit_idx = r.node_index;

r = vpp.add_node_next("l2-output-classify", "pm-out-hit")
print(r)
pm_out_hit_idx = r.node_index;

# configure portmirroring
# 0 -> classifier, 0 -> add
r = pm.pm_conf(if_3_sw_if_index, 0, 0)
print(r)

# add, table_index, nbuckets, memory_size, skip_n_vectors, match_n_vectors, next_table_index, miss_next_index, mask
cl0 = vpp.classify_add_del_table(1, 0xffffffff, 64, 1024*1024, 0, 1, 0xffffffff, pm_in_hit_idx, '')
print(cl0)
cl1 = vpp.classify_add_del_table(1, 0xffffffff, 64, 1024*1024, 0, 1, 0xffffffff, pm_out_hit_idx, '')
print(cl1)

# input -> 1, output -> 0
r = vpp.classify_set_interface_l2_tables(if_1_sw_if_index, cl0.new_table_index, 0xffffffff, 0xffffffff, 1)
print(r)
r = vpp.classify_set_interface_l2_tables(if_1_sw_if_index, cl1.new_table_index, 0xffffffff, 0xffffffff, 0)
print(r)

r = vpp.disconnect()
exit(r)
