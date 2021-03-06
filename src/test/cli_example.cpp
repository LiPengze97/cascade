#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <derecho/core/derecho.hpp>
#include <derecho/utils/logger.hpp>
#include <cascade/cascade.hpp>
#include <cascade/object.hpp>
#include <derecho/mutils-serialization/SerializationSupport.hpp>

using namespace derecho::cascade;
using derecho::ExternalClientCaller;

#ifdef __CDT_PARSER__
#define REGISTER_RPC_FUNCTIONS(...)
#define RPC_NAME(...) 0ULL
#endif

using std::cout;
using std::endl;

inline uint64_t get_time_us()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static void print_help(const char* cmd_str) {
    std::cout << "Usage: " << cmd_str << " [(derecho options) --] <server|client>" << std::endl;
    return;
}

typedef ObjectWithUInt64Key obj_t;

using PCS = PersistentCascadeStore<uint64_t,ObjectWithUInt64Key,&ObjectWithUInt64Key::IK,&ObjectWithUInt64Key::IV,ST_FILE>;


class PerfCascadeWatcher : public CascadeWatcher<uint64_t,ObjectWithUInt64Key,&ObjectWithUInt64Key::IK,&ObjectWithUInt64Key::IV> {
public:
    // @overload
    void operator () (derecho::subgroup_id_t sid,
               const uint32_t shard_id,
               const uint64_t& key,
               const ObjectWithUInt64Key& value,
               void* cascade_context) {
                dbg_default_info("Watcher is called with\n\tsubgroup id = {},\n\tshard number = {},\n\tkey = {},\n\tvalue = [hidden].", sid, shard_id, key);
    }
};


/************ Start of Server Replicated State *************/

class S_state: public mutils::ByteRepresentable,
               public derecho::PersistsFields,
               public derecho::GroupReference {

    using derecho::GroupReference::group;
    uint64_t high_timestamp;
    node_id_t my_id;

public:
    PCS Core;
    S_state(PCS& _Core, uint64_t _high_timestamp, node_id_t _my_id):
        Core(std::move(_Core)),
        high_timestamp(_high_timestamp),
        my_id(_my_id) {}

    S_state(persistent::PersistentRegistry* pr, 
            CascadeWatcher<uint64_t, ObjectWithUInt64Key, &ObjectWithUInt64Key::IK, &ObjectWithUInt64Key::IV>* cw,
            uint64_t _high_timestamp,
            node_id_t _my_id):
        Core(PCS(pr, cw)),
        high_timestamp(_high_timestamp),
        my_id(_my_id) {}

    uint64_t get_high_timestamp() { return high_timestamp; }

    uint64_t put(const obj_t& value);

    void ordered_put(const obj_t& value, uint64_t new_high_ts, node_id_t primary_id);

    const obj_t get(uint64_t key);

    DEFAULT_SERIALIZATION_SUPPORT(S_state, Core, high_timestamp, my_id);
    REGISTER_RPC_FUNCTIONS(S_state, get_high_timestamp, put, ordered_put, get);
};

uint64_t S_state::put(const obj_t& value) {
    dbg_default_debug("Enter S_state: put");
    derecho::Replicated<S_state>& subgroup_handle = group->get_subgroup<S_state>(this->subgroup_index);
    uint64_t new_high_ts = get_time_us();
    dbg_default_debug("new high timestamp = {}", new_high_ts);
    this->high_timestamp = new_high_ts;
    if (!this->Core.persistent_core->ordered_put(value, this->Core.persistent_core.getLatestVersion())) {
        dbg_default_error("put failed");
    }
    subgroup_handle.ordered_send<RPC_NAME(ordered_put)>(value, this->high_timestamp, this->my_id);
    dbg_default_debug("leaving S_state: put");
    return new_high_ts;
}

void S_state::ordered_put(const obj_t& value, uint64_t new_high_ts, node_id_t primary_id) {
    dbg_default_debug("Enter S_state ordered_put");
    if (this->my_id == primary_id) return;
    this->high_timestamp = new_high_ts;
    if (!this->Core.persistent_core->ordered_put(value, this->Core.persistent_core.getLatestVersion())) {
        dbg_default_error("put failed");
    }
}

const obj_t S_state::get(uint64_t key) {
    return this->Core.persistent_core->ordered_get(key);
}
/****************************************************************/



// helper functions
static std::vector<std::string> tokenize(std::string& line) {
    std::vector<std::string> tokens;
    char line_buf[1024];
    std::strcpy(line_buf, line.c_str());
    char *token = std::strtok(line_buf, " ");
    while (token != nullptr) {
        tokens.push_back(std::string(token));
        token = std::strtok(NULL, " ");
    }
    return tokens; // RVO
}

