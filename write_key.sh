#!/bin/sh

KEYFILE="my_enc_key.bin"
UARTPORT="/dev/cu.SLAB_USBtoUART"

if [ -e ${KEYFILE} ]; then
    echo "nothing to do"
else
    espsecure.py generate_flash_encryption_key ${KEYFILE}
    espefuse.py --port ${UARTPORT} burn_key flash_encryption ${KEYFILE}
fi

exit 0