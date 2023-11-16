/*****************************************************************************
 * Copyright (C) 2020, 2021, 2022 fortiss GmbH
  *@author Sugandh Huthanahally Mohan – huthanahally@fortiss.org
 * @author Chitiphat Chongaraenssamg, fortiss GmbH
 * version 1.0
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * Provides a scenario for 1 wired TSN region (Time Aware Shaper) interconnected with
 * 1 wireless TSN region
 * Implements a TWT based scheduling approach - see documentation
 *
 * Topology:
 *
 *	Wifi 192.168.1.0			      			LAN 192.168.5.0
 *	    W1 ----\				        			/--- n4
 *	    W2 ----- AP ---- |Switch0| ---- |Switch1| ------ n5
 *	    W3 ----/   		 	        			\--- n6
 *	                     
 *
 * Switches and STA TWT cycle
 * 
 * 				  T=0															T=100ms
 * TAS on Switches |---Prio 5---|-----All closed----|------------Prio 0----------|
 * 							   20ms				  50ms
 * W1			   |------------Resume--------------|-----------Suspend----------|
 * 												  50ms
 * W2			   |------------Suspend-------------|-----------Resume-----------|
 * 											  	  50ms
*/

 
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ssid.h"
#include "ns3/tsn-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-global-routing-helper.h" 
#include "ns3/bridge-helper.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"
#include "ns3/perfect-clock-model-impl.h"
#include "ns3/local-clock.h"

#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"

#include "ns3/sleep-cycle-application.h"
#include "ns3/switch-trace-application.h"
#include "ns3/send-action-frame-application.h"
#include "ns3/wifi-setup.h"
#include "ns3/Sta-priority.h"

#include "ns3/on-off-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood-plus.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"

#define NUMBER_OF_SCHEDULE_ENTRYS 100




using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TSNWifi");


/* These functions are defined in src/tsnwifi - refer to documentation
* sets up Ethernet priorities
* SendActioFrameSetup is an ns-3 application to model TWT Action Frames
* SleepCycleSetup is the application that models TWT behaviour
*/

int32_t ipv4PacketFilterPrio5(Ptr<QueueDiscItem> item);
int32_t ipv4PacketFilterPrio0(Ptr<QueueDiscItem> item);
// void SendActionFrameSetup(Ptr<ns3::Node> Node, Time Freq, int StaID, int SpID, int WakeInterval, int WakeDuration, int WakeTime, Time StartTime, Time StopTime);
// void SleepCycleSetup(Ptr<ns3::Node> Node, int StaID, Time StartTime, Time StopTime);

Time callbackfunc(){
  return Simulator::Now();
}

