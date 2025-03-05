#include <msp430.h>
#include <stdio.h>
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>

DECLARE_SM(foo, 0x1234);

int enter_foo(int arg);

int main()
{
    msp430_io_init();

    pr_info("enabling foo SM..");
    uint tsc1, tsc2;
    timer_tsc_start();
    tsc1 = timer_tsc_end();
    pr_info1("tsc overhead: %u\n", tsc1);
    
    
    
    // timer_tsc_start();
    // dump_buf((uint8_t*)SM_GET_WRAP_TAG(test), 16, "  Tag");
    // sancus_enable_wrapped(&hello, SM_GET_WRAP_NONCE(hello), SM_GET_WRAP_TAG(test));
    // tsc2 = timer_tsc_end();
    // pr_info2("wrong nonce Time to enable: %u, tsc overhead: %u\n", tsc2, tsc1);
    
    unsigned no = SM_GET_WRAP_NONCE(foo);
    char* tag = SM_GET_WRAP_TAG(foo);
    char guess_tag[SANCUS_TAG_SIZE] = {0};
    pr_info1("Nonce: %u\n", no);
    dump_buf((uint8_t*)tag, 16, "  Tag");
    guess_tag[0] = tag[0];
    guess_tag[1] = tag[1];
    guess_tag[2] = tag[2];
    guess_tag[3] = tag[3];
    
    timer_tsc_start();
    sancus_enable_wrapped(&foo, no, guess_tag);
    tsc2 = timer_tsc_end();
    pr_info3("Time to verify if only %d/8 bytes correct: %u, tsc overhead: %u\n", 4, tsc2, tsc1);
    
    timer_tsc_start();
    sancus_enable_wrapped(&foo, no, tag);
    tsc2 = timer_tsc_end();
    pr_info3("Time to verify if only %d/8 bytes correct: %u, tsc overhead: %u\n", 8, tsc2, tsc1);

    pr_info("entering foo..");
    int null = enter_foo(0x6c0a);
    printf("foo(0)=%#x\n", null);

    pr_info("all done!");
    EXIT();
}

