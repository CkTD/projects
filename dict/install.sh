#!/bin/bash
DICT_DIR=$HOME/.dict
INSTALL_DIR=$HOME/bin
ALL="dictconf.py dict note _dict.py"


if ! [ -d $DICT_DIR ]; then
    mkdir -p $DICT_DIR
fi

install $ALL $INSTALL_DIR
