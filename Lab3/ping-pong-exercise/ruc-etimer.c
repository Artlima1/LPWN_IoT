/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "sys/node-id.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/* Application Configuration */
#define UNICAST_CHANNEL 146
#define APP_TIMER_DELAY (CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2))
/*---------------------------------------------------------------------------*/
typedef struct ping_pong_msg {
  uint16_t number;
  int16_t noise_floor;
}
__attribute__((packed))
ping_pong_msg_t;
/*---------------------------------------------------------------------------*/
static linkaddr_t receiver = {{0xAA, 0xBB}};
/*---------------------------------------------------------------------------*/
static uint16_t ping_pong_number = 0;
static struct etimer et;
/*---------------------------------------------------------------------------*/
process_event_t app_event;
/*---------------------------------------------------------------------------*/
/* Declaration of static functions */
static void print_rf_conf(void);
static void set_ping_pong_msg(ping_pong_msg_t *m);
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from);
static void sent_unicast(struct unicast_conn *c, int status, int num_tx);
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks uc_callbacks = {
  .recv     = recv_unicast,
  .sent     = sent_unicast
};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS(unicast_process, "Unicast Process");
AUTOSTART_PROCESSES(&unicast_process);
/*---------------------------------------------------------------------------*/
static void print_rf_conf(void) {
  radio_value_t rfval = 0;

  printf("RF Configuration:\n");
  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &rfval);
  printf("\tRF Channel: %d\n", rfval);
  NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &rfval);
  printf("\tTX Power: %ddBm\n", rfval);
  NETSTACK_RADIO.get_value(RADIO_PARAM_CCA_THRESHOLD, &rfval);
  printf("\tCCA Threshold: %d\n", rfval);
}
/*---------------------------------------------------------------------------*/
static void set_ping_pong_msg(ping_pong_msg_t *m) {
  radio_value_t noise_floor;
  NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &noise_floor);
  m->number = ping_pong_number;
  m->noise_floor = (int) noise_floor;
}
/*---------------------------------------------------------------------------*/
static void app_send_msg(void) {
  ping_pong_msg_t msg; /* Message struct to be sent in unicast */

  set_ping_pong_msg(&msg);
  packetbuf_clear();
  packetbuf_copyfrom(&msg, sizeof(ping_pong_msg_t));
  unicast_send(&uc, &receiver);
}
/*---------------------------------------------------------------------------*/
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from) {
  /* Local variables */
  ping_pong_msg_t msg;
  radio_value_t rssi;

  ping_pong_msg_t * data = (ping_pong_msg_t*) packetbuf_dataptr();
  memcpy(&msg, data, sizeof(ping_pong_msg_t));

  NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &rssi);

  printf("Recv num %d with noise floor %d and rssi %d\n", msg.number, msg.noise_floor, rssi);

  ping_pong_number = msg.number + 1;

  // 4. Post a process event (app_event) to continue the ping pong process
  process_post(PROCESS_BROADCAST, app_event, NULL);
}
/*---------------------------------------------------------------------------*/
static void sent_unicast(struct unicast_conn *c, int status, int num_tx) {
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  /* TO DO 5 [OPTIONAL]:
   * Print some info about the packet sent, e.g., ping-pong number, num_tx, etc.
   */
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_process, ev, data)
{
  PROCESS_BEGIN();

#if CONTIKI_TARGET_SKY
  /* Cooja Setup for Lab 3:
   * Set the right destination address for each node in the network. 
   * In hardware, simply set the receiver address (see above).
   */
  if(node_id == 1) {
    receiver.u8[0] = 0x02;
    receiver.u8[1] = 0x00;
  } else if(node_id == 2) {
    receiver.u8[0] = 0x01;
    receiver.u8[1] = 0x00;
  }
#endif

  /* Print RF configuration */
  print_rf_conf();

  /* Print node link layer address */
  printf("Node Link Layer Address: %02X:%02X\n",
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

  /* Print receiver link layer address */
  printf("Node: receiver address %02X:%02X\n",
    receiver.u8[0], receiver.u8[1]);

  /* Print size of struct ping_pong_msg_t */
  printf("Sizeof struct: %u\n", sizeof(ping_pong_msg_t));

  unicast_open(&uc, UNICAST_CHANNEL, &uc_callbacks);

  etimer_set(&et, APP_TIMER_DELAY);

  app_event = process_alloc_event();
  

  while(1) {
    /* Wait for an event */
    PROCESS_WAIT_EVENT();

    /* TO DO:
     * If the etimer expires, send a message to the receiver.
     * Instead, it the process event is an app_event set the etimer.
     */
    if(ev==PROCESS_EVENT_TIMER && etimer_expired(&et)){
      app_send_msg();
    }
    else if (ev==app_event)
    {
      etimer_set(&et, APP_TIMER_DELAY);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
