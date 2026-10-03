// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/net/message.h>
#include <daECS/net/msgSink.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_adjpow2.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>

#include <algorithm>
#include <functional>

#include "nau/utils/sorted_vector.h"

namespace net
{

extern void clear_net_msg_handlers();
extern void register_net_msg_handler(const net::MessageClass &klass, net::msg_handler_t handler);

struct MessageClassPtrLess
{
  constexpr bool operator()(const MessageClass *a, const MessageClass *b) const { return a->classHash < b->classHash; }
};
MessageClass *MessageClass::classLinkList = NULL;
int MessageClass::numClassIdBits = -1;
typedef nau::SortedVectorSet<const MessageClass*, MessageClassPtrLess> MessageClassesesVec;
static MessageClassesesVec incomingMessageClasses;

nau::Span<net::IConnection *> broadcast_rcptf(Tab<net::IConnection *> &, ecs::EntityId, const IMessage &)
{
  // no body, since it's never actually called (only address of this function is used)
  return nau::Span<net::IConnection *>();
}

nau::Span<net::IConnection *> direct_connection_rcptf(Tab<net::IConnection *> &, ecs::EntityId, const IMessage &msg)
{
  if (msg.connection)
    return make_span((net::IConnection **)&msg.connection, 1);
  else
    return nau::Span<net::IConnection *>();
}

MessageClass::MessageClass(const char *class_name, uint32_t class_hash, uint32_t class_sz, MessageRouting rout, bool timed,
  recipient_filter_t rcptf, PacketReliability rlb, uint8_t chn, uint32_t flags_, int dup_delay_ms,
  void (*msg_sink_handler)(const IMessage *)) :
  msgSinkHandler(msg_sink_handler), debugClassName(class_name), classHash(class_hash), memSize(class_sz)
{
  routing = rout, reliability = rlb;
  channel = chn;
  flags = flags_ | (timed ? MF_TIMED : 0);
  rcptFilter = rcptf;
  dupDelay = dup_delay_ms;
  NAU_FAST_ASSERT(memSize != 0);
  // add to list
  next = MessageClass::classLinkList;
  MessageClass::classLinkList = this;
}

/* static */
uint32_t MessageClass::init(bool server)
{
  if (!classLinkList)
    return 0;
  clear_net_msg_handlers();
  MessageClassesesVec outgoingMessageClasses;
  incomingMessageClasses.clear();
  incomingMessageClasses.reserve(64);
  outgoingMessageClasses.reserve(64);
  uint32_t nmc = 0;
  size_t maxMesageClassLen = 0;
  for (MessageClass *cls = classLinkList; cls; cls = cls->next)
  {
    MessageRouting rout = cls->routing;
    if (rout == ROUTING_SERVER_TO_CLIENT && !cls->rcptFilter)
    {
      logerr("Server to client message '%s'(%x) must have recipientFilter. Check message registration options.", cls->debugClassName,
        cls->classHash);
      continue;
    }
    bool outgoing = (server ? (rout == ROUTING_SERVER_TO_CLIENT)
                            : (rout == ROUTING_CLIENT_TO_SERVER || rout == ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER));
    auto ins = (outgoing ? outgoingMessageClasses : incomingMessageClasses).insert(cls);
    NAU_ASSERT(ins.second, "Net messages hash collision %x for %s/%s!", cls->classHash, (*ins.first)->debugClassName,
      cls->debugClassName);
    NAU_UNUSED(ins);
    if (cls->msgSinkHandler && !outgoing)
      register_net_msg_handler(*cls, cls->msgSinkHandler);
    if (cls->debugClassName)
        maxMesageClassLen = std::max(maxMesageClassLen, strlen(cls->debugClassName));
    ++nmc;
  }
  for (int i = 0; i < incomingMessageClasses.size(); ++i)
    const_cast<MessageClass *>(incomingMessageClasses[i])->classId = i; // for receival (getById)
  for (int i = 0; i < outgoingMessageClasses.size(); ++i)
    const_cast<MessageClass *>(outgoingMessageClasses[i])->classId = i; // for sending
  incomingMessageClasses.shrink_to_fit();
  int numMessageClasses = (int)std::max(incomingMessageClasses.size(), outgoingMessageClasses.size());  // To consider: use different
                                                                                                        // number bits for incoming &
                                                                                                        // outgoing messages
  NAU_ASSERT(numMessageClasses >= 1);
#if _DEBUG
  if (numClassIdBits < 0 && maxMesageClassLen && dgs_get_settings()->getBlockByNameEx("net")->getBool("debugDumpMessageClasses", true))
  {
    NAU_CORE_DEBUG_LF("%3s %*s/hash %*s/hash", "#", maxMesageClassLen, "incoming_msg", maxMesageClassLen, "outgoing_msg");
    int j = 0;
    do
    {
      const MessageClass *ic = j < incomingMessageClasses.size() ? incomingMessageClasses[j] : nullptr;
      const MessageClass *oc = j < outgoingMessageClasses.size() ? outgoingMessageClasses[j] : nullptr;
      if (!ic && !oc)
        break;
      NAU_CORE_DEBUG_LF("%3d %*s/%#8x %*s/%#8x", j, maxMesageClassLen, ic ? ic->debugClassName : nullptr, ic ? ic->classHash : 0,
        maxMesageClassLen, oc ? oc->debugClassName : nullptr, oc ? oc->classHash : 0);
      j++;
    } while (1);
  }
#else
  NAU_UNUSED(maxMesageClassLen);
#endif
  numClassIdBits = (numMessageClasses == 1) ? 1 : (get_log2i(numMessageClasses) + 1);
  return nmc;
}

/* static */
const MessageClass *MessageClass::getById(int msg_class_id)
{
  return ((unsigned)msg_class_id < incomingMessageClasses.size()) ? incomingMessageClasses[msg_class_id] : nullptr;
}

/* static */
int MessageClass::getNumClassIdBits() { return numClassIdBits; }

/* static */
uint32_t MessageClass::calcNumMessageClasses()
{
  uint32_t nmc = 0;
  for (const MessageClass *cls = classLinkList; cls; cls = cls->next)
    ++nmc;
  return nmc;
}

} // namespace net