class M_state: public mutils::ByteRepresentable,
               public derecho::PersistsFields,
               public derecho::GroupReference {
    // Coefs
    uint64_t* HTimeStamp;
    uint64_t* latency;

    std::mutex H_mutex[10];
    std::mutex L_mutex[10];

public:
    M_state(uint64_t*& _H, uint64_t*& _L) : 
        HTimeStamp(std::move(_H)), latency(std::move(_L)) {}
    M_state() {
        HTimeStamp = latency = NULL;
    }

    void init(const uint32_t N) {
        dbg_default_debug("initializing monitor");
        HTimeStamp = new uint64_t[N];
        latency = new uint64_t[N];
        dbg_default_debug("monitor initialized");
    }

    uint64_t get_hts(node_id_t id) {
        H_mutex[id].lock();
        uint64_t ret = HTimeStamp[id];
        H_mutex[id].unlock();
        return ret;
    }

    uint64_t get_lag(node_id_t id) {
        L_mutex[id].lock();
        uint64_t ret = latency[id];
        L_mutex[id].unlock();
        return ret;
    }

    bool update(node_id_t id, uint64_t H, uint64_t L, bool Push) {
        if (H_mutex[id].try_lock()) {
            HTimeStamp[id] = H;
            H_mutex[id].unlock();
        }
        if (L_mutex[id].try_lock()) {
            if (!Push) {
                latency[id] = latency[id] / 10 * 9 + L / 10;
            }
            else latency[id] = L;
            L_mutex[id].unlock();
        }
        return 1;
    }

    bool trigger() { return 1; }

    DEFAULT_SERIALIZATION_SUPPORT(M_state, HTimeStamp, latency);
    REGISTER_RPC_FUNCTIONS(M_state, init, trigger, get_hts, get_lag, update);
} ;

void do_monitor(node_id_t my_id, derecho::Group<S_state, M_state>& G, uint64_t HeartBeat = 1000) {
    dbg_default_debug("209");
    std::vector<node_id_t> server_ids = G.get_subgroup_members<S_state>(0)[0];

    cout << "------------- Server List -------------" << endl;
    for (int i = 0; i < server_ids.size(); ++i) {
        cout << server_ids[i] << ' ';
    }
    cout << endl;
    cout << "---------------------------------------" << endl;

    derecho::ExternalCaller<S_state>& server_handler = G.get_nonmember_subgroup<S_state>();
    dbg_default_debug("get server subgroup handler");
    derecho::Replicated<M_state>& monitor_handler = G.get_subgroup<M_state>();
    dbg_default_debug("get monitor subgroup handler");
    node_id_t primary_site = server_ids[0];

    int sz = server_ids.size();
    auto init_lock = monitor_handler.p2p_send<RPC_NAME(init)>(my_id, sz);
    init_lock.get();
    dbg_default_debug("Contacting each serer");
    for (int i = 0; i < sz; ++i) {
        auto tic = get_time_us();
        auto res = server_handler.p2p_send<RPC_NAME(get_high_timestamp)>(server_ids[i]);
        auto reply = res.get().get(server_ids[i]);
        auto cur_lag = get_time_us() - tic;
        auto update_lock = monitor_handler.p2p_send<RPC_NAME(update)>(my_id, server_ids[i], reply, cur_lag, 1);
        update_lock.get();
    }
    dbg_default_info("Established connection with each server");
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(HeartBeat));
        for (int i = 0; i < sz; ++i) {
            auto tic = get_time_us();
            auto res = server_handler.p2p_send<RPC_NAME(get_high_timestamp)>(server_ids[i]);
            auto reply = res.get().get(server_ids[i]);
            auto cur_lag = get_time_us() - tic;
            auto update_lock = monitor_handler.p2p_send<RPC_NAME(update)>(my_id, server_ids[i], reply, cur_lag, 0);
            update_lock.get();
        }
    }
}

namespace Client {

    std::vector<node_id_t> servers;
    std::vector<node_id_t> monitors;
    int my_monitor;

    #define CST_LEVEL_EVENTUAL 0
    #define CST_LEVEL_STRONG 1

