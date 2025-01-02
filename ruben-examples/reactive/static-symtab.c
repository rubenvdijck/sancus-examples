#include <sancus_support/private/symbol.h>
#include <sancus/sm_support.h>

#include <stdlib.h>
#include <stdio.h>

#include "enclave_utils.h"
#include "led_insecure.h"

STATIC_SYMBOLS_START
    SYMBOL(exit),
    SYMBOL(malloc),
    SYMBOL(free),
    SYMBOL(putchar),
    SYMBOL(puts),
    SYMBOL(printf),
    SYMBOL(__unprotected_entry),
    SYMBOL(reactive_handle_output),
    SYMBOL(led_insecure_toggle)
STATIC_SYMBOLS_END
