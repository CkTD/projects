
function test_one {
    go test >test_res_$1
    rc=$?
    if [ $rc -ne 0 ]; then
        echo "test $1 failed."
    fi
}

export -f test_one


for x in $(seq 1 1000); do echo "\"test_one $x\"" ; done | xargs -n 1 -P 16 bash -c

for f in `ls test_res*`; do tail -n 1 $f | awk '{print $1}'; done | uniq
