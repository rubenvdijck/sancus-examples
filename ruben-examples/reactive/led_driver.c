#if USE_MMIO_LED
  #include "led_driver.h"

  #include <sancus/reactive.h>
  #include <stdio.h>
  #include <sancus/sm_support.h>

  #include "pmodled.h"

  DECLARE_SM(led_driver, 0x1234);

  #define LED_ON 0x1 << 5
  #define LED_OFF 0

  static unsigned int SM_DATA(led_driver) initialized = 0;
  static unsigned int SM_DATA(led_driver) led_on = 0;

  // initialize the LED, implicitly attesting pmodled
  SM_INPUT(led_driver, init_led, data, len) {
    (void) data;
    (void) len;

    if(!initialized) {
      puts("[led_driver] initializing LED and attesting MMIO SM");
      pmodled_init();
      initialized = 1;
    }
    else {
      puts("[led_driver] LED already initialized. Nothing to do.");
    }
  }

  // This input function toggles the LED (if ON->OFF and viceversa)
  SM_INPUT(led_driver, toggle_led, data, len) {
    //puts("[led_driver] toggling led");
    (void) data;
    (void) len;

    if(!initialized) {
      puts("[led_driver] error: LED not initialized.");
      return;
    }

    if (led_on) {
      puts("[led_driver] Turning LED off");
      pmodled_actuate(LED_OFF);
      led_on = 0;
    }
    else {
      puts("[led_driver] Turning LED on");
      pmodled_actuate(LED_ON);
      led_on = 1;
    }
  }

  // This input functions turns the LED on or off according to the value received
  SM_INPUT(led_driver, update_led, data, len) {
    //puts("[led_driver] updating led");

    if (len != 2) {
      puts("[led_driver] toggle_led: wrong data");
    }

    if(!initialized) {
      puts("[led_driver] error: LED not initialized.");
      return;
    }

    unsigned int state = *(unsigned int*) data;

    if (state) {
      puts("[led_driver] Turning LED on");
      pmodled_actuate(LED_ON);
      led_on = 1;
    }
    else {
      puts("[led_driver] Turning LED off");
      pmodled_actuate(LED_OFF);
      led_on = 0;
    }
  }

#endif
