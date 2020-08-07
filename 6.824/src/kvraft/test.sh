
function test_one {
    go test -run TestSnapshotUnreliableRecoverConcurrentPartitionLinearizable3B >test_res_$1
    rc=$?
    if [ $rc -ne 0 ]; then
        echo "test $1 failed.";
    else
        rm test_res_$1;
    fi
}

export -f test_one

for x in $(seq 1 1000); do echo "\"test_one $x\"" ; done | xargs -n 1 -P 10 bash -c

for f in `ls test_res*`; do tail -n 1 $f | awk '{print $1}'; done | sort | uniq
