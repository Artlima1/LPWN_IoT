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

static int number;
static struct ctimer timer;

static void send_msg(void * ptr);

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
  int msg_len = packetbuf_datalen();
  char * data = (char*) packetbuf_dataptr();
  char * msg = malloc((msg_len+1));
  memcpy(msg, data, msg_len);
  msg[msg_len] = '\0';

  printf("Recv from %02X:%02X Message: '%s'\n",
        from->u8[0], from->u8[1], msg);

  number++;
  ctimer_set(&timer, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 3), send_msg, NULL);
}

static const struct broadcast_callbacks broadcast_cb = {
  .recv = broadcast_recv,
  .sent = NULL
};

static struct broadcast_conn broadcast;

static void send_msg(void * ptr){
  char msg[20];
  sprintf(msg, "%d", number);
  packetbuf_clear();
  packetbuf_copyfrom(msg, strlen(msg));
  broadcast_send(&broadcast);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 0, &broadcast_cb);

  /* Print node link layer address */
  printf("Node Link Layer Address: %02X:%02X\n",
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

  ctimer_set(&timer, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 3), send_msg, NULL);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
