#!/bin/bash
DICT_DIR=$HOME/.dict
INSTALL_DIR=$HOME/.local/
ALL="dictconf.py dict _dict.py"


if ! [ -d $DICT_DIR ]; then
    mkdir -p $DICT_DIR
fi

INSTALL_BIN=${INSTALL_DIR}bin
INSTALL_LIB=${INSTALL_DIR}lib/dict

if ! [ -d $INSTALL_LIB ]; then
    mkdir -p $INSTALL_LIB
fi

cp $ALL $INSTALL_LIB
ln -s ${INSTALL_LIB}/dict ${INSTALL_BIN}/dict
