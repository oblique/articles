#!/bin/sh

if [ `uname -m` = x86_64 ]; then
    if [ -e /usr/include/asm/unistd_32.h ]; then
        sed -e 's/__NR_/__NR32_/g' /usr/include/asm/unistd_32.h > unistd_32.h
    else
        if [ -e /usr/include/asm-i386/unistd.h ]; then
            sed -e 's/__NR_/__NR32_/g' /usr/include/asm-i386/unistd.h > unistd_32.h
        else
            echo "asm/unistd_32.h and asm-386/unistd.h does not exist."
        fi
    fi
fi
