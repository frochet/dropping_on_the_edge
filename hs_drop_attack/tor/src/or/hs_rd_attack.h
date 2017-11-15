
/**
 * \file hs_rd_attack.h
 * \brief Header file for hs_rd_attacK.c
 **/

#ifndef TOR_HS_RD_ATTACK_H
#define TOR_HS_RD_ATTACK_H

#define RETRY_THRESHOLD 100
#define HS_ATTACK_TESTING 0
#define HS_ATTACK_CIRC_TIMEOUT 60
#include "or.h"
#include "circuituse.h"
#include "circuitlist.h"
#include "circuitbuild.h"
#include "circpathbias.h"
#include "rendclient.h"
#include "util.h"
#include "control.h"
#include "router.h"
#include "relay.h"
#include "time.h"
#include "compat.h"
#include "config.h"
#ifdef HAVE_EVENT2_EVENT_H
#include <event2/event.h>
#else
#include <event.h>
#endif



typedef enum {
  INITIALIZED=0,
  ATTACK_STATE_CONNECT_TO_INTRO=1,
  ATTACK_STATE_LAUNCHED=2
} attack_state_t;

typedef enum {
  CIRC_NO_STATE,
  REND_CIRC_BUILDING,
  REND_CIRC_READY_FOR_INTRO,
  REND_CIRC_READY_FOR_RD,
  REND_CIRC_INTRO_CELL_SENT,
  INTRO_CIRC_BUILDING,
  INTRO_CIRC_READY,
  INTRO_CIRC_LAUNCHED
} circuit_state_t;

typedef enum {
  ESTABLISH_RDV=0,
  SEND_RD=1,
  CHECK_HEALTHINESS=2
} hs_attack_cmd_t;

/*
 * Tor has a 1to1 mapping between intro and rend circ
 * when receiving cells; we can't have 1tomany
 * */
typedef struct circ_info_t {
  origin_circuit_t *rendcirc;
  origin_circuit_t *introcirc;
  extend_info_t *extend_info;
  circuit_state_t state_rend;
  circuit_state_t state_intro;
  time_t launched_at;
  int do_not_touch;
} circ_info_t;

typedef struct hs_attack_stats_t {
  uint64_t tot_cells;
  int nbr_rendcircs;
  int cells_per_circuit;
  tor_mutex_t attack_mutex;
} hs_attack_stats_t;

typedef struct hs_rd_attack_t {
  /* contain information needed to carry out the attack*/
  connection_t *conns;
  attack_state_t state;
  smartlist_t *rendcircs;
  hs_attack_stats_t *stats;
  const char *current_target;
  int retry_intro;
  int nbr_circuits;
  time_t *until;
  struct event *ev;
} hs_rd_attack_t;



hs_attack_stats_t*  hs_attack_entry_point(hs_attack_cmd_t, const char*, int, time_t*);

void hs_attack_mark_rendezvous_ready(origin_circuit_t*);

void hs_attack_mark_rendezvous_ready_for_intro(origin_circuit_t*);

void hs_attack_mark_intro_ready(origin_circuit_t *);

void hs_attack_send_intro_cell_callback(origin_circuit_t*);

void hs_attack_intro_circ_callback();

void hs_attack_mark_for_close_cb(circuit_t *circ, int reason);

#endif
