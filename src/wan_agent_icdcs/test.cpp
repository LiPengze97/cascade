#include <cascade/cascade.hpp>
#include <cascade/object.hpp>
#include <chrono>
#include <derecho/core/derecho.hpp>
#include <derecho/utils/logger.hpp>
#include <derecho/mutils-serialization/SerializationSupport.hpp>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sstream>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <tuple>
#include <unistd.h>
#include <vector>

using namespace derecho::cascade;
using namespace std;
using namespace mutils;
using derecho::ExternalClientCaller;

using WPCSU = WANPersistentCascadeStore<uint64_t, ObjectWithUInt64Key, &ObjectWithUInt64Key::IK, &ObjectWithUInt64Key::IV, ST_FILE>;
using WPCSS = WANPersistentCascadeStore<std::string, ObjectWithStringKey, &ObjectWithStringKey::IK, &ObjectWithStringKey::IV, ST_FILE>;

#define SHUTDOWN_SERVER_PORT (2300)
#define SLEEP_GRANULARITY_US (50)

inline uint64_t get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int do_client() {

    /** 1 - create external client group*/
    derecho::ExternalGroup<WPCSU, WPCSS> group;
    uint32_t my_node_id = derecho::getConfUInt32(CONF_DERECHO_LOCAL_ID);
    
    ExternalClientCaller<WPCSS, std::remove_reference<decltype(group)>::type>& wpcss_ec = group.get_subgroup_caller<WPCSS>();
    auto members = group.template get_shard_members<WPCSS>(0, 0);
    node_id_t server_id = members[my_node_id % members.size()];

    cout << my_node_id << ' ' << members.size() << ' ' << server_id << endl;

    uint64_t message_index = 0;
    wpcss_ec.p2p_send<RPC_NAME(start_wanagent)>(server_id);
    uint64_t start_time = get_time_us();
    cout << "start time" << start_time << endl;
    
    string op;
    char buf[derecho::getConfUInt64(CONF_SUBGROUP_DEFAULT_MAX_PAYLOAD_SIZE)];
    string key;

    DeserializationManager DSM = {{}};

    string read_request_buf = "READ_REQUEST";
    while(cin >> op) {
        if (op == "read") {
            cin >> key;
            ObjectWithStringKey o(key, Blob(read_request_buf.c_str(), read_request_buf.size()));
            auto res = wpcss_ec.p2p_send<RPC_NAME(read)>(server_id, o, key);
            wan_agent::Blob obj_bytes = res.get().get(server_id);
            cerr << "message size = " << obj_bytes.size << endl;
            if (strcmp(obj_bytes.bytes, "NO_SUCH_KEY") == 0) {
                cerr << obj_bytes.bytes << endl;
            }
            else {
                auto obj = from_bytes<ObjectWithStringKey>(&DSM, obj_bytes.bytes);
                cerr << (*obj) << endl;
            }
        } else if (op == "write") {
            cin >> key >> buf;
            ObjectWithStringKey o(key, Blob(buf, strlen(buf)));
            auto res = wpcss_ec.p2p_send<RPC_NAME(write)>(server_id, o, key);
            cerr << res.get().get(server_id) << endl;
        } else {
            cerr << "Invalid Operator" << endl;
        }
    }

    dbg_default_debug("leaving do_client");

    return 0;
}

int main(int argc, char** argv) {
    /** initialize the parameters */
    derecho::Conf::initialize(argc, argv);

    dbg_default_debug("initialization done!");

    do_client();
}
