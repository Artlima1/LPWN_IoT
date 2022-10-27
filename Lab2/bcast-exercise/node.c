/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>
#include <stdlib.h>
/*---------------------------------------------------------------------------*/
PROCESS(broadcast_process, "Broadcast Process");
AUTOSTART_PROCESSES(&broadcast_process);
/*---------------------------------------------------------------------------*/
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
  int msg_len = packetbuf_datalen();
  char * data = (char*) packetbuf_dataptr();
  char * msg = malloc((msg_len+1));
  memcpy(msg, data, msg_len);
  msg[msg_len] = '\0';

  printf("Recv from %02X:%02X Message: '%s'\n",
        from->u8[0], from->u8[1], msg);
}

static void broadcast_sent(struct broadcast_conn *c, int status, int num_tx){
  printf("Sent Result: Status %d TX %d\n", 
          status, num_tx);
}

static const struct broadcast_callbacks broadcast_cb = {
  .recv = broadcast_recv,
  .sent = broadcast_sent
};

static struct broadcast_conn broadcast;

static void send_msg(char* msg){
  packetbuf_clear();
  packetbuf_copyfrom(msg, strlen(msg));
  broadcast_send(&broadcast);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);

  broadcast_open(&broadcast, 0, &broadcast_cb);

  /* Print node link layer address */
  printf("Node Link Layer Address: %02X:%02X\n",
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

  etimer_set(&et, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 3));

  while(1) {

    /* Wait for event (timer expire or button) */
    PROCESS_WAIT_EVENT();
    if(ev==PROCESS_EVENT_TIMER && etimer_expired(&et)){
      /* Delay 5-8 seconds */
      etimer_set(&et, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 3));
      char msg[] = "Hello world";
      send_msg(msg);
    }
    else if(ev == sensors_event && button_sensor.value(0)==1){
      char msg[] = "Button Pressed";
      send_msg(msg);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
