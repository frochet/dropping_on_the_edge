/*
 * The Shadow Simulator
 * Copyright (c) 2013-2014, John Geddes
 * See LICENSE for licensing information
 */

#ifndef SHD_TCP_SCOREBOARD_H_
#define SHD_TCP_SCOREBOARD_H_

#include "shadow.h"

typedef struct _ScoreBoard ScoreBoard;

ScoreBoard* scoreboard_new();
void scoreboard_clear(ScoreBoard* scoreboard);
void scoreboard_free(ScoreBoard* scoreboard);

TCPProcessFlags scoreboard_update(ScoreBoard* scoreboard, GList* selectiveACKs, gint32 unacked, gint32 next);
gint scoreboard_getNextRetransmit(ScoreBoard* scoreboard);
void scoreboard_markRetransmitted(ScoreBoard* scoreboard, gint sequence, gint sendNext);
void scoreboard_markLoss(ScoreBoard* scoreboard, gint unacked, gint sendNext);

void scoreboard_packetDropped(ScoreBoard* scoreboard, gint sequence);

#endif /* SHD_TCP_SCOREBOARD_H_ */