    void init_client(derecho::ExternalGroup<S_state, M_state>& group) {

        dbg_default_debug("Start initialize client");

        servers = group.template get_shard_members<S_state>(0, 0);
        std::cout << "Servers in top derecho group:[ ";
        for(auto& nid: servers) {
            std::cout << nid << " ";
        }
        std::cout << "]" << std::endl;

        monitors = group.template get_shard_members<M_state>(0, 0);
        std::cout << "Monitors in top derecho group:[ ";
        for(auto& nid: monitors) {
            std::cout << nid << " ";
        }
        std::cout << "]" << std::endl;

        int sz = servers.size();

        ExternalClientCaller<S_state, std::remove_reference<decltype(group)>::type>& server_ec = group.get_subgroup_caller<S_state>();
        for (int i = 0; i < sz; ++i) {
            auto nid = servers[i];
            uint64_t tic = get_time_us();
            auto res = server_ec.p2p_send<RPC_NAME(get_high_timestamp)>(nid);
            auto reply = res.get().get(nid);
            uint64_t cur_lag = get_time_us() - tic;
            dbg_default_debug("Servers {}, latency = {}", nid, cur_lag);
        }
        dbg_default_debug("Successfully contacted each server");

        ExternalClientCaller<M_state, std::remove_reference<decltype(group)>::type>& monitor_ec = group.get_subgroup_caller<M_state>();
        sz = monitors.size();
        for (int i = 0; i < sz; ++i) {
            auto nid = monitors[i];
            uint64_t tic = get_time_us();
            auto res = monitor_ec.p2p_send<RPC_NAME(trigger)>(nid);
            uint64_t cur_lag = get_time_us() - tic;
            dbg_default_debug("Monitors {}, latency = {}", nid, cur_lag);
        }
        dbg_default_debug("Successfully contacted each monitor");

        my_monitor = monitors[0];
        dbg_default_debug("My monitor = {}", my_monitor);
    }

    // put
    static void client_put(derecho::ExternalGroup<S_state, M_state>& group,
                           node_id_t primary_id,
                           const std::vector<std::string>& tokens) {
        if (tokens.size() != 3) {
            std::cout << "Invalid format of 'put' command." << std::endl;
        }
        auto tic = get_time_us();
        uint64_t key = std::stoll(tokens[1]);
        
        //TODO: the previous_version should be used to enforce version check. INVALID_VERSION disables the feature.
        ObjectWithUInt64Key o(key,Blob(tokens[2].c_str(), tokens[2].size()));

        ExternalClientCaller<S_state, std::remove_reference<decltype(group)>::type>& server_ec = group.get_subgroup_caller<S_state>();
        auto result = server_ec.p2p_send<RPC_NAME(put)>(primary_id, o);
        auto reply = result.get().get(primary_id);
        cout << "put finished, timestamp = " << reply << ", latency = " << (get_time_us() - tic) / 1000 << "(ms)" << endl;
    }

    uint64_t get_min_acc_ts(int consistency_L) {
        if (consistency_L == 0) {
            return 0;
        } else if (consistency_L == 1) {
            return uint64_t(-1);
        } else {
            cout << "Invalid Consistency Level, return (INFINITY)-ish" << endl;
            return uint64_t(-1);
        }
    }

    auto get_best_server(derecho::ExternalGroup<S_state, M_state>& group,
                         uint64_t min_acc_ts) {
        int sz = servers.size();
        uint64_t min_latency = -1;
        auto ret = servers[0];
        ExternalClientCaller<M_state, std::remove_reference<decltype(group)>::type>& monitor_ec = group.get_subgroup_caller<M_state>();
        for (int i = 0; i < sz; ++i) {
            auto nid = servers[i];
            auto res_hts = monitor_ec.p2p_send<RPC_NAME(get_hts)>(my_monitor, nid);
            auto res_lag = monitor_ec.p2p_send<RPC_NAME(get_lag)>(my_monitor, nid);
            auto hts = res_hts.get().get(my_monitor);
            auto latency = res_lag.get().get(my_monitor);
            if (hts >= min_acc_ts && latency < min_latency) {
                min_latency = latency;
                ret = nid;
            }
        }
        return ret;
    }

    // get
    static void client_get(derecho::ExternalGroup<S_state, M_state>& group,
                           const std::vector<std::string>& tokens) {
        auto tic = get_time_us();
        if (tokens.size() != 2) {
            std::cout << "Invalid format of 'get' command." << std::endl;
            return;
        }


        uint64_t key = std::stoll(tokens[1]);
        auto chosen_server = get_best_server(group, get_min_acc_ts(CST_LEVEL_EVENTUAL));

        std::optional<derecho::rpc::QueryResults<const ObjectWithUInt64Key>> opt;
        ExternalClientCaller<S_state, std::remove_reference<decltype(group)>::type>& pcs_ec = group.get_subgroup_caller<S_state>();
        opt.emplace(pcs_ec.p2p_send<RPC_NAME(get)>(chosen_server, key));
        auto reply = opt.value().get().get(chosen_server);
        cout << "Consistency level = EVENTUAL " << "chosen server = " << chosen_server << endl;
        std::cout << "get finished with object:" << reply << " latency = " << (get_time_us() - tic) / 1000 << endl;
    }

