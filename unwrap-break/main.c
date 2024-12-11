#include <msp430.h>
#include <stdio.h>
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>
#include <sancus_support/tsc.h>

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
DECLARE_SM(attacker, 0x1234);

void SM_ENTRY(hello) hello_init(void)
{
    pr_info("Hello from second SM");
}

int SM_ENTRY(hello) super_secure_ecall(nonce_t ad, CipherData* cipher)
{
    tsc_t cmd;
    pr_info("should never see this!");
    if (sancus_unwrap(&ad, sizeof(ad), &cipher->cipher, sizeof(&cipher->cipher), &cipher->tag, &cmd))
    {
        if (cmd !=0)
        {
            pr_info("should never see this!");
        }
    }
    return 0;
}

void SM_ENTRY(attacker) attacker_init(void)
{
    pr_info("Hello from attacker SM");
}

void SM_ENTRY(attacker) attacker_guess_tag(nonce_t no, CipherData* cipher)
{
    pr_info("Testing ...\n\n");
    ASSERT(sancus_is_outside_sm(attacker, cipher, sizeof(CipherData)));

    ASSERT(sancus_get_caller_id() == SM_ID_UNPROTECTED);
    ASSERT(sancus_get_self_id() == 1);
    ASSERT(sancus_get_id((void*) super_secure_ecall) == 2);

    uint tsc1, tsc2;
    timer_tsc_start();
    tsc1 = timer_tsc_end();
    pr_info1("tsc overhead: %u\n", tsc1);


    timer_tsc_start();
    super_secure_ecall(no, cipher);
    tsc2 = timer_tsc_end();
    pr_info2("Time to verify if valid: %u, tsc overhead: %u\n", tsc2, tsc1);

}

/* ======== UNTRUSTED CONTEXT ======== */

int main()
{
    msp430_io_init();

    sancus_enable(&hello);
    pr_sm_info(&hello);
    sancus_enable(&attacker);
    pr_sm_info(&attacker);

    nonce_t no = 0xabcd;
    CipherData cipher = { .cipher = "\x80\x2c\x80\xae\x26\x9f\x5f\x8b", 
                        .tag = "\xe7\x1c\x2f\x8e\x29\xba\x6f\xfc\xd0\x36\x94\x83\xb2\x77\xb2\x9c" };
    
    pr_info("calling attacker SM\n");
    attacker_guess_tag(no, &cipher);

}

void exit_success(void)
{
    // TODO unprotect instruction should also clear caller ID
    //ASSERT(!sancus_get_caller_id());

    pr_info("SM disabled; all done!\n\n");
    FINISH();
}