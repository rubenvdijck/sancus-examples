#if USE_MMIO_BTN
  #include "button_driver.h"

  #include <sancus/reactive.h>
  #include <stdio.h>
  #include <sancus/sm_support.h>
  #include <sancus_support/sm_io.h>
  #include "pmodbtn.h"

  DECLARE_SM(button_driver, 0x1234);

  static unsigned int SM_DATA(button_driver) initialized = 0;

  SM_OUTPUT(button_driver, button_pressed);

  SM_INPUT(button_driver, trigger_button_press, data, len) {
    (void) data;
    (void) len;

    puts("[button_driver] Button has been pressed, sending output");
    button_pressed(NULL, 0);
  }

  SM_INPUT(button_driver, poll_device, data, len) {
    unsigned int driver_state = 0x00;
    (void) data;
    (void) len;

    puts("[button_driver] polling device");

    if(!initialized) {
      pmodbtn_init();
      initialized = 1;
    }

    driver_state = pmodbtn_poll();
    pr_info1("driver says 0x%x\n", driver_state);
  }

  SM_ISR(button_driver, num)
  {
      if (num != 2)
      {
          puts("Wrong IRQ");
          return;
      }

      puts("Interrupt!");
  }

  SM_HANDLE_IRQ(button_driver, 2);
#endif
