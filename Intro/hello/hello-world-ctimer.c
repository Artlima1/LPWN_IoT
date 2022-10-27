#include "contiki.h"
#include "leds.h"
#include "node-id.h"
#include <stdio.h> /* For printf() */

#define PERIOD_ON (CLOCK_SECOND/10)
#define PERIOD_OFF (CLOCK_SECOND/10)*9

static struct ctimer timer;  // timer object

/*---------------------------------------------------------------------------*/

static int status;
static void led_cb(void * ptr){
  if(status){
    printf("Turning LEDs off\n");
    leds_toggle(LEDS_RED|LEDS_GREEN);
    status = 0;
    ctimer_set(&timer, PERIOD_OFF, led_cb, NULL);
  }
  else{
    printf("Turning LEDs on\n");
    leds_toggle(LEDS_RED|LEDS_GREEN);
    status = 1;
    ctimer_set(&timer, PERIOD_ON, led_cb, NULL);
  }
}

/*---------------------------------------------------------------------------*/
PROCESS(led_process, "LED process");
AUTOSTART_PROCESSES(&led_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(led_process, ev, data)
{
  PROCESS_BEGIN();
  
  ctimer_set(&timer, PERIOD_OFF, led_cb, NULL);
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
