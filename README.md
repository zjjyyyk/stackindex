## Compile
- make

## Build Graph
Based on the random arrival model, generate the initial graph and the edge update.
```sh
# convert the input graph from a text format to a binary format
format <data_path> --directed|undirected <--undirected> --skipstart  --onestart
[meta, graph]

# split the binary graph: basic graphs and the remaining edges for insertion
divide <data_path> --base_ratio <size ratio of basic graph>
[graph_base, edges_del, edges_ins]

# generate workloads
# workload format:
#   i<num insert>d<num delete>q<num query>k<topk>
#   <topk> = 0 to generate full queries
process <data_path> [workloads]
[workloads/<workload>]
```
Example: 
```sh
# The dataset dblp is an undirected graph
./format datasets/dblp --skipstart

# we randomly select 90% edges from the oringal graph as the inital graph 
./divide datasets/dblp --base_ratio 0.9

# generate five workloads
./process datasets/dblp i50d50q0k0
./process datasets/dblp i37d37q25k0
./process datasets/dblp i25d25q50k0
./process datasets/dblp i12d12q75k0
./process datasets/dblp i0d0q100k0
```

# Run Experiments
```sh
./build_time <data_path> stackindex|rwindex|realtime <save_dir>
./exp_query <data_path> <alpha> stackindex|rwindex <truth_dir> <save_dir>
./alpha_update <data_path> stackindex|rwindex <truth_dir> <save_dir>
./edge_update <data_path> stackindex|rwindex|realtime <workload> <save_dir>
```

Example:
```sh
./build_time datasets/dblp stackindex exps/exp_results/exp_query/build_time/dblp
./exp_query datasets/dblp 0.2 stackindex groundTruth/pagerank/singlesource/dblp/0.20/ exps/exp_results/exp_query/time_err/dblp
./alpha_update dataset/dblp stackindex groundTruth/pagerank/singlesource/dblp exps/exp_results/exp_update/alpha_update/dblp
./edge_update datasets/dblp stackindex i12d12q75k0 exps/exp_results/exp_update/edge_update/dblp
```
