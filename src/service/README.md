# The Cascade Service

The Cascade Service is a configurable K/V store with fast RDMA data paths. Cascade service is the easier one of the two approaches using Cascade because the application access the service through two simple APIs: [`service_client_api.hpp`](https://github.com/Derecho-Project/cascade/blob/master/include/cascade/service_client_api.hpp) and [`service_server_api`](https://github.com/Derecho-Project/cascade/blob/master/include/cascade/service_server_api.hpp) instead of using the more complicated native Derecho API. However, to learn how to use Cascade service, we still need to remind the group, subgroup, shard, and node concepts deriving from Derecho. Curious users can read our [paper](http://www.cs.cornell.edu/ken/derecho-tocs.pdf) for details on Derecho.

The cascade service is composed of a set of distributed processes talking to each other through RDMA data paths. We call each of the processes a `node`. Each `node` is identified by an integer ID. All nodes in the cascade service form a top-level `group`. The nodes doing the same job specified by a C++ Type are grouped into a `Subgroup`. Please note that we allow the C++ Subgroup Type to be reused for multiple subgroups if the logic in different subgroups is the same. One subgroup may do a huge work like managing a database table with billions of lines. In such a case, the nodes in a subgroup can be partitioned into `shards`, each of which takes care of a manageable part of the table. Inside a shard, the nodes are replicas.

The cascade service comes with four pre-defined subgroup types: `VCSU`, `VCSS`, `PCSU`, and `PCSS`, where V for Volatile, P for Persistent, S for 'Store', and the last character is for the key type of the object ('U' for uint64_t and 'S' for std::string). Please refer to [`service_types.hpp`](https://github.com/Derecho-Project/cascade/blob/master/include/cascade/service_types.hpp) for details of those types. All four subgroup types expose a K/V API (see [ICascadeStore](https://github.com/Derecho-Project/cascade/blob/master/include/cascade/cascade.hpp)). The persistent types support versioned and timestamped queries.

Once the cascade service is configured and started, the application can store and retrieve the data using the client API defined in [`service_client_api.hpp`](https://github.com/Derecho-Project/cascade/blob/master/include/cascade/service_client_api.hpp). Core to the client API is an `external client` talking to the Cascade services with an efficient RDMA data path. Please check [`client.cpp`](https://github.com/Derecho-Project/cascade/blob/master/src/service/client.cpp) for how to use the client API.

Cascade service also allows the application to insert logic on the data path. In order for that, the application needs to implement the cascade server API in [`service_server_api.hpp`](https://github.com/Derecho-Project/cascade/blob/master/include/cascade/service_server_api.hpp). We provide an example implementation in [`ondata_library_example.cpp`](https://github.com/Derecho-Project/cascade/blob/master/src/service/ondata_library_example.cpp).

# Configuring Cascade Service
Cascade derives Derecho's configuration file. Besides the derecho configurations, Cascade added a section called `[CASCADE]` in the same file to configure the Cascade service. There are only two options in this section: `ondata_library` and `group_layout`. `ondata_library` specifies the dynamic library containing the server APIs implementation. `group_layout` specifies the group, subgroup, and shard layout of the cascade service. Please read the comments below for how to describe a layout. 

```
[CASCADE]
# Cascade server allows application-defined behavior. The behavior is divided into two parts: one is on-critical data
# path, the other is off-critical data path. The behavior API is defined in <cascade/service_server_api.hpp>. An
# application using this feature needs to implement that API and create a shared library. And then tell the server
# where to find it by "ondata_library". We show a reference implementation in cascade source code:
#     <cascade_source>/src/service/ondata_library_exmaple.cpp
# "ondata_library" is defaulted to empty
ondata_library = 
# Specify group layout here. The layout specifies the following items:
# - How many subgroups of corresponding type (see note below) to create?
# - For each subgroup, how many shards to create?
# - For each shard, what are the minimum number of nodes required(min_nodes_by_shard, defaulted to 1), the maximum
#   number of nodes allowed(max_nodes_by_shard, defaulted to 1), the delivery mode(delivery_modes_by_shard, either
#   "Ordered" or "Raw", defaulted to "Ordered"), and the profile name(profiles_by_shard, defaulted to "DEFAULT")
# Derecho parameters in "[SUBGROUP/<profile>]" will be used for the corresponding shard.
# 
# The setup is defined in a json array, where each element is a dictionary specifying the layout for a corresponding
# subgroup type. OK, I mentioned "corresponding subgroup type" again and here is the mapping between the configuration
# elements and types used to define a Cascade service --- in the cascade service server code, we started a derecho
# group with a list of types, which currently given as "VCSU,VCSS,PCSU,PCSS"; each of the type in the list CORRESPONDS
# to an entry in the layout json array defined here, following the order in the type list. Therefore, with the current
# type list setup, the layout has four elements with the 1st for type VCSU, the 2nd for VCSS, the 3rd for PCSU, and the
# 4th for PCSS. You can define more elements than types, but the rest are ignored without side effect.
# 
# Each dictionary element has two keys: "type_alias" and "layout". The "type_alias" specifies the human-readable name
# (string) for the corresponding (sigh...the first stressless "corresponding") type. The "layout" define, with a json 
# array, the setup of subgroups of this type. Each element (a layout dict) for a subgroup. Each layout dict has four
# entries corresponding to the above four items. The value for each entry is an array whose length is equal to the
# number of shards. Each element in that array is a setting for the corresponding shard.
# For example, the following configuration defined two subgroups of VCSU type. The first subgroup has one shard and the
# second subgroup has three shards
#
# group_layout = '
# [
#     {
#         "type_alias":  "VCSU",
#         "layout":     [
#                           {
#                               "min_nodes_by_shard": [1],
#                               "max_nodes_by_shard": [1],
#                               "delivery_modes_by_shard": ["Ordered"],
#                               "profiles_by_shard": ["DEFAULT"]
#                           },
#                           {
#                               "min_nodes_by_shard": [1,3,5],
#                               "max_nodes_by_shard": [3,5,7],
#                               "delivery_modes_by_shard": ["Ordered","Ordered","Raw"],
#                               "profiles_by_shard": ["VCS2_SHARD1","VCS2_SHARD2","VCS2_SHARD3"]
#                           }
#                       ]
#     },
#     { ... },
#     { ... },
#     { ... }
#
# ]
# '
# Please make sure a pair of single quotation marks ' are used around the json. GetPot formation enforced that for
# multiline values.
group_layout = 
```

# To Run the Cascade Service Example
Once cascade is built, you will find five folders named `n0` to `n4` in `<build_dir>/src/service/cfg/`. Each of them contains a configuration file to run a demo service node with localhost IP and RDMA API layer over TCP/IP. In the folder, start the node by calling:
```
# ../../server
```
Since the demo service requires four nodes to start running, let's start the server nodes in `n0`, `n1`, `n2`, and `n3` and leave `n4` for the client by calling
```
# ../../client
```
Once the client connnects to the service, it is going to show prompt for command.
```
cmd> help
list_all_members
        list all members in top level derecho group.
list_type_members <type> [subgroup_index] [shard_index]
        list members in shard by subgroup type.
list_subgroup_members [subgroup_id] [shard_index]
        list members in shard by subgroup id.
set_member_selection_policy <type> <subgroup_index> <shard_index> <policy> [user_specified_node_id]
        set member selection policy
get_member_selection_policy <type> [subgroup_index] [shard_index]
        get member selection policy
put <type> <key> <value> [subgroup_index] [shard_index]
        put an object
remove <type> <key> [subgroup_index] [shard_index]
        remove an object
get <type> <key> [version] [subgroup_index] [shard_index]
        get an object(by version)
get_by_time <type> <key> <ts_us> [subgroup_index] [shard_index]
        get an object by timestamp
get_size <type> <key> [version] [subgroup_index] [shard_index]
        get the size of an object(by version)
get_size_by_time <type> <key> <ts_us> [subgroup_index] [shard_index]
        get the size of an object by timestamp
list_keys <type> [version] [subgroup_index] [shard_index]
        list keys in shard (by version)
list_keys_by_time <type> <ts_us> [subgroup_index] [shard_index]
        list keys in shard by time
quit|exit
        exit the client.
help
        print this message.

type:=VCSU|VCSS|PCSU|PCSS
policy:=FirstMember|LastMember|Random|FixedRandom|RoundRobin|UserSpecified

cmd>
```
Then you can try process like this:
```
cmd> put VCSU 100 ABCDEFG
cmd> get VCSU 100
node(0) replied with value:ObjectWithUInt64Key{ver: 0x1200000000, ts: 1599366091779929, id:100, data:[size:7, data: A B C D E F G]}
```

# The File System API
We also provided a file system API to Cascade. The API is implemented as a libfuse driver talking to the service through an `external client`. Once mounted, the file system presents the data in the following structure:
```
<mount_point>/<subgroupType>/subgroup_index/shard_index/Key
```
You can also check the ".cascade" file to get Cascade service properties like membership. Currently, the fuse file system is ReadOnly. We are working on adding time/version index and write supports to it. 

To start the file system API, run `fuse_client` in n4:
```
# mkdir fcc
# ../../fuse_client -s -f fcc 
```
Then, the file system api will mount on `fcc` folder.
```
# ls fcc
PCSS  PCSU  VCSS  VCSU
# cat fcc/.cascade
number of nodes in cascade service: 4.
node IDs: 0,1,3,2,
# cat fcc/VCSU/subgroup-0/shard-0/key100
hdABCDEFG#
```
Please note that the file contents are deserialized byte array of the corresponding object.
