#!/bin/bash

# Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# source shflags from current directory
#mydir="${BASH_SOURCE%/*}"
#if [[ ! -d "$mydir" ]]; then mydir="$PWD"; fi
#. $mydir/../shflags
source ./shflags

# define command-line flags
DEFINE_boolean clean 1 'Remove old "runtime" dir before running'
DEFINE_integer bthread_concurrency '8' 'Number of worker pthreads'
DEFINE_integer server_port 8100 "Port of the first server"
DEFINE_integer server_num  3 "NUMBER OF SERVER NUMBER"
DEFINE_integer thread_num 1 'Number of sending thread'
DEFINE_string crash_on_fatal 'true' 'Crash on fatal log'
DEFINE_string valgrind 'false' 'Run in valgrind'
DEFINE_string use_bthread "true" "Use bthread to send request"

DEFINE_string cmd "0" 'echo start script but not run'

FLAGS "$@" || exit 1

# hostname prefers ipv6
IP=`hostname -i | awk '{print $NF}'`


raft_peers=""
for ((i=0; i<$FLAGS_server_num; ++i)); do
    raft_peers="${raft_peers}${IP}:$((${FLAGS_server_port}+i)):0,"
done

echo "raft peers:" $raft_peers

export TCMALLOC_SAMPLE_PARAMETER=524288

if [ "$FLAGS_cmd" == "0" ]; then
        ./dlevel_client \
                --bthread_concurrency=${FLAGS_bthread_concurrency} \
                --conf="${raft_peers}" \
                --crash_on_fatal_log=${FLAGS_crash_on_fatal} \
                --thread_num=${FLAGS_thread_num} \
                --use_bthread=${FLAGS_use_bthread};
else
echo "./dlevel_client \
--bthread_concurrency=${FLAGS_bthread_concurrency} \
--conf="${raft_peers}" \
--crash_on_fatal_log=${FLAGS_crash_on_fatal} \
--thread_num=${FLAGS_thread_num} \
--use_bthread=${FLAGS_use_bthread}";
fi