int 
main (int argc, char *argv[])
{

	int simulationTime = 10;
	bool verbose = true;
	uint32_t nWifi = 3;
	uint32_t nWire = 3;
	uint32_t nSwitch = 2;
	uint32_t nAP = 1;
	int mcs {9};
	std::string dlAckSeqType {"MU-BAR"};
	//double frequency {5}; //whether 2.4, 5 or 6 GHz
	//uint8_t maxChannelWidth = frequency == 2.4 ? 40 : 160;
	int channelWidth = 40;
	int gi = 800;
	bool useExtendedBlockAck {false};

	nWire = nWire == 0 ? 3 : nWire;
	int trafficNo = 1;
	CommandLine cmd;
	// TODO: add total of nodes via the command line.
	cmd.AddValue("nWire", "Number of Sta nodes", nWire);
	cmd.AddValue("nWifi", "Number of Wifi nodes", nWifi);
	cmd.AddValue("nSwitch", "Number of Switch", nSwitch);
	cmd.AddValue("nAP", "Number of Access Point", nAP);
	cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
	cmd.AddValue ("mcs", "if set, limit testing to a specific MCS (0-11)", mcs);
	cmd.AddValue ("dlAckType", "Ack sequence type for DL OFDMA (NO-OFDMA, ACK-SU-FORMAT, MU-BAR, AGGR-MU-BAR)",
                dlAckSeqType);
	cmd.AddValue ("channelWidth", "Channel Width", channelWidth);
	cmd.AddValue ("gi", "Gaurd Interval", gi);
	cmd.AddValue ("useExtendedBlockAck", "Enable/disable use of extended BACK", useExtendedBlockAck);
	cmd.AddValue("trafficNo", "Which traffic profile to send?", trafficNo);
	cmd.Parse(argc, argv);
	Config::SetDefault ("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                          EnumValue (WifiAcknowledgment::DL_MU_TF_MU_BAR));

	// LOGGING information; Clock modelling only required if sync experiments are done
	if (verbose) {
		
		LogComponentEnable("UdpL4Protocol", LOG_LEVEL_ALL);
		LogComponentEnable("UdpSocketImpl", LOG_LEVEL_ALL);
		LogComponentEnable("UdpClient", LOG_LEVEL_ALL);
		LogComponentEnable("UdpServer", LOG_LEVEL_ALL);
		LogComponentEnable("LocalClock", LOG_LEVEL_ALL);
		LogComponentEnable("ClockModel", LOG_LEVEL_ALL);
		LogComponentEnable("PerfectClockModelImpl", LOG_LEVEL_ALL);
		LogComponentEnable("LocalTimeSimulatorImpl", LOG_LEVEL_ALL);		
	}
	GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
	Time::SetResolution (Time::NS);
	Time sendPeriod,scheduleDuration,simulationDuration;
	simulationDuration = 2*scheduleDuration*NUMBER_OF_SCHEDULE_ENTRYS;
	sendPeriod = MilliSeconds(250);
	CallbackValue timeSource = MakeCallback(&callbackfunc);

	/*******************************************************************************************
	 * Create Devices and  connections between devices
	 ******************************************************************************************/
	 
	//Create devices	
	NodeContainer wireStaNodeContainer;
	wireStaNodeContainer.Create(nWire);
	NodeContainer switchNodeContainer;
	switchNodeContainer.Create(nSwitch); 
	NodeContainer APNodeContainer;
	APNodeContainer.Create(nAP);
	NodeContainer wifiStaNodeContainer;
  	wifiStaNodeContainer.Create (nWifi);

	// Create link between switches
	CsmaHelper csmaX;
	csmaX.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csmaX.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (10560)));

	NetDeviceContainer switchFullLink;
	switchFullLink = csmaX.Install (switchNodeContainer);

	// Create container for switch0 and the link to switch1
	NetDeviceContainer switch0Link;
	switch0Link.Add(switchFullLink.Get(0));

	// Create link between switch0 and AP0
	CsmaHelper csmaY;
	csmaY.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csmaY.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
	NetDeviceContainer switchAPLink;
	switchAPLink = csmaX.Install (NodeContainer(switchNodeContainer.Get (0), APNodeContainer.Get(0))); //Why not csmaY

	// Create container for the link and devices   (WHY?)
	NetDeviceContainer APDevices;
	APDevices.Add(switchAPLink.Get(1)); //link to switch0 and AP0
	switch0Link.Add(switchAPLink.Get(0)); // link to AP0 and switch0

	// Create the csma links, from each terminal to the switch1 
	CsmaHelper csma; 
	csma.SetChannelAttribute ("DataRate", StringValue ("10Mbps"));
	csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (560)));
	
	// Create device with link to switch1
	NetDeviceContainer wireDevices;
	NetDeviceContainer switch1Link;
	for (uint32_t i = 0; i < nWire; i++) {
		NetDeviceContainer link = csma.Install(NodeContainer(wireStaNodeContainer.Get (i), switchNodeContainer.Get(1)));
		wireDevices.Add(link.Get (0));
		switch1Link.Add(link.Get (1));
	}
	switch1Link.Add(switchFullLink.Get (1));

	
	/********************************************************************************************
	 * CLOCK MODEL: only required for time sync purposes
	 ********************************************************************************************/

	GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::LocalTimeSimulatorImpl"));
	NodeContainer AllNodeContainer = NodeContainer(wireStaNodeContainer, wifiStaNodeContainer, APNodeContainer, switchNodeContainer);
	int numnodes = AllNodeContainer.GetN();
	NS_LOG_UNCOND("\nTotal number of Nodes: " << numnodes);
	Ptr<LocalClock> clock[numnodes] = {CreateObject<LocalClock> ()};
	
	Time init_offset = Seconds (0);
	double freq = 1;

	for(int i = 0; i < numnodes;i++)
	{
		Ptr<PerfectClockModelImpl> clockImpl = CreateObject <PerfectClockModelImpl> ();
		clockImpl->SetAttribute ("Frequency", DoubleValue (freq));
		clockImpl->SetAttribute ("Offset", TimeValue (init_offset));
		clock[i] = CreateObject<LocalClock> ();
		clock[i]->SetAttribute ("ClockModel", PointerValue (clockImpl));
		Ptr<Node> n = AllNodeContainer.Get (i);
		n->AggregateObject (clock[i]);
	}

