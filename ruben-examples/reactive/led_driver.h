#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__

#if USE_MMIO_LED
  #include <sancus/sm_support.h>

  extern struct SancusModule led_driver;
#endif

#endif
