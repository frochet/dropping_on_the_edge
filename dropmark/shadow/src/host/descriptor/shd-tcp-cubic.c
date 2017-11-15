/*
 * The Shadow Simulator
 * Copyright (c) 2013-2014, John Geddes
 * See LICENSE for licensing information
 */

#include "shadow.h"

#define BETA_SCALE 1024
#define HZ 1024
#define BICTCP_HZ 10

struct _Cubic {
    TCPCongestion super;
    gint32 maxWindow;
    gint32 lastMaxWindow;
    gint32 lossWindow;
    SimulationTime epochStart;
    SimulationTime lastTime;
    gint32 originPoint;
    gint32 delayMin;
    gint32 tcpWindowEst;
    gint32 k;
    gint32 ackCount;
    gint32 count;
    gint32 windowCount;

    gint32 beta;
    gint32 scalingFactor;
   
    gint32 betaScale;
    gint32 rttScale;
    gint64 cubeFactor;

    struct {
        gint32 found;
        gint32 lowThreshold;
        gint32 nSampling;
        gint32 samplingCount;
        gint32 roundStart;
        gint32 lastReset;
        gint32 lastTime;
        gint32 lastRTT;
        gint32 currRTT;
        gint32 delayMin;
        gint32 endSequence;
    } hystart;

    MAGIC_DECLARE;
};

static void _cubic_hystartReset(Cubic* cubic, gint ack) {
    MAGIC_ASSERT(cubic);
    TCPCongestion* congestion = (TCPCongestion*)cubic;

    SimulationTime now = worker_getCurrentTime() / SIMTIME_ONE_MILLISECOND;
    cubic->hystart.roundStart = now;
    cubic->hystart.lastReset = now;
    cubic->hystart.lastTime = now;
    cubic->hystart.lastRTT = congestion->rttSmoothed;
    cubic->hystart.currRTT = 0;
    cubic->hystart.samplingCount = cubic->hystart.nSampling;
    cubic->hystart.endSequence = ack;
}

static void _cubic_hystartUpdate(Cubic* cubic) {
    MAGIC_ASSERT(cubic);
    TCPCongestion* congestion = (TCPCongestion*)cubic;

    SimulationTime now = worker_getCurrentTime() / SIMTIME_ONE_MILLISECOND;
    gint rtt = congestion->rttSmoothed;
    if(!rtt) {
        rtt = 100;
    }

    gint delayMin = MIN(cubic->hystart.delayMin, rtt);

    if(!cubic->hystart.delayMin) {
        delayMin = rtt;
        cubic->hystart.delayMin = delayMin;
    }

    if(!cubic->hystart.found && congestion->window <= congestion->threshold) {
        if(now - cubic->hystart.lastTime <= 2) {
            cubic->hystart.lastTime = now;
            if(now - cubic->hystart.roundStart >= delayMin / 2) {
                cubic->hystart.found = 1;
            }
        }

        if(cubic->hystart.samplingCount) {
            cubic->hystart.currRTT = MIN(cubic->hystart.currRTT, rtt);
            if(!cubic->hystart.currRTT) {
                cubic->hystart.currRTT = rtt;
            }
            cubic->hystart.samplingCount--;
        }

        gint n = MAX(2, ceil(cubic->hystart.lastRTT / 16.0));
        if(!cubic->hystart.samplingCount && cubic->hystart.currRTT >= cubic->hystart.lastRTT + n) {
            cubic->hystart.found = 2;
        }

        if(cubic->hystart.found && congestion->window >= cubic->hystart.lowThreshold) {
            congestion->threshold = congestion->window;
        }
    }
}

    