/********************************************************************************************
 * END OF CLOCK MODEL: only required for time sync purposes
 ********************************************************************************************/

/********************************************************************************************
 * Setting up of WiFi devices, SSIDs, channels, etc
 ********************************************************************************************/

	NetDeviceContainer apDevices, staDevices;
	std::string ssid_name = "ns-3-ssid";
	WifiSetup wifi;
	wifi.ConfigureDevices(APNodeContainer, wifiStaNodeContainer,ssid_name,0,46,40);
	apDevices = wifi.GetNetDeviceContainerAP();
	staDevices = wifi.GetNetDeviceContainerSTA();

	// ======================================================================
	// Install internet stack
	// ----------------------------------------------------------------------
	InternetStackHelper stack;
	stack.Install(wireStaNodeContainer);
	stack.Install(switchNodeContainer);
	//stack.Install(APNodeContainer);	
	//stack.Install(wifiStaNodeContainer);

	// ======================================================================
	// Scheduling the packets
	// ----------------------------------------------------------------------
	TrafficControlHelper tsnHelperBBB1, tsnHelperBBB2, tsnHelperSwitch1, tsnHelperSwitch0;
	NetDeviceListConfig schedulePlanBBB1, schedulePlanBBB2, schedulePlanSwitch1,schedulePlanSwitch0;
	scheduleDuration = Seconds(1);
	
	/*NUMBER_OF_SCHEDULE_ENTRYS = 100, 100ms * 100 = 10secs*/
	//schedulePlanSwitch1.Add(MilliSeconds(5),{0,0,0,0,0,0,0,0});
	//schedulePlanSwitch0.Add(MilliSeconds(1000),{1,1,1,1,1,1,1,1});
	for (int i = 0; i < NUMBER_OF_SCHEDULE_ENTRYS; i++)
	{
		/**
		 * Install in switch1 and switch0. 
		*/
		schedulePlanSwitch1.Add(MilliSeconds(20),{0,1,0,0,0,0,0,0});
		schedulePlanSwitch1.Add(MilliSeconds(50),{0,0,0,0,0,1,0,0});
		schedulePlanSwitch1.Add(MilliSeconds(30),{0,1,0,0,0,0,0,0});

		schedulePlanSwitch0.Add(MilliSeconds(50),{0,1,0,0,0,0,0,0});
		schedulePlanSwitch0.Add(MilliSeconds(50),{0,0,0,0,0,1,0,0});
	}

	StaPrio bbb3;
	bbb3.SetPrio(5);
	bbb3.install(wireDevices.Get(nWire-3));
	StaPrio bbb2;
	bbb2.SetPrio(5);
	bbb2.install(wireDevices.Get(nWire-2));
	StaPrio bbb1;
	bbb1.SetPrio(5);
	bbb1.install(wireDevices.Get(nWire-1));


	tsnHelperSwitch1.SetRootQueueDisc("ns3::TasQueueDisc", 
									"NetDeviceListConfig", NetDeviceListConfigValue(schedulePlanSwitch1), 
									"TimeSource", timeSource,
									"DataRate", StringValue ("10Mbps"));
	//tsnHelperSwitch1.AddPacketFilter(0,"ns3::TsnIpv4PacketFilter","Classify",CallbackValue(MakeCallback(&ipv4PacketFilterPrio0)));
	
	tsnHelperSwitch0.SetRootQueueDisc("ns3::TasQueueDisc", 
									"NetDeviceListConfig", NetDeviceListConfigValue(schedulePlanSwitch0), 
									"TimeSource", timeSource,
									"DataRate", StringValue ("10Mbps"));
	//tsnHelperSwitch0.AddPacketFilter(0,"ns3::TsnIpv4PacketFilter","Classify",CallbackValue(MakeCallback(&ipv4PacketFilterPrio0)));

	/**
	 * QueueDiscHelper needs to install the queuedisc on the correct netdevice.
	 * switchFullLink.Get(1) = netdevice(on switch1) with the port that is connected to switch0.
	 * switchAPLink.Get(0) = netdevice(on switch0) with the port that is connected to the AP.
	 * 
	 * STA1<------AP<-------Switch0<-------Switch1<-----BBB1
	*/
	//QueueDiscContainer a = tsnHelperSwitch1.Install (switchFullLink.Get(1));
	//QueueDiscContainer b = tsnHelperSwitch0.Install (switchAPLink.Get(0));

	// ======================================================================
	// Assign address to devices
	// ----------------------------------------------------------------------
	Ipv4AddressHelper address;
	address.SetBase ("192.168.5.0", "255.255.255.0");
	Ipv4InterfaceContainer wireInterfaces;
	wireInterfaces = address.Assign (wireDevices);
	Ipv4InterfaceContainer APInterfaces;
	APInterfaces = address.Assign (APDevices);

	address.SetBase ("192.168.1.0", "255.255.255.0");
	Ipv4InterfaceContainer staInterfaces;
	staInterfaces = address.Assign (staDevices);
	Ipv4InterfaceContainer APWifiInterfaces;
	APWifiInterfaces = address.Assign (apDevices);

	
	// ======================================================================
	// Install bridging code on each switch
	// ----------------------------------------------------------------------
	BridgeHelper bridge;
	
	NetDeviceContainer switch0DeviceContainer;
	NetDeviceContainer switch1DeviceContainer;
	
	switch0DeviceContainer = bridge.Install(switchNodeContainer.Get(0), switch0Link);
	switch1DeviceContainer = bridge.Install(switchNodeContainer.Get(1), switch1Link);

	
	// ======================================================================
	// UDP Flow Application Setup
	// ----------------------------------------------------------------------
	ApplicationContainer serverApp1;
	UdpServerHelper server1 (9);
	serverApp1 = server1.Install (wireStaNodeContainer.Get (0));
	serverApp1.Start (Seconds (0.0));
	serverApp1.Stop (Seconds (simulationTime + 1));


	UdpClientHelper client1 (wireInterfaces.GetAddress (0), 9);
	client1.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
	client1.SetAttribute ("Interval", TimeValue (Time ("0.003"))); // packet interval
	client1.SetAttribute ("PacketSize", UintegerValue (200)); // packet size in bytes
	ApplicationContainer clientApp1 = client1.Install (wifiStaNodeContainer.Get (nWifi - 3));
	clientApp1.Start (Seconds (1.2));
	clientApp1.Stop (Seconds (simulationTime + 1));
                    
	// ======================================================================
	// Switch trace application
	// ----------------------------------------------------------------------
	Ptr<SwitchTraceApplication> trace1 = CreateObject<SwitchTraceApplication>();
	trace1->SetNetDevice(switch1DeviceContainer.Get(0));
	trace1->SetStartTime (Seconds (0.2));
    	//trace1->SetStopTime (Seconds (simulationTime));
	//switchNodeContainer.Get(1)->AddApplication(trace1);

	Ptr<SwitchTraceApplication> trace2 = CreateObject<SwitchTraceApplication>();
	trace2->SetNetDevice(switch0DeviceContainer.Get(0));
	trace2->SetStartTime (Seconds (0.2));
    	//trace2->SetStopTime (Seconds (simulationTime));
	//switchNodeContainer.Get(0)->AddApplication(trace2);

	// ======================================================================
	// Sleep Resume cycle application
	// ----------------------------------------------------------------------
	/**
	 * @brief Example installing SleepCycleApplication
	 * 
	 */
	//SleepCycleSetup(wifiStaNodeContainer.Get(0), 3, Seconds(0.2), Seconds(simulationTime));

	// ======================================================================
	// Send action frame application
	// ----------------------------------------------------------------------
	/**
	 * @brief Example installing SendActionFrameApplication
	 * 
	 */
	/*
	Time Frequency = MilliSeconds(100);
	int StaID = 3;
	int SpID = 1;
	int WakeInterval = 100;
	int WakeDuration = 50;
	int WakeTime = 30;
	Time StartTime = Seconds(1);
	Time StopTime = Seconds(2);
	//SendActionFrameSetup(APNodeContainer.Get(0), Frequency, StaID, SpID, WakeInterval, WakeDuration, WakeTime, StartTime, StopTime);
	*/
	// ======================================================================
	// Video flow
	// ----------------------------------------------------------------------
	/**
	 * The server will be sending the frames to the client. 
	 * Meaning, STA will be client and BBB will be server.
	*/
	/*
	VideoStreamClientHelper videoClient (wireInterfaces.GetAddress(nWire-2), 5000);
    	ApplicationContainer clientAppVideo = videoClient.Install (wifiStaNodeContainer.Get(nWifi-2));
    	clientAppVideo.Start (Seconds (0.5));
    	clientAppVideo.Stop (Seconds (simulationTime));

    	VideoStreamServerHelper videoServer (5000);
    	videoServer.SetAttribute ("MaxPacketSize", UintegerValue (1440));
    	//videoServer.SetAttribute ("FrameFile", StringValue ("./frameList.txt"));
    	videoServer.SetAttribute ("FrameFile", StringValue ("./small.txt"));
	//videoServer.SetAttribute ("Interval", TimeValue(Seconds (0.1)) ); //should be able to varies
	videoServer.SetAttribute ("Maximal", IntegerValue(10)); //maximal interval between each packets in milliseconds, minimal will be 0.1 of maximimal

    	ApplicationContainer serverAppVideo = videoServer.Install (wireStaNodeContainer.Get(nWire-2));
    	serverAppVideo.Start (Seconds (0.5));
    	serverAppVideo.Stop (Seconds (simulationTime-5));
	*/
	// ======================================================================
	Simulator::Schedule (Seconds (0), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

	FlowMonitorHelper flowmon;
  	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	Simulator::Stop (Seconds (simulationTime));

	/**
	 * Saving output as .pcap file
	*/
	//csma.EnablePcap ("TSNWifi-wire", APDevices.Get (0), true);
	//csma.EnablePcap ("TSNWifi-wire", wireDevices.Get (0), true);
	//csma.EnablePcap ("TSNWifi-Switch1-AP", switchFullLink.Get (1), true);
	//csma.EnablePcap ("TSNWifi-Switch1-Node0", switch1Link.Get (0), false);
	//csma.EnablePcap ("TSNWifi-Switch1-Switch0", switch1Link.Get (3), true);
	//csma.EnablePcap ("TSNWifi-Switch0-Switch1", switchFullLink.Get (0), false);

	Simulator::Run ();

	// ======================================================================
	// Additional information such as delay, jitter and packet loss etc.
	// ----------------------------------------------------------------------

	uint32_t SentPackets = 0;
	uint32_t ReceivedPackets = 0;
	uint32_t LostPackets = 0;
	int j=0;
	float AvgThroughput = 0;
	Time Jitter;
	Time Delay;

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
	{
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

		NS_LOG_UNCOND("\n----Flow ID:" <<iter->first);
		NS_LOG_UNCOND("Src Addr " <<t.sourceAddress << " Dst Addr "<< t.destinationAddress);
		NS_LOG_UNCOND("Sent Packets=" <<iter->second.txPackets);
		NS_LOG_UNCOND("Received Packets =" <<iter->second.rxPackets);
		//NS_LOG_UNCOND("Lost Packets =" <<iter->second.txPackets-iter->second.rxPackets);
		NS_LOG_UNCOND("Packet delivery ratio =" <<iter->second.rxPackets*100/iter->second.txPackets << "%");
		//NS_LOG_UNCOND("Packet loss ratio =" << (iter->second.txPackets-iter->second.rxPackets)*100/iter->second.txPackets << "%");
		NS_LOG_UNCOND("Packet loss percentage =" << iter->second.txPackets-iter->second.rxPackets << "/" << iter->second.txPackets << "(" << (iter->second.txPackets-iter->second.rxPackets)*100/iter->second.txPackets << "%)");
		NS_LOG_UNCOND("Avg. Delay =" << NanoSeconds(((iter->second.delaySum).GetInteger())/iter->second.rxPackets));
		NS_LOG_UNCOND("Sum. Delay =" << iter->second.delaySum);
		NS_LOG_UNCOND("Avg. Jitter =" << NanoSeconds(((iter->second.jitterSum).GetInteger())/iter->second.rxPackets));
		NS_LOG_UNCOND("Sum. Jitter =" <<iter->second.jitterSum); //
		/**
		 * @brief check how the jitter is computed
		 * 	Time jitter = stats.lastDelay - delay;
			if (jitter > Seconds (0))
				{
					stats.jitterSum += jitter;
					stats.jitterHistogram.AddValue (jitter.GetSeconds ());
				}
		 * 
		 */
		NS_LOG_UNCOND("Throughput =" <<iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024<<"Kbps");

		SentPackets = SentPackets +(iter->second.txPackets);
		ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
		LostPackets = LostPackets + (iter->second.txPackets-iter->second.rxPackets);
		AvgThroughput = AvgThroughput + (iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024);
		Delay = Delay + (iter->second.delaySum);
		Jitter = Jitter + (iter->second.jitterSum);

		j = j + 1;

	}

	AvgThroughput = AvgThroughput/j;
	NS_LOG_UNCOND("\n--------Total Results of the simulation----------");
	NS_LOG_UNCOND("Total sent packets  =" << SentPackets);
	NS_LOG_UNCOND("Total Received Packets =" << ReceivedPackets);
	NS_LOG_UNCOND("Total Lost Packets =" << LostPackets);
	NS_LOG_UNCOND("Packet Loss ratio =" << ((LostPackets*100)/SentPackets)<< "%");
	NS_LOG_UNCOND("Packet delivery ratio =" << ((ReceivedPackets*100)/SentPackets)<< "%");
	NS_LOG_UNCOND("Average Throughput =" << AvgThroughput<< "Kbps");
	NS_LOG_UNCOND("End to End Delay =" << Delay);
	NS_LOG_UNCOND("End to End Jitter delay =" << Jitter);
	NS_LOG_UNCOND("Total Flow id " << j);

	/**
	 * Saving output as .xml file
	*/
	//monitor->SerializeToXmlFile("manet-routing.xml", true, true);

	Simulator::Destroy ();
	

	return 0;
}

void SendActionFrameSetup(Ptr<ns3::Node> Node, Time Freq, int StaID, int SpID, int WakeInterval, int WakeDuration, int WakeTime, Time StartTime, Time StopTime)
{
	Ptr<SendActionFrameApplication> action = CreateObject<SendActionFrameApplication>();
	action->SetFrequency(Freq);
	action->SetStaID(StaID);
	action->SetSpID(SpID);
	action->SetWakeInterval(WakeInterval);
	action->SetWakeDuration(WakeDuration);
	action->SetWakeTime(WakeTime);
    action->SetStartTime (StartTime);
    action->SetStopTime (StopTime);
	Node->AddApplication (action);
	return;
}

void SleepCycleSetup(Ptr<ns3::Node> Node, int StaID, Time StartTime, Time StopTime)
{
	Ptr<SleepCycleApplication> app = CreateObject<SleepCycleApplication>();
	app->SetStaID(StaID);
    app->SetStartTime (StartTime);
    app->SetStopTime (StopTime);
    Node->AddApplication (app);
	return;
}

int32_t ipv4PacketFilterPrio5(Ptr<QueueDiscItem> item){
  return 5;
}

int32_t ipv4PacketFilterPrio0(Ptr<QueueDiscItem> item){
  return 0;
}
