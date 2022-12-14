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
static struct ctimer ct;
/*---------------------------------------------------------------------------*/
/* Declaration of static functions */
static void print_rf_conf(void);
static void set_ping_pong_msg(ping_pong_msg_t *m);
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from);
static void sent_unicast(struct unicast_conn *c, int status, int num_tx);
/*---------------------------------------------------------------------------*/
/* Unicast callbacks to be registered when opening the unicast connection */
static const struct unicast_callbacks uc_callbacks = {
  .recv     = recv_unicast,
  .sent     = sent_unicast
};
static struct unicast_conn uc;
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
  /* Use this function to set the values of the message struct. This can
   * also be done directly in the ct_cb function. */
  radio_value_t noise_floor;
  NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &noise_floor);
  m->number = ping_pong_number;
  m->noise_floor = (int) noise_floor;
}
/*---------------------------------------------------------------------------*/
/* ctimer callback */
static void ct_cb(void *ptr) {
  ping_pong_msg_t msg; /* Message struct to be sent in unicast */

  /* TO DO 3: send the message to the receiver
   * 1. Build message to be sent using set_ping_pong_msg. This can be also
   *    done directly in this function if preferred. 
   * 2. Clear packetbuf
   * 3. Copy message to packetbuf
   * 4. Send the packet to the receiver
   */

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

  /* TO DO 4:
   * 1. Copy the received message from the packetbuf to the msg struct
   * 2. [OPTIONAL] Get the RSSI from the received packet
   * 3. Print the received ping pong number together with the RSSI
   * 4. Increase the local ping pong number
   * 4. Set the ctimer to continue the ping pong exchange
   */
  ping_pong_msg_t * data = (ping_pong_msg_t*) packetbuf_dataptr();
  memcpy(&msg, data, sizeof(ping_pong_msg_t));

  NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &rssi);

  printf("Recv num %d with noise floor %d and rssi %d\n", msg.number, msg.noise_floor, rssi);

  ping_pong_number = msg.number + 1;

  ctimer_set(&ct, APP_TIMER_DELAY, ct_cb, NULL);
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

  printf("Sent number %d\n", ping_pong_number);
}
/*---------------------------------------------------------------------------*/
PROCESS(unicast_process, "Unicast Process");
AUTOSTART_PROCESSES(&unicast_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_process, ev, data) {
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
  printf("Node Address: %02X:%02X\n",
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

  /* Print receiver link layer address */
  printf("Receiver address: %02X:%02X\n",
    receiver.u8[0], receiver.u8[1]);

  /* Print size of struct ping_pong_msg_t */
  printf("Size of struct: %u\n", sizeof(ping_pong_msg_t));

  /* TO DO 1:
   * Open the unicast connection using the defined uc_callbacks */
  unicast_open(&uc, UNICAST_CHANNEL, &uc_callbacks);

  /* TO DO 2:
   * Set the ctimer with callback ct_cb
   */
  ctimer_set(&ct, APP_TIMER_DELAY, ct_cb, NULL);

  while(1) {
    /* Do nothing */
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
