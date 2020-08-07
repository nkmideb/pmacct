#!/usr/bin/env bash

#    Copyright
#    (c) 2019-2020 Claudio Ortega <claudio.alberto.ortega@gmail.com>
#    (c) 2019-2020 Paolo Lucente <paolo@pmacct.net>
#
#    Permission to use, copy, modify, and distribute this software for any
#    purpose with or without fee is hereby granted, provided that the above
#    copyright notice and this permission notice appear in all copies.
#
#    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

VARIANT_SPEC=${1}
BUILD_BRANCH=${2}

declare -a CONFIG_FLAGS

if [[ ( ${VARIANT_SPEC} == "single" ) ]]; then
  CONFIG_FLAGS[0]="--enable-debug --enable-mysql --enable-pgsql --enable-sqlite3 --enable-kafka --enable-geoipv2 --enable-jansson --enable-rabbitmq --enable-ndpi --enable-zmq --enable-avro --enable-serdes --enable-redis"
elif [[ ( ${VARIANT_SPEC} == "all" ) ]]; then
  CONFIG_FLAGS[0]="--enable-debug --enable-mysql --enable-pgsql --enable-sqlite3 --enable-kafka --enable-geoipv2 --enable-jansson --enable-rabbitmq --enable-ndpi --enable-zmq --enable-avro --enable-serdes --enable-redis"
  CONFIG_FLAGS[1]="--enable-mysql --enable-pgsql --enable-sqlite3 --enable-kafka --enable-geoipv2 --enable-jansson --enable-rabbitmq --enable-ndpi --enable-zmq --enable-avro --enable-serdes --enable-redis"
  CONFIG_FLAGS[2]="--enable-debug --enable-jansson --enable-zmq --enable-kafka"
  CONFIG_FLAGS[3]="--enable-jansson --enable-zmq --enable-kafka"
  CONFIG_FLAGS[4]="--enable-debug --enable-jansson --enable-zmq"
  CONFIG_FLAGS[5]="--enable-jansson --enable-zmq"
  CONFIG_FLAGS[6]="--enable-debug --enable-jansson --enable-kafka --enable-avro --enable-serdes"
  CONFIG_FLAGS[7]="--enable-jansson --enable-kafka --enable-avro --enable-serdes"
  CONFIG_FLAGS[8]="--enable-debug --enable-jansson --enable-kafka"
  CONFIG_FLAGS[9]="--enable-jansson --enable-kafka"
  CONFIG_FLAGS[10]="--enable-debug --enable-zmq"
  CONFIG_FLAGS[11]="--enable-zmq"
  CONFIG_FLAGS[12]="--enable-debug --enable-jansson"
  CONFIG_FLAGS[13]="--enable-jansson"
  CONFIG_FLAGS[14]="--enable-debug"
  CONFIG_FLAGS[15]=""
else
  echo "unsupported variant spec: ["${VARIANT_SPEC}"]"
  exit 1
fi

export AVRO_LIBS="-L/usr/local/avro/lib -lavro"
export AVRO_CFLAGS="-I/usr/local/avro/include"
export LD_LIBRARY_PATH=/usr/local/lib

for config_loop_var in "${CONFIG_FLAGS[@]}"; do

    echo BEGIN-1 --- build pmacct based on CONFIG_FLAGS := ${config_loop_var}

    rm -rf pmacct
    git clone --single-branch --branch ${BUILD_BRANCH} https://github.com/pmacct/pmacct.git

    cd pmacct
    rm -rf ./.git
    ./autogen.sh
    ./configure ${config_loop_var}

    make
    sudo make install

    cd ..

    echo END-1   --- build pmacct based on CONFIG_FLAGS := ${config_loop_var}
    echo "pwd(1):"$(pwd)

    ls -l pmacct/src/nfacctd
    pmacct/src/nfacctd -V

done
