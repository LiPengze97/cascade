#pragma once
#include <memory>
#include <map>

namespace derecho
{
    namespace cascade
    {

#define debug_enter_func_with_args(format, ...) \
    dbg_default_debug("Entering {} with parameter:" #format ".", __func__, __VA_ARGS__)
#define debug_leave_func_with_value(format, ...) \
    dbg_default_debug("Leaving {} with " #format ".", __func__, __VA_ARGS__)
#define debug_enter_func() dbg_default_debug("Entering {}.")
#define debug_leave_func() dbg_default_debug("Leaving {}.")

        ///////////////////////////////////////////////////////////////////////////////
        // 1 - Volatile Cascade Store Implementation
        ///////////////////////////////////////////////////////////////////////////////

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::tuple<persistent::version_t, uint64_t> VolatileCascadeStore<KT, VT, IK, IV>::put(const VT &value)
        {
            debug_enter_func_with_args("value.key={}", value.key);
            derecho::Replicated<VolatileCascadeStore> &subgroup_handle = group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_put)>(value);
            auto &replies = results.get();
            std::tuple<persistent::version_t, uint64_t> ret(CURRENT_VERSION, 0);
            // TODO: verfiy consistency ?
            for (auto &reply_pair : replies)
            {
                ret = reply_pair.second.get();
            }
            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(ret), std::get<1>(ret));
            return ret;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::tuple<persistent::version_t, uint64_t> VolatileCascadeStore<KT, VT, IK, IV>::remove(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);
            derecho::Replicated<VolatileCascadeStore> &subgroup_handle = group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_remove)>(key);
            auto &replies = results.get();
            std::tuple<persistent::version_t, uint64_t> ret(CURRENT_VERSION, 0);
            // TODO: verify consistency ?
            for (auto &reply_pair : replies)
            {
                ret = reply_pair.second.get();
            }
            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(ret), std::get<1>(ret));
            return ret;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        const VT VolatileCascadeStore<KT, VT, IK, IV>::get(const KT &key, const persistent::version_t &ver)
        {
            debug_enter_func_with_args("key={},ver=0x{:x}", key, ver);
            if (ver != CURRENT_VERSION)
            {
                debug_leave_func_with_value("Cannot support versioned get, ver=0x{:x}", ver);
                return *IV;
            }
            derecho::Replicated<VolatileCascadeStore> &subgroup_handle = group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_get)>(key);
            auto &replies = results.get();
            // TODO: verify consistency ?
            // for (auto& reply_pair : replies) {
            //     ret = reply_pair.second.get();
            // }
            debug_leave_func();
            return replies.begin()->second.get();
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        const VT VolatileCascadeStore<KT, VT, IK, IV>::get_by_time(const KT &key, const uint64_t &ts_us)
        {
            // VolatileCascadeStore does not support this.
            debug_enter_func();
            debug_leave_func();

            return *IV;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::vector<KT> VolatileCascadeStore<KT, VT, IK, IV>::list_keys(const persistent::version_t &ver)
        {
            debug_enter_func_with_args("ver=0x{:x}", ver);
            if (ver != CURRENT_VERSION)
            {
                debug_leave_func_with_value("Cannot support versioned list_keys, ver=0x{:x}", ver);
                return {};
            }
            derecho::Replicated<VolatileCascadeStore> &subgroup_handle = group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_list_keys)>();
            auto &replies = results.get();
            std::vector<KT> ret;
            // TODO: verfity consistency ?
            for (auto &reply_pair : replies)
            {
                ret = reply_pair.second.get();
            }
            debug_leave_func();
            return ret;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::vector<KT> VolatileCascadeStore<KT, VT, IK, IV>::list_keys_by_time(const uint64_t &ts_us)
        {
            // VolatileCascadeStore does not support this.
            debug_enter_func_with_args("ts_us=0x{:x}", ts_us);
            debug_leave_func();
            return {};
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        uint64_t VolatileCascadeStore<KT, VT, IK, IV>::get_size(const KT &key, const persistent::version_t &ver)
        {
            debug_enter_func_with_args("key={},ver=0x{:x}", key, ver);
            if (ver != CURRENT_VERSION)
            {
                debug_leave_func_with_value("Cannot support versioned get, ver=0x{:x}", ver);
                return 0;
            }
            derecho::Replicated<VolatileCascadeStore> &subgroup_handle = group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_get_size)>(key);
            auto &replies = results.get();
            // TODO: verify consistency ?
            // for (auto& reply_pair : replies) {
            //     ret = reply_pair.second.get();
            // }
            debug_leave_func();
            return replies.begin()->second.get();
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        uint64_t VolatileCascadeStore<KT, VT, IK, IV>::get_size_by_time(const KT &key, const uint64_t &ts_us)
        {
            // VolatileCascadeStore does not support this.
            debug_enter_func();

            debug_leave_func();
            return 0;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::vector<KT> VolatileCascadeStore<KT, VT, IK, IV>::ordered_list_keys()
        {
            std::vector<KT> key_list;
            debug_enter_func();
            for (auto kv : this->kv_map)
            {
                key_list.push_back(kv.first);
            }
            debug_leave_func();
            return key_list;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::tuple<persistent::version_t, uint64_t> VolatileCascadeStore<KT, VT, IK, IV>::ordered_put(const VT &value)
        {
            debug_enter_func_with_args("key={}", value.key);

            std::tuple<persistent::version_t, uint64_t> version = group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index).get_next_version();
            this->kv_map.erase(value.key);
            value.ver = version;
            this->kv_map.emplace(value.key, value); // copy constructor
            if (cascade_watcher_ptr)
            {
                (*cascade_watcher_ptr)(
                    group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index).get_subgroup_id(),
                    group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index).get_shard_num(),
                    value.key, value, nullptr /*cascade_context*/);
            }

            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(version), std::get<1>(version));

            return version;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::tuple<persistent::version_t, uint64_t> VolatileCascadeStore<KT, VT, IK, IV>::ordered_remove(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);

            std::tuple<persistent::version_t, uint64_t> version = group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index).get_next_version();
            if (this->kv_map.erase(key))
            {
                if (cascade_watcher_ptr)
                {
                    (*cascade_watcher_ptr)(
                        group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index).get_subgroup_id(),
                        group->template get_subgroup<VolatileCascadeStore>(this->subgroup_index).get_shard_num(),
                        key, *IV, nullptr /*cascade_context*/);
                }
            }

            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(version), std::get<1>(version));

            return version;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        const VT VolatileCascadeStore<KT, VT, IK, IV>::ordered_get(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);

            if (this->kv_map.find(key) != this->kv_map.end())
            {
                debug_leave_func_with_value("key={}", key);
                return this->kv_map.at(key);
            }
            else
            {
                debug_leave_func();
                return *IV;
            }
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        uint64_t VolatileCascadeStore<KT, VT, IK, IV>::ordered_get_size(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);

            if (this->kv_map.find(key) != this->kv_map.end())
            {
                return mutils::bytes_size(this->kv_map.at(key));
            }
            else
            {
                debug_leave_func();
                return 0;
            }
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::unique_ptr<VolatileCascadeStore<KT, VT, IK, IV>> VolatileCascadeStore<KT, VT, IK, IV>::from_bytes(
            mutils::DeserializationManager *dsm,
            char const *buf)
        {
            auto kv_map_ptr = mutils::from_bytes<std::map<KT, VT>>(dsm, buf);
            auto volatile_cascade_store_ptr =
                std::make_unique<VolatileCascadeStore>(std::move(*kv_map_ptr), &(dsm->mgr<CascadeWatcher<KT, VT, IK, IV>>()));
            return volatile_cascade_store_ptr;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        VolatileCascadeStore<KT, VT, IK, IV>::VolatileCascadeStore(
            CascadeWatcher<KT, VT, IK, IV> *cw) : cascade_watcher_ptr(cw)
        {
            debug_enter_func();
            debug_leave_func();
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        VolatileCascadeStore<KT, VT, IK, IV>::VolatileCascadeStore(
            const std::map<KT, VT> &_kvm, CascadeWatcher<KT, VT, IK, IV> *cw) : kv_map(_kvm),
                                                                                cascade_watcher_ptr(cw)
        {
            debug_enter_func_with_args("copy to kv_map, size={}", kv_map.size());
            debug_leave_func();
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        VolatileCascadeStore<KT, VT, IK, IV>::VolatileCascadeStore(
            std::map<KT, VT> &&_kvm, CascadeWatcher<KT, VT, IK, IV> *cw) : kv_map(std::move(_kvm)),
                                                                           cascade_watcher_ptr(cw)
        {
            debug_enter_func_with_args("move to kv_map, size={}", kv_map.size());
            debug_leave_func();
        }

        ///////////////////////////////////////////////////////////////////////////////
        // 2 - Persistent Cascade Store Implementation
        ///////////////////////////////////////////////////////////////////////////////
        template <typename KT, typename VT, KT *IK, VT *IV>
        void DeltaCascadeStoreCore<KT, VT, IK, IV>::_Delta::set_opid(DeltaCascadeStoreCore<KT, VT, IK, IV>::_OPID opid)
        {
            assert(buffer != nullptr);
            assert(capacity >= sizeof(uint32_t));
            *(DeltaCascadeStoreCore::_OPID *)buffer = opid;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        void DeltaCascadeStoreCore<KT, VT, IK, IV>::_Delta::set_data_len(const size_t &dlen)
        {
            assert(capacity >= (dlen + sizeof(uint32_t)));
            this->len = dlen + sizeof(uint32_t);
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        char *DeltaCascadeStoreCore<KT, VT, IK, IV>::_Delta::data_ptr()
        {
            assert(buffer != nullptr);
            assert(capacity > sizeof(uint32_t));
            return buffer + sizeof(uint32_t);
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        void DeltaCascadeStoreCore<KT, VT, IK, IV>::_Delta::calibrate(const size_t &dlen)
        {
            size_t new_cap = dlen + sizeof(uint32_t);
            if (this->capacity >= new_cap)
            {
                return;
            }
            // calculate new capacity
            int width = sizeof(size_t) << 3;
            int right_shift_bits = 1;
            new_cap--;
            while (right_shift_bits < width)
            {
                new_cap |= new_cap >> right_shift_bits;
                right_shift_bits = right_shift_bits << 1;
            }
            new_cap++;
            // resize
            this->buffer = (char *)realloc(buffer, new_cap);
            if (this->buffer == nullptr)
            {
                dbg_default_crit("{}:{} Failed to allocate delta buffer. errno={}", __FILE__, __LINE__, errno);
                throw derecho::derecho_exception("Failed to allocate delta buffer.");
            }
            else
            {
                this->capacity = new_cap;
            }
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        bool DeltaCascadeStoreCore<KT, VT, IK, IV>::_Delta::is_empty()
        {
            return (this->len == 0);
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        void DeltaCascadeStoreCore<KT, VT, IK, IV>::_Delta::clean()
        {
            this->len = 0;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        void DeltaCascadeStoreCore<KT, VT, IK, IV>::_Delta::destroy()
        {
            if (this->capacity > 0)
            {
                free(this->buffer);
            }
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        void DeltaCascadeStoreCore<KT, VT, IK, IV>::initialize_delta()
        {
            delta.buffer = (char *)malloc(DEFAULT_DELTA_BUFFER_CAPACITY);
            if (delta.buffer == nullptr)
            {
                dbg_default_crit("{}:{} Failed to allocate delta buffer. errno={}", __FILE__, __LINE__, errno);
                throw derecho::derecho_exception("Failed to allocate delta buffer.");
            }
            delta.capacity = DEFAULT_DELTA_BUFFER_CAPACITY;
            delta.len = 0;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        void DeltaCascadeStoreCore<KT, VT, IK, IV>::finalizeCurrentDelta(const persistent::DeltaFinalizer &df)
        {
            df(this->delta.buffer, this->delta.len);
            this->delta.clean();
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        void DeltaCascadeStoreCore<KT, VT, IK, IV>::applyDelta(char const *const delta)
        {
            const char *data = (delta + sizeof(const uint32_t));
            switch (*reinterpret_cast<const uint32_t *>(delta))
            {
            case PUT:
                apply_ordered_put(*mutils::from_bytes<VT>(nullptr, data));
                break;
            case REMOVE:
                apply_ordered_remove(*mutils::from_bytes<KT>(nullptr, data));
                break;
            default:
                std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " " << std::endl;
            };
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::unique_ptr<DeltaCascadeStoreCore<KT, VT, IK, IV>> DeltaCascadeStoreCore<KT, VT, IK, IV>::create(mutils::DeserializationManager *dm)
        {
            if (dm != nullptr)
            {
                try
                {
                    return std::make_unique<DeltaCascadeStoreCore<KT, VT, IK, IV>>();
                }
                catch (...)
                {
                }
            }
            return std::make_unique<DeltaCascadeStoreCore<KT, VT, IK, IV>>();
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        void DeltaCascadeStoreCore<KT, VT, IK, IV>::apply_ordered_put(const VT &value)
        {
            // put
            this->kv_map.erase(value.key);
            this->kv_map.emplace(value.key, value);
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        bool DeltaCascadeStoreCore<KT, VT, IK, IV>::apply_ordered_remove(const KT &key)
        {
            bool ret = false;
            // remove
            if (this->kv_map.erase(key))
            {
                ret = true;
            }
            return ret;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        bool DeltaCascadeStoreCore<KT, VT, IK, IV>::ordered_put(const VT &value)
        {
            // create delta.
            assert(this->delta.is_empty());
            this->delta.calibrate(mutils::bytes_size(value));
            mutils::to_bytes(value, this->delta.data_ptr());
            this->delta.set_data_len(mutils::bytes_size(value));
            this->delta.set_opid(DeltaCascadeStoreCore<KT, VT, IK, IV>::PUT);
            // apply ordered_put
            apply_ordered_put(value);
            return true;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        bool DeltaCascadeStoreCore<KT, VT, IK, IV>::ordered_remove(const KT &key)
        {
            // create delta.
            assert(this->delta.is_empty());
            this->delta.calibrate(mutils::bytes_size(key));
            mutils::to_bytes(key, this->delta.data_ptr());
            this->delta.set_data_len(mutils::bytes_size(key));
            this->delta.set_opid(DeltaCascadeStoreCore<KT, VT, IK, IV>::REMOVE);
            // remove
            return apply_ordered_remove(key);
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        const VT DeltaCascadeStoreCore<KT, VT, IK, IV>::ordered_get(const KT &key)
        {
            if (kv_map.find(key) != kv_map.end())
            {
                return kv_map.at(key);
            }
            else
            {
                return *IV;
            }
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        std::vector<KT> DeltaCascadeStoreCore<KT, VT, IK, IV>::ordered_list_keys()
        {
            std::vector<KT> key_list;
            for (auto &kv : kv_map)
            {
                key_list.push_back(kv.first);
            }
            return key_list;
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        uint64_t DeltaCascadeStoreCore<KT, VT, IK, IV>::ordered_get_size(const KT &key)
        {
            if (kv_map.find(key) != kv_map.end())
            {
                return mutils::bytes_size(kv_map.at(key));
            }
            else
            {
                return 0;
            }
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        DeltaCascadeStoreCore<KT, VT, IK, IV>::DeltaCascadeStoreCore()
        {
            initialize_delta();
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        DeltaCascadeStoreCore<KT, VT, IK, IV>::DeltaCascadeStoreCore(const std::map<KT, VT> &_kv_map) : kv_map(_kv_map)
        {
            initialize_delta();
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        DeltaCascadeStoreCore<KT, VT, IK, IV>::DeltaCascadeStoreCore(std::map<KT, VT> &&_kv_map) : kv_map(_kv_map)
        {
            initialize_delta();
        }

        template <typename KT, typename VT, KT *IK, VT *IV>
        DeltaCascadeStoreCore<KT, VT, IK, IV>::~DeltaCascadeStoreCore()
        {
            if (this->delta.buffer != nullptr)
            {
                free(this->delta.buffer);
            }
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::tuple<persistent::version_t, uint64_t> PersistentCascadeStore<KT, VT, IK, IV, ST>::put(const VT &value)
        {
            debug_enter_func_with_args("value.key={}", value.key);
            derecho::Replicated<PersistentCascadeStore> &subgroup_handle = group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_put)>(value);
            auto &replies = results.get();
            std::tuple<persistent::version_t, uint64_t> ret(CURRENT_VERSION, 0);
            // TODO: verfiy consistency ?
            for (auto &reply_pair : replies)
            {
                ret = reply_pair.second.get();
            }
            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(ret), std::get<1>(ret));
            return ret;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::tuple<persistent::version_t, uint64_t> PersistentCascadeStore<KT, VT, IK, IV, ST>::remove(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);
            derecho::Replicated<PersistentCascadeStore> &subgroup_handle = group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_remove)>(key);
            auto &replies = results.get();
            std::tuple<persistent::version_t, uint64_t> ret(CURRENT_VERSION, 0);
            // TODO: verify consistency ?
            for (auto &reply_pair : replies)
            {
                ret = reply_pair.second.get();
            }
            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(ret), std::get<1>(ret));
            return ret;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        const VT PersistentCascadeStore<KT, VT, IK, IV, ST>::get(const KT &key, const persistent::version_t &ver)
        {
            debug_enter_func_with_args("key={},ver=0x{:x}", key, ver);
            if (ver != CURRENT_VERSION)
            {
                return persistent_core.get(ver)->kv_map.at(key);
            }
            derecho::Replicated<PersistentCascadeStore> &subgroup_handle = group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_get)>(key);
            auto &replies = results.get();
            // TODO: verify consistency ?
            // for (auto& reply_pair : replies) {
            //     ret = reply_pair.second.get();
            // }
            debug_leave_func();
            return replies.begin()->second.get();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        const VT PersistentCascadeStore<KT, VT, IK, IV, ST>::get_by_time(const KT &key, const uint64_t &ts_us)
        {
            debug_enter_func_with_args("key={},ts_us={}", key, ts_us);
            const HLC hlc(ts_us, 0ull);
            try
            {
                debug_leave_func();
                return persistent_core.get(hlc)->kv_map.at(key);
            }
            catch (const int64_t &ex)
            {
                dbg_default_warn("temporal query throws exception:0x{:x}. key={}, ts={}", ex, key, ts_us);
            }
            catch (...)
            {
                dbg_default_warn("temporal query throws unknown exception. key={}, ts={}", key, ts_us);
            }
            debug_leave_func();
            return *IV;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        uint64_t PersistentCascadeStore<KT, VT, IK, IV, ST>::get_size(const KT &key, const persistent::version_t &ver)
        {
            debug_enter_func_with_args("key={},ver=0x{:x}", key, ver);
            if (ver != CURRENT_VERSION)
            {
                return mutils::bytes_size(persistent_core.get(ver)->kv_map.at(key));
            }
            derecho::Replicated<PersistentCascadeStore> &subgroup_handle = group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_get_size)>(key);
            auto &replies = results.get();
            // TODO: verify consistency ?
            // for (auto& reply_pair : replies) {
            //     ret = reply_pair.second.get();
            // }
            debug_leave_func();
            return replies.begin()->second.get();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        uint64_t PersistentCascadeStore<KT, VT, IK, IV, ST>::get_size_by_time(const KT &key, const uint64_t &ts_us)
        {
            debug_enter_func_with_args("key={},ts_us={}", key, ts_us);
            const HLC hlc(ts_us, 0ull);
            try
            {
                debug_leave_func();
                return mutils::bytes_size(persistent_core.get(hlc)->kv_map.at(key));
            }
            catch (const int64_t &ex)
            {
                dbg_default_warn("temporal query throws exception:0x{:x}. key={}, ts={}", ex, key, ts_us);
            }
            catch (...)
            {
                dbg_default_warn("temporal query throws unknown exception. key={}, ts={}", key, ts_us);
            }
            debug_leave_func();
            return 0;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::vector<KT> PersistentCascadeStore<KT, VT, IK, IV, ST>::list_keys(const persistent::version_t &ver)
        {
            debug_enter_func_with_args("ver=0x{:x}.", ver);
            if (ver != CURRENT_VERSION)
            {
                std::vector<KT> key_list;
                auto kv_map = persistent_core.get(ver)->kv_map;
                for (auto &kv : kv_map)
                {
                    key_list.push_back(kv.first);
                }
                debug_leave_func();
                return key_list;
            }
            derecho::Replicated<PersistentCascadeStore> &subgroup_handle = group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_list_keys)>();
            auto &replies = results.get();
            // TODO: verify consistency ?
            debug_leave_func();
            return replies.begin()->second.get();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::vector<KT> PersistentCascadeStore<KT, VT, IK, IV, ST>::list_keys_by_time(const uint64_t &ts_us)
        {
            debug_enter_func_with_args("ts_us={}", ts_us);
            const HLC hlc(ts_us, 0ull);
            try
            {
                auto kv_map = persistent_core.get(hlc)->kv_map;
                std::vector<KT> key_list;
                for (auto &kv : kv_map)
                {
                    key_list.push_back(kv.first);
                }
                debug_leave_func();
                return key_list;
            }
            catch (const int64_t &ex)
            {
                dbg_default_warn("temporal query throws exception:0x{:x]. ts={}", ex, ts_us);
            }
            catch (...)
            {
                dbg_default_warn("temporal query throws unknown exception. ts={}", ts_us);
            }
            debug_leave_func();
            return {};
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::tuple<persistent::version_t, uint64_t> PersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_put(const VT &value)
        {
            debug_enter_func_with_args("key={}", value.key);

            std::tuple<persistent::version_t, uint64_t> version = group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index).get_next_version();
            value.ver = version;
            this->persistent_core->ordered_put(value);
            if (cascade_watcher_ptr)
            {
                (*cascade_watcher_ptr)(
                    group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index).get_subgroup_id(),
                    group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index).get_shard_num(),
                    value.key, value, nullptr /*cascade context*/);
            }

            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(version), std::get<1>(version));

            return version;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::tuple<persistent::version_t, uint64_t> PersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_remove(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);

            std::tuple<persistent::version_t, uint64_t> version = group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index).get_next_version();
            if (this->persistent_core->ordered_remove(key))
            {
                if (cascade_watcher_ptr)
                {
                    (*cascade_watcher_ptr)(
                        group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index).get_subgroup_id(),
                        group->template get_subgroup<PersistentCascadeStore>(this->subgroup_index).get_shard_num(),
                        key, *IV, nullptr /*cascade context*/);
                }
            }

            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(version), std::get<1>(version));

            return version;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        const VT PersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_get(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);

            debug_leave_func();

            return this->persistent_core->ordered_get(key);
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        uint64_t PersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_get_size(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);

            debug_leave_func();

            return this->persistent_core->ordered_get_size(key);
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::vector<KT> PersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_list_keys()
        {
            debug_enter_func();

            debug_leave_func();

            return this->persistent_core->ordered_list_keys();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::unique_ptr<PersistentCascadeStore<KT, VT, IK, IV, ST>> PersistentCascadeStore<KT, VT, IK, IV, ST>::from_bytes(mutils::DeserializationManager *dsm, char const *buf)
        {
            auto persistent_core_ptr = mutils::from_bytes<persistent::Persistent<DeltaCascadeStoreCore<KT, VT, IK, IV>, ST>>(dsm, buf);
            auto persistent_cascade_store_ptr =
                std::make_unique<PersistentCascadeStore>(std::move(*persistent_core_ptr), &(dsm->mgr<CascadeWatcher<KT, VT, IK, IV>>()));
            return persistent_cascade_store_ptr;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        PersistentCascadeStore<KT, VT, IK, IV, ST>::PersistentCascadeStore(
            persistent::PersistentRegistry *pr,
            CascadeWatcher<KT, VT, IK, IV> *cw)
            : persistent_core([]() {
                  return std::make_unique<DeltaCascadeStoreCore<KT, VT, IK, IV>>();
              },
                              nullptr, pr),
              cascade_watcher_ptr(cw) {}

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        PersistentCascadeStore<KT, VT, IK, IV, ST>::PersistentCascadeStore(
            persistent::Persistent<DeltaCascadeStoreCore<KT, VT, IK, IV>, ST> &&
                _persistent_core,
            CascadeWatcher<KT, VT, IK, IV> *cw) : persistent_core(std::move(_persistent_core)),
                                                  cascade_watcher_ptr(cw) {}

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        PersistentCascadeStore<KT, VT, IK, IV, ST>::PersistentCascadeStore(
            PersistentCascadeStore<KT, VT, IK, IV, ST> &&_persistent_cascade_store)
            : persistent_core(std::move(_persistent_cascade_store.persistent_core)),
              cascade_watcher_ptr(std::move(_persistent_cascade_store.cascade_watcher_ptr.get()))
        {
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        PersistentCascadeStore<KT, VT, IK, IV, ST>::~PersistentCascadeStore() {}

        ///////////////////////////////////////////////////////////////////////////////
        // 3 - WAN Persistent Cascade Store Implementation
        ///////////////////////////////////////////////////////////////////////////////
        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::tuple<persistent::version_t, uint64_t> WANPersistentCascadeStore<KT, VT, IK, IV, ST>::put(const VT &value)
        {
            debug_enter_func_with_args("value.key={}", value.key);
            derecho::Replicated<WANPersistentCascadeStore> &subgroup_handle = group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_put)>(value);
            auto &replies = results.get();
            std::tuple<persistent::version_t, uint64_t> ret(CURRENT_VERSION, 0);
            // TODO: verfiy consistency ?
            for (auto &reply_pair : replies)
            {
                ret = reply_pair.second.get();
            }
            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(ret), std::get<1>(ret));

            // TODO: need wan support

            // byte_size/actual_size may be larger than WAN_AGENT_MAX_PAYLOAD_SIZE
            size_t byte_size = value.bytes_size();
            char *buffer = static_cast<char *>(malloc(byte_size));
            size_t actual_size = value.to_bytes(buffer);
            std::cout << "Alloc size: " << byte_size << ", actual size: " << actual_size << std::endl;
            if (byte_size != actual_size)
            {
                throw std::runtime_error("to_bytes() and bytes_size() return different size");
            }

            // do not use wan_agent::WAN_AGENT_MAX_PAYLOAD_SIZE, macro define is not constrained by namespace, even class definition, function definition
            const size_t wan_max_payload_size = wan_conf_json[WAN_AGENT_MAX_PAYLOAD_SIZE];

            if (actual_size < wan_max_payload_size)
            {
                wan_agent_sender->send(buffer, actual_size);
            }
            else // need divide into blocks
            {
                while (actual_size > wan_max_payload_size)
                {
                    wan_agent_sender->send(buffer, wan_max_payload_size);
                    buffer += wan_max_payload_size;
                    actual_size -= wan_max_payload_size;
                }
            }

            debug_leave_func_with_value("Send to WanAgent server {} bytes.", actual_size);
            return ret;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::tuple<persistent::version_t, uint64_t> WANPersistentCascadeStore<KT, VT, IK, IV, ST>::remove(const KT &key)
        {

            ///////////////////////////////////////////////////////////////////////////////
            // TODO: It makes sense to send the remove command to the WanAgent's server
            // as well,but the WanAgent's server doesn't actually store any data at the
            // moment, so it's ignored.
            ///////////////////////////////////////////////////////////////////////////////

            debug_enter_func_with_args("key={}", key);
            derecho::Replicated<WANPersistentCascadeStore> &subgroup_handle = group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_remove)>(key);
            auto &replies = results.get();
            std::tuple<persistent::version_t, uint64_t> ret(CURRENT_VERSION, 0);
            // TODO: verify consistency ?
            for (auto &reply_pair : replies)
            {
                ret = reply_pair.second.get();
            }
            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(ret), std::get<1>(ret));

            return ret;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        const VT WANPersistentCascadeStore<KT, VT, IK, IV, ST>::get(const KT &key, const persistent::version_t &ver)
        {
            debug_enter_func_with_args("key={},ver=0x{:x}", key, ver);
            if (ver != CURRENT_VERSION)
            {
                return persistent_core.get(ver)->kv_map.at(key);
            }
            derecho::Replicated<WANPersistentCascadeStore> &subgroup_handle = group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_get)>(key);
            auto &replies = results.get();
            // TODO: verify consistency ?
            // for (auto& reply_pair : replies) {
            //     ret = reply_pair.second.get();
            // }
            debug_leave_func();
            return replies.begin()->second.get();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        const VT WANPersistentCascadeStore<KT, VT, IK, IV, ST>::get_by_time(const KT &key, const uint64_t &ts_us)
        {
            debug_enter_func_with_args("key={},ts_us={}", key, ts_us);
            const HLC hlc(ts_us, 0ull);
            try
            {
                debug_leave_func();
                return persistent_core.get(hlc)->kv_map.at(key);
            }
            catch (const int64_t &ex)
            {
                dbg_default_warn("temporal query throws exception:0x{:x}. key={}, ts={}", ex, key, ts_us);
            }
            catch (...)
            {
                dbg_default_warn("temporal query throws unknown exception. key={}, ts={}", key, ts_us);
            }
            debug_leave_func();
            return *IV;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        uint64_t WANPersistentCascadeStore<KT, VT, IK, IV, ST>::get_size(const KT &key, const persistent::version_t &ver)
        {
            debug_enter_func_with_args("key={},ver=0x{:x}", key, ver);
            if (ver != CURRENT_VERSION)
            {
                return mutils::bytes_size(persistent_core.get(ver)->kv_map.at(key));
            }
            derecho::Replicated<WANPersistentCascadeStore> &subgroup_handle = group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_get_size)>(key);
            auto &replies = results.get();
            // TODO: verify consistency ?
            // for (auto& reply_pair : replies) {
            //     ret = reply_pair.second.get();
            // }
            debug_leave_func();
            return replies.begin()->second.get();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        uint64_t WANPersistentCascadeStore<KT, VT, IK, IV, ST>::get_size_by_time(const KT &key, const uint64_t &ts_us)
        {
            debug_enter_func_with_args("key={},ts_us={}", key, ts_us);
            const HLC hlc(ts_us, 0ull);
            try
            {
                debug_leave_func();
                return mutils::bytes_size(persistent_core.get(hlc)->kv_map.at(key));
            }
            catch (const int64_t &ex)
            {
                dbg_default_warn("temporal query throws exception:0x{:x}. key={}, ts={}", ex, key, ts_us);
            }
            catch (...)
            {
                dbg_default_warn("temporal query throws unknown exception. key={}, ts={}", key, ts_us);
            }
            debug_leave_func();
            return 0;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::vector<KT> WANPersistentCascadeStore<KT, VT, IK, IV, ST>::list_keys(const persistent::version_t &ver)
        {
            debug_enter_func_with_args("ver=0x{:x}.", ver);
            if (ver != CURRENT_VERSION)
            {
                std::vector<KT> key_list;
                auto kv_map = persistent_core.get(ver)->kv_map;
                for (auto &kv : kv_map)
                {
                    key_list.push_back(kv.first);
                }
                debug_leave_func();
                return key_list;
            }
            derecho::Replicated<WANPersistentCascadeStore> &subgroup_handle = group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index);
            auto results = subgroup_handle.template ordered_send<RPC_NAME(ordered_list_keys)>();
            auto &replies = results.get();
            // TODO: verify consistency ?
            debug_leave_func();
            return replies.begin()->second.get();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::vector<KT> WANPersistentCascadeStore<KT, VT, IK, IV, ST>::list_keys_by_time(const uint64_t &ts_us)
        {
            debug_enter_func_with_args("ts_us={}", ts_us);
            const HLC hlc(ts_us, 0ull);
            try
            {
                auto kv_map = persistent_core.get(hlc)->kv_map;
                std::vector<KT> key_list;
                for (auto &kv : kv_map)
                {
                    key_list.push_back(kv.first);
                }
                debug_leave_func();
                return key_list;
            }
            catch (const int64_t &ex)
            {
                dbg_default_warn("temporal query throws exception:0x{:x]. ts={}", ex, ts_us);
            }
            catch (...)
            {
                dbg_default_warn("temporal query throws unknown exception. ts={}", ts_us);
            }
            debug_leave_func();
            return {};
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::tuple<persistent::version_t, uint64_t> WANPersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_put(const VT &value)
        {
            debug_enter_func_with_args("key={}", value.key);

            std::tuple<persistent::version_t, uint64_t> version = group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index).get_next_version();
            value.ver = version;
            this->persistent_core->ordered_put(value);
            if (cascade_watcher_ptr)
            {
                (*cascade_watcher_ptr)(
                    group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index).get_subgroup_id(),
                    group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index).get_shard_num(),
                    value.key, value, nullptr /*cascade context*/);
            }

            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(version), std::get<1>(version));

            return version;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::tuple<persistent::version_t, uint64_t> WANPersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_remove(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);

            std::tuple<persistent::version_t, uint64_t> version = group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index).get_next_version();
            if (this->persistent_core->ordered_remove(key))
            {
                if (cascade_watcher_ptr)
                {
                    (*cascade_watcher_ptr)(
                        group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index).get_subgroup_id(),
                        group->template get_subgroup<WANPersistentCascadeStore>(this->subgroup_index).get_shard_num(),
                        key, *IV, nullptr /*cascade context*/);
                }
            }

            debug_leave_func_with_value("version=0x{:x},timestamp={}", std::get<0>(version), std::get<1>(version));

            return version;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        const VT WANPersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_get(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);

            debug_leave_func();

            return this->persistent_core->ordered_get(key);
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        uint64_t WANPersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_get_size(const KT &key)
        {
            debug_enter_func_with_args("key={}", key);

            debug_leave_func();

            return this->persistent_core->ordered_get_size(key);
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::vector<KT> WANPersistentCascadeStore<KT, VT, IK, IV, ST>::ordered_list_keys()
        {
            debug_enter_func();

            debug_leave_func();

            return this->persistent_core->ordered_list_keys();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        std::unique_ptr<WANPersistentCascadeStore<KT, VT, IK, IV, ST>> WANPersistentCascadeStore<KT, VT, IK, IV, ST>::from_bytes(mutils::DeserializationManager *dsm, char const *buf)
        {
            auto persistent_core_ptr = mutils::from_bytes<persistent::Persistent<DeltaCascadeStoreCore<KT, VT, IK, IV>, ST>>(dsm, buf);
            auto wan_persistent_cascade_store_ptr =
                std::make_unique<WANPersistentCascadeStore>(std::move(*persistent_core_ptr), &(dsm->mgr<CascadeWatcher<KT, VT, IK, IV>>()));
            return wan_persistent_cascade_store_ptr;
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        void WANPersistentCascadeStore<KT, VT, IK, IV, ST>::init_wan_config()
        {
            ///////////////////////////////////////////////////////////////////////////////
            // TODO: determine how to register Predicate to cascade.
            ///////////////////////////////////////////////////////////////////////////////

            // table: key is site_id, value is seq_no
            pl = [](const std::map<uint32_t, uint64_t> &table) {
                for (auto &item : table)
                {
                    std::cout << "site " << item.first << " ack for seq_no " << item.second << std::endl;
                }
            };

            wan_agent_sender = std::make_unique<wan_agent::WanAgentSender>(wan_conf_json, pl);
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        WANPersistentCascadeStore<KT, VT, IK, IV, ST>::WANPersistentCascadeStore(
            persistent::PersistentRegistry *pr,
            CascadeWatcher<KT, VT, IK, IV> *cw)
            : persistent_core([]() {
                  return std::make_unique<DeltaCascadeStoreCore<KT, VT, IK, IV>>();
              },
                              nullptr, pr),
              cascade_watcher_ptr(cw), wan_conf_json(nlohmann::json::parse(derecho::getConfString(CONF_WAN_SENDER_CFG)))
        {
            std::cout << derecho::getConfString(CONF_WAN_SENDER_CFG) << std::endl;
            init_wan_config();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        WANPersistentCascadeStore<KT, VT, IK, IV, ST>::WANPersistentCascadeStore(
            persistent::Persistent<DeltaCascadeStoreCore<KT, VT, IK, IV>, ST> &&
                _persistent_core,
            CascadeWatcher<KT, VT, IK, IV> *cw) // move persistent_core
            : persistent_core(std::move(_persistent_core)),
              cascade_watcher_ptr(cw), wan_conf_json(nlohmann::json::parse(derecho::getConfString(CONF_WAN_SENDER_CFG)))
        {
            init_wan_config();
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        WANPersistentCascadeStore<KT, VT, IK, IV, ST>::WANPersistentCascadeStore(
            WANPersistentCascadeStore<KT, VT, IK, IV, ST> &&_wan_persistent_cascade_store) // move wan_persistent_cascade_store, maybe useless
            : persistent_core(std::move(_wan_persistent_cascade_store.persistent_core)),
              cascade_watcher_ptr(std::move(_wan_persistent_cascade_store.cascade_watcher_ptr.get())), wan_agent_sender(std::move(_wan_persistent_cascade_store.wan_agent_sender.get()))
        {
        }

        template <typename KT, typename VT, KT *IK, VT *IV, persistent::StorageType ST>
        WANPersistentCascadeStore<KT, VT, IK, IV, ST>::~WANPersistentCascadeStore() {}

    } //namespace cascade
} //namespace derecho
