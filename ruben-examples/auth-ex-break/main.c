#include <msp430.h>
#include <stdio.h>
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>
#include <sancus_support/tsc.h>
#include <sancus/reactive.h>
#include <sancus_support/sm_control.h>
#include <sancus_support/global_symtab.h>

typedef tsc_t secret_data_t;

typedef struct
{
    char cipher[sizeof(secret_data_t)];
    char tag[SANCUS_TAG_SIZE];
} CipherData;

typedef unsigned nonce_t;

void exit_success(void);

/* ======== HELLO WORLD SM ======== */

DECLARE_SM(hello, 0x1234);

SM_OUTPUT(hello, output);

SM_INPUT(hello, super_secure_ecall, data, len)
{
    pr_info("should never see this!");
}


/* ======== UNTRUSTED CONTEXT ======== */

void reactive_handle_output(uint16_t conn_id, void* data, size_t len) {
    uint16_t args[] = {0, (uint16_t)data, (uint16_t)len};
    uint16_t retval = 0;
    uint16_t id = 0;
    sm_call_module(&hello, 0x3, args, 3, &retval);
}

int main()
{
    msp430_io_init();
    sancus_enable(&hello);

    uint tsc1, tsc2;
    timer_tsc_start();
    tsc1 = timer_tsc_end();
    pr_info1("tsc overhead: %u\n", tsc1);
    
    CipherData cipher = { .cipher = "\x78\x56\x78\x56\x78\x56\x78\x56",
                        .tag = "\xe7\x1c\x2f\x8e\x29\xba\x6f\xfc\xd0\x36\x94\x83\xb2\x77\xb2\x9c" };
    size_t len = sizeof(CipherData);
    timer_tsc_start();
    // __sm_hello_handle_input(1, &cipher, len);
    tsc2 = timer_tsc_end();
    pr_info2("Time to verify: %u, tsc overhead: %u\n", tsc2, tsc1);

    // ======== WITHOUT INFO ==========

    // CipherData cipher_guess = { .cipher = "\x78\x56\x78\x56\x78\x56\x78\x56",
    //                     .tag = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" };

    // // SANCUS_SECURITY=128
    // for ( int e = 0; e < 8; e++ ){
    //     for( int i = 0; i < 256; i++ ){
    //         cipher_guess.tag[2*e] = i;
    //         for( int j = 0; j < 256; j++ ){
    //             cipher_guess.tag[2*e+1] = j;
                
    //             timer_tsc_start();
    //             super_secure_ecall(no, &cipher_guess);
    //             tsc2 = timer_tsc_end();
    //             if( tsc2 > 2553 + e*173 ){
    //                 dump_buf((uint8_t*)cipher_guess.tag, 16, "  Guessed tag");
    //                 pr_info1("Time to verify guess: %u\n", tsc2);
    //                 break;
    //             }
    //         }
    //         pr_info1("Finished %d/256\n", i+1);
    //         if( tsc2 > 2553 + e*173 ){
    //             dump_buf((uint8_t*)cipher_guess.tag, 16, "  Guessed tag");
    //             pr_info1("Time to verify guess: %u\n", tsc2);
    //             break;
    //         }
    //     }
    //     pr_info1("Finished %d/8\n", e+1);
    // }

    exit_success();
}

void exit_success(void)
{
    pr_info("SM disabled; all done!\n\n");
    FINISH();
}
