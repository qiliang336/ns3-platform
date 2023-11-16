/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Chitiphat Chongaraemsang, fortiss GmbH
* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * Sets Ethernet priorities per node
 */

#include "ns3/Sta-priority.h"

#include "ns3/tsn-module.h"
#include "ns3/traffic-control-module.h"

#include "ns3/queue-disc-container.h"
namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("StaPrio");

StaPrio::StaPrio(){}
StaPrio::~StaPrio () {}

Time callbackfunc(){
  return Simulator::Now();
}
int32_t StaPrio::ipv4PacketFilterPrio(Ptr<QueueDiscItem> item){
  return m_prio;
}
void StaPrio::install (Ptr<ns3::NetDevice> node)
{
    TrafficControlHelper tsnHelperSta;
	NetDeviceListConfig schedulePlanSta;
	CallbackValue timeSource = MakeCallback(&callbackfunc);
	
	/*NUMBER_OF_SCHEDULE_ENTRYS = 100, 100ms * 100 = 10secs*/
	for (int i = 0; i < 100; i++)
	{
		/**
		 * Use in Sta to tag the packets with all gate opened.
		*/
		schedulePlanSta.Add(Seconds(1000),{1,1,1,1,1,1,1,1});
	}

    tsnHelperSta.SetRootQueueDisc("ns3::TasQueueDisc", "NetDeviceListConfig", NetDeviceListConfigValue(schedulePlanSta), "TimeSource", timeSource,"DataRate", StringValue ("5Mbps"));
	tsnHelperSta.AddPacketFilter(0,"ns3::TsnIpv4PacketFilter","Classify",CallbackValue(MakeCallback(&StaPrio::ipv4PacketFilterPrio, this)));

	QueueDiscContainer qdiscsSta = tsnHelperSta.Install (node);

}

void StaPrio::SetPrio (int prio)
{
    m_prio = prio;
}

}
