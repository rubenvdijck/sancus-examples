#include <msp430.h>
#include <stdio.h>
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>

void exit_success(void);

/* ======== HELLO WORLD SM ======== */

DECLARE_SM(hello, 0x1234);

void SM_ENTRY(hello) hello_greet(void)
{
    ASSERT(sancus_get_caller_id() == SM_ID_UNPROTECTED);
    ASSERT(sancus_get_self_id() == 1);
    pr_info2("Hi from SM with ID %d, called by %d\n",
        sancus_get_self_id(), sancus_get_caller_id());
}

void SM_ENTRY(hello) hello_disable(void)
{
    ASSERT(sancus_get_caller_id() == SM_ID_UNPROTECTED);
    ASSERT(sancus_get_self_id() == 1);
    sancus_disable(exit_success);
}

DECLARE_SM(test, 0x1234);

void SM_ENTRY(test) test_greet(void)
{
    ASSERT(sancus_get_caller_id() == SM_ID_UNPROTECTED);
    ASSERT(sancus_get_self_id() == 1);
    pr_info2("Hi from SM with ID %d, called by %d\n",
        sancus_get_self_id(), sancus_get_caller_id());
}

/* ======== UNTRUSTED CONTEXT ======== */

int main()
{
    msp430_io_init();

    uint tsc1, tsc2;
    timer_tsc_start();
    tsc1 = timer_tsc_end();
    pr_info1("tsc overhead: %u\n", tsc1);
    unsigned no = SM_GET_WRAP_NONCE(hello);
    char* tag = SM_GET_WRAP_TAG(hello);
    pr_info1("Nonce: %u\n", no);
    dump_buf((uint8_t*)tag, 16, "  Tag");



    timer_tsc_start();
    dump_buf((uint8_t*)SM_GET_WRAP_TAG(test), 16, "  Tag");
    sancus_enable_wrapped(&hello, 0, SM_GET_WRAP_TAG(test));
    tsc2 = timer_tsc_end();
    pr_info2("Time to enable: %u, tsc overhead: %u\n", tsc2, tsc1);

    timer_tsc_start();
    sancus_enable_wrapped(&hello, no, tag);
    tsc2 = timer_tsc_end();
    pr_info2("Time to enable: %u, tsc overhead: %u\n", tsc2, tsc1);

    // tsc_t data = 0x5678;
    // CipherData correct_cipher = { 0 };
    // hello_wrap(&data, no, &correct_cipher);
    // char* correct_tag = correct_cipher.tag;
    // dump_buf((uint8_t*)correct_tag, 16, "  Correct tag");
    // dump_buf((uint8_t*)correct_cipher.cipher, 8, "  Correct cipher");
    
    // CipherData cipher = { .cipher = *correct_cipher.cipher,
    //                     .tag = "\xe7\x1c\x2f\x8e\x29\xba\x6f\xfc\xd0\x36\x94\x83\xb2\x77\xb2\x9c" };

    // for( int i = 0; i < 8; i++ ){
    //     cipher.cipher[i] = correct_cipher.cipher[i];
    // }

    // dump_buf((uint8_t*)cipher.cipher, 8, "  Copy correct cipher");


    // for( int i = 0; i < 8; i++ ){
    //     cipher.tag[2*i] = correct_tag[2*i];
    //     cipher.tag[2*i+1] = correct_tag[2*i+1];
        
    //     timer_tsc_start();
    //     super_secure_ecall(no, &cipher);
    //     tsc2 = timer_tsc_end();
    //     pr_info3("Time to verify if only %d/8 bytes correct: %u, tsc overhead: %u\n", i+1, tsc2, tsc1);
    // }

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
