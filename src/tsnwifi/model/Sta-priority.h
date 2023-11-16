/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Chitiphat Chongaraemsang, fortiss GmbH
* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
*/

#ifndef Sta_PRIO_H
#define Sta_PRIO_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"

namespace ns3
{
/** \brief This is a "utility class".
 */
  class StaPrio
  {
    public:
      StaPrio ();
      virtual ~StaPrio ();

      void install (Ptr<ns3::NetDevice> node);
      void SetPrio (int32_t prio);
      int32_t ipv4PacketFilterPrio(Ptr<QueueDiscItem> item);
      int32_t m_prio = 0;
  };

}

#endif 
