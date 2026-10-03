// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/net/netEvent.h>
#include <daNet/disconnectionCause.h>
#include <generic/dag_tab.h>
#include <matching/types.h>

#include <string>

// server events
#define NET_ECS_EVENTS                                                                                                              \
    NET_ECS_EVENT(EventOnClientConnected, int /*connection_id*/, matching::UserId, std::string /*user_name*/, int64_t /*group_id*/, \
                  int64_t /*orig_group_id*/, uint16_t /*client_flags*/, std::string /*platform_user_id*/, std::string /*platform*/, \
                  int /*mteam team assigned by matching*/, int /*appId*/)                                                           \
    NET_ECS_EVENT(EventOnClientDisconnected, int /*connection_id*/, DisconnectionCause)                                             \
    NET_ECS_EVENT(EventOnDisconnectedFromServer, DisconnectionCause)                                                                \
    NET_ECS_EVENT(EventOnConnectedToServer)                                                                                         \
    NET_ECS_EVENT(EventOnNetworkDestroyed, int /*last_client_dc*/)                                                                  \
    NET_ECS_EVENT(NetEchoReponse, uint32_t /*routeId*/, int /*result*/, uint32_t /*rttOrTimeout*/)

#define NET_ECS_EVENT ECS_BROADCAST_EVENT_TYPE
NET_ECS_EVENTS
#undef NET_ECS_EVENT
