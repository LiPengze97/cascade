for path in `find /root/lcpan/cascade/build/src/wan_agent/big_shard_cfg -regex ".*\.plog"`
do
    rm -rf $path
done

for path in `find /root/lcpan/cascade/build/src/wan_agent/big_shard_cfg -regex ".*\.log"`
do
    rm -rf $path
done