

#ifndef TOR_SIGNALATTACK_H
#define TOR_SIGNALATTACK_H

#define BANDWIDTH_EFFICIENT 0
#define MIN_BLANK 1
#define SIMPLE_WATERMARK 2
#define SIGNAL_ATTACK_MAX_BLANK 2000

//void signal_encode_destination(char *address, circuit_t *circ);
void signal_encode_destination(void *param);

int signal_listen_and_decode(circuit_t *circ);

void signal_free(circuit_t *circ);
void signal_free_all(void);
void signal_send_delayed_destroy_cb(evutil_socket_t fd,
    short events, void *arg);

#ifdef TOR_SIGNALATTACK_PRIVATE
STATIC int signal_compare_signal_decode_(const void **a_, const void **b_);
STATIC int signal_compare_key_to_entry_(const void *_key, const void **_member);
STATIC int signal_minimize_blank_latency(char *address, circuit_t *circ);
STATIC int signal_bandwidth_efficient(char *address, circuit_t *circuit);
STATIC void subip_to_subip_bin(uint8_t, char *subip_bin);
STATIC void signal_encode_simple_watermark(circuit_t *circuit);
#endif

typedef struct signal_decode_t {
  circid_t circid;
  struct timespec first;
  smartlist_t *timespec_list;
  struct timespec last;
  int disabled;
} signal_decode_t;

typedef struct signal_encode_param_t {
  char *address;
  circuit_t *circ;
} signal_encode_param_t;

typedef struct signal_encode_state_t {
  int nb_calls;
  circuit_t *circ;
  int subip[4];
  char *address;
  struct event *ev;
} signal_encode_state_t;


void signal_encode_state_free(signal_encode_state_t *state);


#endif
