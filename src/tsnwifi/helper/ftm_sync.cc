/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Sugandh Huthanahally Mohan – huthanahally@fortiss.org
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This file holds the definitions of all the functions required for time synchronization using FTM.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/perfect-clock-model-impl.h"
#include "ns3/local-clock.h"
#include "ns3/ftm-header.h"
#include "ns3/pointer.h"
#include "ns3/ftm_sync.h"
#include "ns3/wifi-setup.h"


using namespace ns3;

void sendtime(Ptr<Socket> socket, Ipv4Address &ipv4address, uint32_t port, Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_1)
{
	TimestampTag timestamp;
        Ptr<Packet> packet = Create<Packet>(1024);
        timestamp.SetTimestamp (clock_1->GetLocalTime());
        packet->AddByteTag (timestamp);
	/* Send the time of switch to AP as a byte tag*/
        socket->SendTo (packet, 0, InetSocketAddress (ipv4address, port));
}

void printTimes(Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_3, Ptr<LocalClock> clock_1)
{
        NS_LOG_UNCOND("*****Print AP and STA Time*********");
        NS_LOG_UNCOND(" AP " << clock_0->GetLocalTime().GetPicoSeconds() << " STA " << clock_3->GetLocalTime().GetPicoSeconds() << " diff " << clock_0->GetLocalTime() - clock_3->GetLocalTime() << " Switch " << clock_1->GetLocalTime().GetPicoSeconds());
        NS_LOG_UNCOND("*********************************");

}

void SessionOver (FtmSession session)
{
        NS_LOG_UNCOND("Measured RTT from FTM: " << session.GetMeanRTT ());
}

void OffsetManager (Ptr<FtmSession::FtmDialog> dialog, FtmSession session)
{
        Time offset;
       
        /* offset calculation using the ftm timestamps t1,t2,t3 and t4*/	
	if (dialog->t2 > dialog->t1)
        {
                offset = PicoSeconds((dialog->t2 - dialog->t1));
        }
        else
        {
                offset = PicoSeconds((dialog->t1 - dialog->t2));
        }
     
        Ptr<PerfectClockModelImpl> clockImpl = CreateObject<PerfectClockModelImpl> ();
        clockImpl->SetAttribute ("Offset", TimeValue (offset));
        clockImpl->SetAttribute ("Frequency", DoubleValue (1));
        /*Set the clock of STA */
	session.GetSTATSClock()->SetClock (clockImpl);
        
	//NS_LOG_UNCOND("After TS clock3 " << session.GetSTATSClock()->GetLocalTime());
}

void PropagationDelayManager (FtmSession session)
{
        Time rtt_time;
        Ptr<PerfectClockModelImpl> Impl = CreateObject<PerfectClockModelImpl> ();
        
        Impl->SetAttribute ("Offset", TimeValue(PicoSeconds(session.GetMeanRTT ()/2)));
        Impl->SetAttribute ("Frequency", DoubleValue (1));
       
        /* This part of code is not stable. Setting of local clocks sometimes result in improper values
	 * The below part is kind of a workaround to set the clock when propagation delay is available.*/	
	int64_t before = session.GetSTATSClock()->GetLocalTime().GetPicoSeconds();
        session.GetSTATSClock()->SetClock (Impl);
        int64_t after = session.GetSTATSClock()->GetLocalTime().GetPicoSeconds();

        Impl->SetAttribute ("Offset", TimeValue(PicoSeconds(before - after + session.GetMeanRTT ())));
        session.GetSTATSClock()->SetClock (Impl);
        //NS_LOG_UNCOND("After After PD adding clock3 " << session.GetSTATSClock()->GetLocalTime().GetPicoSeconds());

}

void ReceivePacket (Ptr<Socket> socket)
{
        Ptr<Packet> packet;
        Address from;
	Ptr<Node> node;
	Ptr<LocalClock> clk_AP;
        Time off;
        TimestampTag timestamp;
	
	/* Search for the clock of AP from list of nodes */
	for (uint32_t i = 0; i < NodeList::GetNNodes (); i++)
	{
		node = NodeList::GetNode (i);
		if(socket == node->GetObject<Socket>())
		{
			clk_AP = node->GetObject<LocalClock>();
		}
	}

	while ((packet = socket->RecvFrom (from))) 
	{
		if (InetSocketAddress::IsMatchingType (from)) 
		{
			//NS_LOG_UNCOND ("Received " << packet->GetSize () << " bytes from " <<
			//               InetSocketAddress::ConvertFrom (from).GetIpv4 ());
		}
		if (packet->FindFirstMatchingByteTag (timestamp)) 
		{
			Time tx = timestamp.GetTimestamp ();
			off = timestamp.GetTimestamp () - clk_AP->GetLocalTime();
			//NS_LOG_UNCOND (" received ts " << timestamp.GetTimestamp ().GetPicoSeconds() << " clock0 " << clock0->GetLocalTime().GetPicoSeconds() << " offset " << timestamp.GetTimestamp () - clock0->GetLocalTime() << " clock1 " << clock1->GetLocalTime().GetPicoSeconds());

		}
	}

	Ptr<PerfectClockModelImpl> clockImpl = CreateObject<PerfectClockModelImpl> ();
	/* Set the clock of AP after receiving timestamp as bytetag from switch*/
	clockImpl->SetAttribute ("Offset", TimeValue (off));
	clk_AP->SetClock (clockImpl);
}

void GenerateTraffic (Ptr<WifiNetDevice> ap, Ptr<WifiNetDevice> sta, Address recvAddr, FtmParams ftm_params, Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_3)
{

	Ptr<RegularWifiMac> sta_mac = sta->GetMac()->GetObject<RegularWifiMac>();

	Mac48Address to = Mac48Address::ConvertFrom (recvAddr);

	Ptr<FtmSession> session = sta_mac->NewFtmSession(to);
	if (session == 0)
	{
		NS_FATAL_ERROR ("ftm is not enabled");
	}

	//create the wired error model
	//Ptr<WiredFtmErrorModel> wired_error = CreateObject<WiredFtmErrorModel> ();
	//wired_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);
	Ptr<WirelessFtmErrorModel::FtmMap> map;
	map = CreateObject<WirelessFtmErrorModel::FtmMap> ();
	map->LoadMap ("src/wifi/ftm_map/10x10.map");

	//create wireless error model
	//map has to be created prior
	Ptr<WirelessFtmErrorModel> wireless_error = CreateObject<WirelessFtmErrorModel> ();
	wireless_error->SetFtmMap(map);
	wireless_error->SetNode(sta->GetNode());
	wireless_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);

	//using wired error model in this case
	session->SetFtmErrorModel(wireless_error);

	session->SetFtmParams(ftm_params);
	session->SetSessionOverCallback(MakeCallback(&SessionOver));/* Session Over Callback */
	session->SetOffsetCorrectionCallback(MakeCallback(&OffsetManager)); /* Offset Management callback */
	session->SetPropgationDelayCallback(MakeCallback(&PropagationDelayManager)); /* Propagation delay manager callback */
	session->SetSTATSClock(clock_3); /* Set the local clock of STA to the FTM Session */
	session->SessionBegin(); /* Begin FTM Session */
}

