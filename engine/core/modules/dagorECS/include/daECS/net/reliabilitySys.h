// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/net/sequence.h>
#include <util/dag_stdint.h>

#include "nau/utils/sorted_vector.h"

namespace net
{

typedef uint32_t ack_bits_t;

typedef void (*packet_event_cb_t)(sequence_t seq, void *param);

class ReliabilitySystem
{
public:
  ReliabilitySystem(sequence_t initial_seq = 0) : localSequence(initial_seq) {}

  void packetSent(int cur_time, int timeout_ms);
  void packetReceived(sequence_t seq);
  void processAck(sequence_t ack, ack_bits_t ack_bits, packet_event_cb_t packet_acked_cb, packet_event_cb_t packet_lost_cb,
    void *cb_param, int nlost_threshold = 3);
  void update(int cur_time, packet_event_cb_t packet_lost_cb, void *cb_param);

  struct AckData
  {
    sequence_t rseq;
    ack_bits_t ackBits;
  };
  AckData genLastSeqAckData() const; // packedReceived() is expected to be called at least once before call to this
  sequence_t getLocalSequence() const { return localSequence; }
  void incLocalSequence() { ++localSequence; }

private:
  sequence_t localSequence;
  struct PendingAckRec
  {
    sequence_t sequence;
    uint16_t nMissing;
    int timeoutAt;
    bool operator<(const PendingAckRec &rhs) const { return timeoutAt < rhs.timeoutAt; }
  };
  nau::SortedVectorMultiSet<PendingAckRec> pendingAcks;
  nau::SortedVectorSet<sequence_t, LessSeq> receivedQueue;
};

} // namespace net
