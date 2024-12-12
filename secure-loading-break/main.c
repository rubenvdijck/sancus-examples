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

/* ======== Test Module ========= */

DECLARE_SM(test, 0x1234);

void SM_ENTRY(test) test_greet(void)
{
    ASSERT(sancus_get_caller_id() == SM_ID_UNPROTECTED);
    ASSERT(sancus_get_self_id() == 1);
    pr_info2("Hi from SM with ID %d, called by %d\n",
        sancus_get_self_id(), sancus_get_caller_id());
}

void SM_ENTRY(test) test_disable(void)
{
    ASSERT(sancus_get_caller_id() == SM_ID_UNPROTECTED);
    ASSERT(sancus_get_self_id() == 1);
    sancus_disable(exit_success);
}

/* ======== UNTRUSTED CONTEXT ======== */

int main()
{
    msp430_io_init();

    uint tsc1, tsc2;
    timer_tsc_start();
    tsc1 = timer_tsc_end();

    char* guessed_tag = malloc(18);
    guessed_tag = SM_GET_WRAP_TAG(hello);
    dump_buf((uint8_t*)guessed_tag, 16, "  Hello tag");

    timer_tsc_start();
    sancus_enable_wrapped(&hello, SM_GET_WRAP_NONCE(hello), guessed_tag);
    tsc2 = timer_tsc_end();
    pr_info2("Time to verify if valid: %u, tsc overhead: %u\n", tsc2, tsc1);

    char* correct_tag = malloc(18);
    correct_tag = SM_GET_WRAP_TAG(test);    
    dump_buf((uint8_t*)correct_tag, 16, "  Correct tag");


    for( int i = 0; i <= 8; i++ ){
        guessed_tag[2*i] = correct_tag[2*i];
        guessed_tag[2*i+1] = correct_tag[2*i+1];
        
        dump_buf((uint8_t*)guessed_tag, 16, "  Guessed tag");
        char* updated_correct_tag = malloc(18);
        updated_correct_tag = SM_GET_WRAP_TAG(test);
        dump_buf((uint8_t*)updated_correct_tag, 16, "  Updated? correct tag");
        
        timer_tsc_start();
        sancus_enable_wrapped(&test, SM_GET_WRAP_NONCE(test), guessed_tag); // Wrong tag
        tsc2 = timer_tsc_end();
        pr_info2("Time to verify if wrong tag: %u, tsc overhead: %u\n", tsc2, tsc1);
    }

    timer_tsc_start();
    sancus_enable_wrapped(&test, SM_GET_WRAP_NONCE(test), correct_tag); 
    tsc2 = timer_tsc_end();
    pr_info2("Time to verify if correct tag: %u, tsc overhead: %u\n", tsc2, tsc1);

    pr_sm_info(&hello);
    exit_success();
}

void exit_success(void)
{
    // TODO unprotect instruction should also clear caller ID
    //ASSERT(!sancus_get_caller_id());
    ASSERT(!sancus_get_id(hello_greet));

    pr_info("SM disabled; all done!\n\n");
    FINISH();
}