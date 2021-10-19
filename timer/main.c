#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>


volatile char c = '0';
volatile int timer_latency;


DECLARE_SM(bar, 0x1234);

void SM_ENTRY(bar) bar_enter()
{
    while (c == '0');
    pr_info("Hello from bar");
}

DECLARE_SM(foo, 0x1234);

void SM_ENTRY(foo) foo_exit(void)
{
    /* NOTE: only SM 1 can exit on Aion */
    FINISH();
}


/*
 * Untrusted context
 */


void timerA_isr(void)
{
    timer_disable();
    pr_info1("Hi from Timer_A ISR!\n",
                timer_latency);
    if (c == '0')
        c = '1';
    else
        c = '0';
}

int main()
{
    int tsc1, tsc2;
    msp430_io_init();
    asm("eint\n\t");
    
    sancus_enable(&foo);
    sancus_enable(&bar);

    /* First measure TSC function overhead */
    timer_tsc_start();
    tsc1 = timer_tsc_end();

    /* Now measure the operation of interest */
    timer_tsc_start();
    asm("nop\n\t"
        "nop\n\t");
    tsc2 = timer_tsc_end();
    pr_info1("dummy operation took %d cycles\n", tsc2-tsc1);
    
    pr_info("waiting for bar...");
    timer_irq(150);
    bar_enter();
    asm("eint\n\t");
    pr_info("waiting for unprotected code...");
    timer_irq(50);
    while (c == '1');
    timer_disable();

    foo_exit();
    ASSERT(0 && "should never reach here");
}

/* ======== TIMER A ISR ======== */
TIMER_ISR_ENTRY(timerA_isr)

