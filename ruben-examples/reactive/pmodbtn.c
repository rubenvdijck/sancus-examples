#include "pmodbtn.h"
#include <msp430.h>

#define DRIVER_ID 1 + 2 * USE_MMIO_LED

#if USE_MMIO_BTN
  DECLARE_EXCLUSIVE_MMIO_SM(pmodbtn,
                            /*[secret_start, end[=*/ P1IN_, P2IN_,
                            /*caller_id=*/ DRIVER_ID,
                            /*vendor_id=*/ 0x1234);

  uint8_t SM_MMIO_ENTRY(pmodbtn) pmodbtn_poll(void)
  {
      asm(
      "mov.b &%0, r15                                                     \n\t"
      ::"m"(P1IN)
      );
  }

  void SM_MMIO_ENTRY(pmodbtn) pmodbtn_init(void)
  {
      /*
       * Our setup: BTN0123 mapped to P3IN[4:7]
       *
       * https://store.digilentinc.com/pmodbtn-4-user-pushbuttons/
       * http://processors.wiki.ti.com/index.php/Digital_I/O_(MSP430)
       */

      asm(
      "mov.b #0x00, &%0                                                   \n\t"
      "mov.b #0x00, &%1                                                   \n\t"
      "bis.b #0x08, &%2                                                   \n\t"
      "bis.b #0x08, &%3                                                   \n\t"
      "and.b #0xf7, &%4                                                   \n\t"
      ::"m"(P1SEL), "m"(P1DIR), "m"(P1IES), "m"(P1IE), "m"(P1IFG):
      );
  }

#endif