    //echo server-latency-hightimestamp pair
    static void client_echo(derecho::ExternalGroup<S_state, M_state>& group) {
        int sz = servers.size();
        ExternalClientCaller<M_state, std::remove_reference<decltype(group)>::type>& monitor_ec = group.get_subgroup_caller<M_state>();
        cout << "------------ Server Table ------------" << endl;
        for (int i = 0; i < sz; ++i) {
            auto nid = servers[i];
            auto res_hts = monitor_ec.p2p_send<RPC_NAME(get_hts)>(my_monitor, nid);
            auto res_lag = monitor_ec.p2p_send<RPC_NAME(get_lag)>(my_monitor, nid);
            auto hts = res_hts.get().get(my_monitor);
            auto latency = res_lag.get().get(my_monitor);
            cout << nid << ' ' << latency << ' ' << hts << endl;
        }
        cout << "---------------------------------------" << endl;
    }

    void do_client() {
        /** 0 - create External Group */
        derecho::ExternalGroup<S_state, M_state> group;

        /** 1 - initialize client */
        init_client(group);

        /** 2 - run command line. */
        while(true) {
            std::string cmdline;
            std::cout << "cmd> " << std::flush;
            std::getline(std::cin, cmdline);
            auto cmd_tokens = tokenize(cmdline);
            if (cmd_tokens.size() == 0) {
                continue;
            }

            if (cmd_tokens[0].compare("put") == 0) {
                client_put(group, servers[0], cmd_tokens);
            } else if (cmd_tokens[0].compare("get") == 0) {
                client_get(group, cmd_tokens);
            } else if (cmd_tokens[0].compare("echo") == 0) {
                client_echo(group);
            } else if (cmd_tokens[0].compare("quit") == 0 ||
                       cmd_tokens[0].compare("exit") == 0) {
                std::cout << "Exiting client." << std::endl;
                break;
            } else {
                std::cout << "Unknown command:" << cmd_tokens[0] << std::endl;
            }
        }
    }
}

void do_server_monitor(node_id_t this_node_id, bool is_monitor = 0) {
    dbg_default_info("Starting cascade server.");

    /** 1 - group building blocks*/
    derecho::CallbackSet callback_set {
        nullptr,    // delivery callback
        nullptr,    // local persistence callback
        nullptr     // global persistence callback
    };
    derecho::SubgroupInfo si {
        derecho::DefaultSubgroupAllocator({
            {std::type_index(typeid(S_state)), 
             derecho::one_subgroup_policy(derecho::fixed_even_shards(1, 3))},
            {std::type_index(typeid(M_state)),
             derecho::one_subgroup_policy(derecho::fixed_even_shards(1, 1))}
        })
    };
	PerfCascadeWatcher pcw;
    auto server_factory = [&pcw, this_node_id](persistent::PersistentRegistry* pr, derecho::subgroup_id_t) {
        return std::make_unique<S_state>(pr, &pcw, -1, this_node_id);
    };
    auto monitor_factory = [](persistent::PersistentRegistry*, derecho::subgroup_id_t) {
        return std::make_unique<M_state>();
    };
    /** 2 - create group */
    derecho::Group<S_state, M_state> group(callback_set,si,{&pcw}/*deserialization manager*/,
                                  std::vector<derecho::view_upcall_t>{},
                                  server_factory, monitor_factory);
    std::cout << "Finished constructing Derecho group." << std::endl;

    if (is_monitor) {
        cout << "Entering \"do_monitor\" " << endl;
        do_monitor(this_node_id, group, 1000);
    }
    else {
        std::cout << "Press ENTER to shutdown..." << std::endl;
        std::cin.get();
        group.barrier_sync();
        group.leave();
        dbg_default_info("Cascade server shutdown.");
    }
}

int main(int argc, char** argv) {
    /** initialize the parameters */
    derecho::Conf::initialize(argc,argv);

    /** check parameters */
    if (argc < 2) {
        print_help(argv[0]);
        return -1;
    }

    if (std::string("client").compare(argv[argc-1]) == 0) {
        Client::do_client();
    } else if (std::string("server").compare(argv[argc-1]) == 0) {
        do_server_monitor(derecho::getConfUInt32(CONF_DERECHO_LOCAL_ID));
    } else if (std::string("monitor").compare(argv[argc - 1]) == 0) {
        do_server_monitor(derecho::getConfUInt32(CONF_DERECHO_LOCAL_ID), 1);
    } else {
        std::cerr << "Unknown mode:" << argv[argc-1] << std::endl;
        print_help(argv[0]);
        return -1;
    }
    return 0;
}