static void _cubic_update(Cubic* cubic) {
    MAGIC_ASSERT(cubic);
    TCPCongestion* congestion = (TCPCongestion*)cubic;

    gint now = (gint)(worker_getCurrentTime() / SIMTIME_ONE_MILLISECOND);
    gint rtt = congestion->rttSmoothed;

    if(cubic->delayMin) {
        cubic->delayMin = MIN(cubic->delayMin, rtt);
    } else {
        cubic->delayMin = rtt;
    }

    cubic->ackCount += 1;

    if(now - cubic->lastTime <= HZ / 32) {
        return;
    }

    cubic->lastTime = now;

    if(!cubic->epochStart) {
        cubic->epochStart = now;
        if(congestion->window < cubic->lastMaxWindow) {
            cubic->k = cbrt(cubic->cubeFactor * (cubic->lastMaxWindow - congestion->window));
            cubic->originPoint = cubic->lastMaxWindow;
        } else {
            cubic->k = 0;
            cubic->originPoint = congestion->window;
        }
        cubic->ackCount = 1;
        cubic->tcpWindowEst = congestion->window;
    }
    
    gint t = now + cubic->delayMin - cubic->epochStart;
    gint64 offset = 0;
    if(t < cubic->k) {
        offset = cubic->k - t;
    } else {
        offset = t - cubic->k;
    }

    gint originDelta = (gint)((cubic->rttScale * offset * offset * offset) >> 40);
    gint target = 0;
    if(t < cubic->k) {
        target = cubic->originPoint - originDelta;
    } else {
        target = cubic->originPoint + originDelta;
    }
    
    if(target > congestion->window) {
        cubic->count = congestion->window / (target - congestion->window);
    } else {
        cubic->count = congestion->window * 100;
    }

    if(cubic->delayMin > 0) {
        gint minCount = (congestion->window * 1000 * 8) / (10 * 16 * cubic->delayMin);
        if(cubic->count < minCount && t >= cubic->k) {
            cubic->count = minCount;
        }
    }

    /* cubic_tcp_friendliness() */
    gint32 delta = (congestion->window * cubic->betaScale) >> 3;
    while (cubic->ackCount > delta) {       /* update tcp window */
        cubic->ackCount -= delta;
        cubic->tcpWindowEst++;
    }

    cubic->ackCount = 0;
    if(cubic->tcpWindowEst > congestion->window) {
        guint maxCount = congestion->window / (cubic->tcpWindowEst - congestion->window);
        if(cubic->count > maxCount) {
            cubic->count = maxCount;
        }
    }

    cubic->count /= 2;
    if(cubic->count == 0) {
        cubic->count = 1;
    }
}

void cubic_congestionAvoidance(Cubic* cubic, gint inFlight, gint packetsAcked, gint ack) {
    MAGIC_ASSERT(cubic);
    TCPCongestion* congestion = (TCPCongestion*)cubic;

    gint32 now = worker_getCurrentTime() / SIMTIME_ONE_MILLISECOND;
    if(now - cubic->hystart.lastReset >= congestion->rttSmoothed) {
        _cubic_hystartReset(cubic, inFlight);
    }
    _cubic_hystartUpdate(cubic);

    if(congestion->window <= congestion->threshold) {
        congestion->state = TCP_CCS_SLOWSTART;
        congestion->window++;
    } else {
        congestion->state = TCP_CCS_AVOIDANCE;
        _cubic_update(cubic);

        if(cubic->windowCount > cubic->count) {
            congestion->window += 1;
            cubic->windowCount = 0;
        } else {
            cubic->windowCount += 1;
        }
    }
}

guint cubic_packetLoss(Cubic* cubic) {
    MAGIC_ASSERT(cubic);
    TCPCongestion* congestion = (TCPCongestion*)cubic;

    cubic->epochStart = 0;
    if(congestion->window < cubic->lastMaxWindow) {
        cubic->lastMaxWindow = (congestion->window * (BETA_SCALE + cubic->beta)) / (2 * BETA_SCALE);
    } else {
        cubic->lastMaxWindow = congestion->window;
    }

    cubic->lossWindow = congestion->window;

    return MAX((congestion->window * cubic->beta) / BETA_SCALE, 2);
}

static void _cubic_free(Cubic* cubic) {
    MAGIC_ASSERT(cubic);
    MAGIC_CLEAR(cubic);
    g_free(cubic);
}

TCPCongestionFunctionTable CubicFunctions = {
    (TCPCongestionAvoidanceFunc) cubic_congestionAvoidance,
    (TCPCongestionPacketLossFunc) cubic_packetLoss,
    (TCPCongestionFreeFunc) _cubic_free,
    MAGIC_VALUE
};

Cubic* cubic_new(gint window, gint threshold) {
    Cubic *cubic = g_new0(Cubic, 1);
    MAGIC_INIT(cubic);

    if(!threshold) {
        threshold = 0x7FFFFFFF;
    }

    tcpCongestion_init(&(cubic->super), &CubicFunctions, TCP_CC_CUBIC, window, threshold);

    cubic->super.fastRetransmit = TCP_FR_SACK;

    /* cubic parameters */
    cubic->beta = 819;
    cubic->scalingFactor = 41;

    /* constants used in calculations */
    cubic->betaScale = 8 * (BETA_SCALE + cubic->beta) / 3 / (BETA_SCALE - cubic->beta);
    cubic->rttScale = cubic->scalingFactor * 10;
    cubic->cubeFactor = (gint64)(1ull << (10+3*BICTCP_HZ)) / (gint64)cubic->rttScale;

    cubic->hystart.found = FALSE;
    cubic->hystart.lowThreshold = 16;
    cubic->hystart.nSampling = 8;
    cubic->hystart.samplingCount = 8;
    
    return cubic;
}
