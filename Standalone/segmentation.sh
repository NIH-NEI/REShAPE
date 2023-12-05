#!/bin/sh
exe_name=$0
exe_dir=`dirname "$0"`
export LD_LIBRARY_PATH=.:/opt/MatlabRT/v97/runtime/glnxa64:/opt/MatlabRT/v97/bin/glnxa64:/opt/MatlabRT/v97/sys/os/glnxa64:/opt/MatlabRT/v97/extern/bin/glnxa64:${LD_LIBRARY_PATH}
args=
while [ $# -gt 0 ]; do
    token=$1
    args="${args} "${token}"" 
    shift
done
eval ""${exe_dir}/segmfunc"" $args
