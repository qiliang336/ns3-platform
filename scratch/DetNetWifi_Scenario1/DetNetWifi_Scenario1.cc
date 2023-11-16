/**
 * Copyright (C) fortiss GmbH 2022
 * @author Sugandh Huthanahally Mohan – huthanahally@fortiss.org
 * @author Ahmed Hasan Ansari 
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 *
 * version 1.0
 * Provides a scenario for two overlapping BSSs, i.e., 2 APs sharing the same primary channel
 * Scenario has been used to explore multi-AP coordination approaches
 * File includes our proposed modelling for a multi-AP Co-OFDMA probabilistic approach, where x APs (currently 2)
 * share a channel based on probabilities, in an attempt to optimize shared resources across overlapping areas
 * The scenario is based on the TSNWifi scenario, so the 2 APs connect to the wired part (1 AP to each switch)
 * However, for the co-OFDMA experiments, we consider only the wireless elements AP1, AP2, W1 and W3.
 *
 *    W3 -- AP1 -----                             ---- ETH1
*                    \                            \ 
 *    W2             \-- switch0 --- switch1 ----------ETH2
 *                   \                             \
 *    W1 -- AP2 -----\                             \    ETH3
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

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/ftm-header.h"
#include "ns3/mgt-headers.h"
#include "ns3/ftm-error-model.h"
#include "ns3/pointer.h"


#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"

#include "ns3/sleep-cycle-application.h"
#include "ns3/switch-trace-application.h"
#include "ns3/send-action-frame-application.h"
#include "ns3/wifi-setup.h" //Header file has been modified
#include "ns3/Sta-priority.h"
#include "ns3/ftm_sync.h"

#include "ns3/on-off-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood-plus.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-address.h"
#include "myheader.h"

#define NUMBER_OF_SCHEDULE_ENTRYS 100

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DetNetWifi_Scenario1");

int32_t ipv4PacketFilterPrio5(Ptr<QueueDiscItem> item);
int32_t ipv4PacketFilterPrio0(Ptr<QueueDiscItem> item);
void SendActionFrameSetup(Ptr<ns3::Node> Node, Time Freq, int StaID, int SpID, int WakeInterval, int WakeDuration, int WakeTime, Time StartTime, Time StopTime);
void SleepCycleSetup(Ptr<ns3::Node> Node, int StaID, Time StartTime, Time StopTime);
/**
 * sendtime - Send time from switch to AP.
 *
 * \param socket socket to send time from switch
 * \param ipv4address send to this address
 * \param port send to this port
 * \param clock_0 local clock of the AP
 * \param clock_1 local clock of the switch
 in src/tsnwifi/helper/ftm_sync.h in 3.34
 */
void sendtime(Ptr<Socket> socket, Ipv4Address &ipv4address, uint32_t port, Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_1);
/**
 * ReceivePacket - Receive the time sent from switch to AP.
 *
 * \param socket socket to receive.
 in src/tsnwifi/helper/ftm_sync.h in 3.34
 */
void ReceivePacket (Ptr<Socket> socket);

/**
 * printTimes - Prints the local clock values of AP, STA and the Switch.
 *
 * \param clock_0 local clock of AP
 * \param clock_3 local clock of STA
 * \param clock_1 local clock of Switch
 in src/tsnwifi/helper/ftm_sync.h in 3.34
 */
void printTimes(Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_3, Ptr<LocalClock> clock_1);

int rss;


Time callbackfunc(){
  return Simulator::Now();
}

/**********************************************************************************************************
 * Sending traffic
 * UDPFlows is an ns-3 application to send UDP flows in parallel
 **********************************************************************************************************/

void UDPFlows(Ipv4InterfaceContainer& server,NodeContainer& client){

	UdpClientHelper client1 (server.GetAddress(0), 9);
  client1.SetAttribute ("MaxPackets", UintegerValue (4294967295u)); //total number of packets
  client1.SetAttribute ("Interval", TimeValue (Time ("1"))); //packet interarrival time
  client1.SetAttribute ("PacketSize", UintegerValue (30)); //packet size in bytes
  ApplicationContainer clientApp1 = client1.Install (client.Get(0));
  clientApp1.Start (Seconds (2));
  clientApp1.Stop (Seconds (200 + 1));
                    NS_LOG_UNCOND("\nUDP client ");

}

/**
 * Definition of APs for debugging purposes
 */

	WifiSetup *WAP1;
	WifiSetup *WAP2;
	Ptr<WifiNetDevice> wnd;
	Ptr<WifiNetDevice> wnd2;




/**
 * For the coordination of resources, the communication between APs is performed via the MAC Layer.
 * For this, we model an Ad-Hoc network with all APs
 */
NetDeviceContainer AdHocNet(NodeContainer& Ap1,NodeContainer& Ap2){

   //std::string phyMode ("DsssRate1Mbps");
  //double rss = -80;  // -dBm
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 10;
  double interval = 1.0; // seconds
  bool verbose = false;
 
  CommandLine cmd (__FILE__);

  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);

  /* TODO: commands are currently statically set; for the next version they can be passed via the command line */
  //cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);
 
  // Fix non-unicast data rate to be the same as that of unicast
  //Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
    //                  StringValue (phyMode));

  Config::SetDefault ("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
						EnumValue (WifiAcknowledgment::DL_MU_TF_MU_BAR));
 
  NodeContainer c;
  c.Add(Ap1);
  c.Add(Ap2);
 
  // Define the channel settings, including propagation model, frequency, bandwidth, etc.
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on Wifi logging
    }
  //wifi.SetStandard (WIFI_STANDARD_80211ax_5GHZ);
  wifi.SetStandard (WIFI_STANDARD_80211ax);

 	Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();

 	Ptr<FriisPropagationLossModel> lossModel
          = CreateObject<FriisPropagationLossModel> ();
	lossModel->SetFrequency (5.180e9);
          spectrumChannel->AddPropagationLossModel (lossModel);
 	Ptr<ConstantSpeedPropagationDelayModel> delayModel
            = CreateObject<ConstantSpeedPropagationDelayModel> ();
          spectrumChannel->SetPropagationDelayModel (delayModel);

	SpectrumWifiPhyHelper phy;
	phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
	phy.SetChannel (spectrumChannel);
  
	//Set channel physical parameters through new attribute system

	TupleValue<UintegerValue, UintegerValue, EnumValue, UintegerValue> value;
	value.Set (WifiPhy::ChannelTuple {48, 20, WIFI_PHY_BAND_5GHZ, 0});
	phy.Set ("ChannelSettings", value);
	WifiMacHelper wifiMac;
	int m_mcs = 9;
  std::ostringstream oss;	
	oss << "HeMcs" << m_mcs;
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()),
							"ControlMode", StringValue (oss.str ()));



	// Activate Ad-Hoc mode
	wifiMac.SetType ("ns3::AdhocWifiMac");
	NetDeviceContainer devices = wifi.Install (phy, wifiMac, c);

	return devices;

}

/******************************************************************************************************
* The next functions handle the implementation of the co-OFDMA probabilistic approach
* 
****************************************************************************************************/

/**
* RECEIVER function: SetChanWidth handles the reception by an AP of a frame (model for an Action Frame based on raw sockets) used to 
* coordinate the share resources. It then changes the channel frequency in accordance with the received parameters
*/

void SetChanWidth(Ptr<PacketSocketServer>& server)
{

	Ptr<Packet> recv = server->GetPkt();
	MyHeader destinationHeader;
	recv->RemoveHeader (destinationHeader);
	int randpara = destinationHeader.GetData();
	NS_LOG_UNCOND("Value sent and received= "<<randpara);
	NS_LOG_UNCOND("Time is sentrx: "<<Simulator::Now().GetSeconds());

	// printf("check point\n");

 
	float thresh= 0.5;
	float result = float(randpara)/float(100);

	if (result >= thresh){	

	// Modifying channel width and number through attribute system
	// Both AP and station parameters need to be changed

	// Node 6, device 1 (AP2)		
	Config::Set ("/NodeList/6/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
             StringValue ("{40, 20, BAND_5GHZ, 0}"));

	// printf("check point0.01\n");
	

	// Node 9, device 0 (WS1)
	Config::Set ("/NodeList/9/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
             StringValue ("{40, 20, BAND_5GHZ, 0}"));
	// printf("check point0.02\n");
	Ptr<WifiPhy> wp2 = wnd2->GetPhy ();
	// printf("check point0.03\n");
	NS_LOG_UNCOND("\n Channel freq of AP2 is currently =" << wp2->GetFrequency()<<"and channelWidth = "<< wp2->GetChannelWidth());
	// printf("check point0.04\n");

}

	else if (result < thresh){

	
	Config::Set ("/NodeList/6/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
             StringValue ("{38, 40, BAND_5GHZ, 0}"));

	Config::Set ("/NodeList/9/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
             StringValue ("{38, 40, BAND_5GHZ, 0}"));
	Ptr<WifiPhy> wp2 = wnd2->GetPhy ();
	NS_LOG_UNCOND("\n Channel freq of AP2 is currently =" << wp2->GetFrequency()<<"and channelWidth = "<< wp2->GetChannelWidth());

}
	// printf("check point0.01\n");

}


/**
* SENDER function: On the initiating AP, SimValSetup is used to probabilistically select when to change channel parameters
* If the selected value is larger than a threshould then an adaptation to the channel is immediately done, and a message to neighboring
* APs is sent.
*/

void SimValSetup(NodeContainer& AP1NodeContainer,NodeContainer& AP2NodeContainer,NetDeviceContainer& ad_hoc)
{

	// Generating random number

	RngSeedManager::SetSeed (6);
	int randpara;
	int min = 1;
	int max = 100;
	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
	x->SetAttribute ("Min", DoubleValue (min));
	x->SetAttribute ("Max", DoubleValue (max));
	randpara = x->GetValue ();
	NS_LOG_UNCOND("Random value "<<randpara);
 
 // TODO: explore dynamic thresholds
 // TODO: allow the threshold value to be set via arguments on the command line.
	float thresh= 0.5;
	float result = float(randpara)/float(100);

	if (result >= thresh){
	// Modifying channel width and number through attribute system
	// Both AP and station parameters need to be changed
	
	// Node 5, device 1 (AP1)
	// Node 7, device 0 (WS3)
	// Node 8, device 0 (WS2)	
	Config::Set ("/NodeList/5/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
             StringValue ("{36, 20, BAND_5GHZ, 0}"));
	Config::Set ("/NodeList/7/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
             StringValue ("{36, 20, BAND_5GHZ, 0}"));
	Config::Set ("/NodeList/8/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
             StringValue ("{36, 20, BAND_5GHZ, 0}"));

	Ptr<WifiPhy> wp1 = wnd->GetPhy ();
	NS_LOG_UNCOND("\n Channel freq of AP1 is currently =" << wp1->GetFrequency()<<"and channelWidth = "<< wp1->GetChannelWidth());

}

	else if (result < thresh){

	Config::Set ("/NodeList/5/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
             StringValue ("{38, 40, BAND_5GHZ, 0}"));
	Config::Set ("/NodeList/7/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
            StringValue ("{38, 40, BAND_5GHZ, 0}"));
	Config::Set ("/NodeList/8/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/ChannelSettings",
             StringValue ("{38, 40, BAND_5GHZ, 0}"));

	Ptr<WifiPhy> wp1 = wnd->GetPhy ();
	NS_LOG_UNCOND("\n Channel freq of AP1 is currently =" << wp1->GetFrequency()<<"and channelWidth = "<< wp1->GetChannelWidth());

}


/**
* Creates a custom header for the frame to be sent via raw sockets from the initiating AP to the receiving AP
* Header carries the random value randpara
*/
	MyHeader test;
	MyHeader test2;
	Ptr<Packet> p = Create<Packet> (10);
	test.SetData(randpara);
	p->AddHeader (test);
	NS_LOG_UNCOND("Size of packet with header "<<p->GetSize());

	// Creating raw sockets
	PacketSocketAddress socketAddr;
	socketAddr.SetSingleDevice (ad_hoc.Get (0)->GetIfIndex ());
	socketAddr.SetPhysicalAddress (ad_hoc.Get (1)->GetAddress ());
	// Arbitrary protocol type.
	// Note: PacketSocket doesn't have any L4 multiplexing or demultiplexing
	// The only mux/demux is based on the protocol field
	socketAddr.SetProtocol (1);

	Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient> ();
	client->SetRemote (socketAddr);
	client->SetPacket(p);
 

	Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer> ();
	server->SetLocal (socketAddr);
  

	AP1NodeContainer.Get (0)->AddApplication (client);
	AP2NodeContainer.Get (0)->AddApplication (server);

	// Sends frame/packet every 10 seconds - worst case scenario, similar to Beacon interval
	// TODO: explore other intervals
	Simulator::Schedule ( Seconds (10), &SimValSetup, AP1NodeContainer, AP2NodeContainer,ad_hoc);
	//printf("\ncheck point\n");
	Simulator::Schedule ( Seconds (0.02), &SetChanWidth, server);
}


/***********************************************************************************************
* core of the simulation
***/

int 
main (int argc, char *argv[])
{

/*
* The topology is currently set in a static way, based on the TSNWiFi example
* it currently considers 2 APs, 3 wireless stations (STAs) where 1 is on an overlapping region
***/

	int simulationTime = 200;
	bool verbose = true;
	uint32_t nWifi = 3;	// Wireless endpoint nodes
	uint32_t nWire = 3; // Wired endpoint nodes
	uint32_t nSwitch = 2; // Switches
	uint32_t nAP = 2; // APs
	int mcs {9};
	std::string dlAckSeqType {"MU-BAR"};
	//double frequency {5}; //whether 2.4, 5 or 6 GHz
	//uint8_t maxChannelWidth = frequency == 2.4 ? 40 : 160;
	int channelWidth = 40;
	int gi = 800;
	bool useExtendedBlockAck {false};
	nWire = nWire == 0 ? 3 : nWire;
	int trafficNo = 1;

	//TODO next lines will be used to allow the definition of the topology via the command line. NOT YET WORKING
	CommandLine cmd;
	cmd.AddValue("nWire", "Number of wired nodes", nWire);
	cmd.AddValue("nWifi", "Number of wireless STAs", nWifi);
	cmd.AddValue("nSwitch", "Number of Switches", nSwitch);
	cmd.AddValue("nAP", "Number of Access Points", nAP);
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
	
	/*
	 * LOGGING: comment out the features you do not need. For instance, if you are not using synchronization, comment out
	 * the clock logs
	 ***/

	if (verbose) {
		
		LogComponentEnable("UdpL4Protocol", LOG_LEVEL_ALL);
		//LogComponentEnable("UdpSocketImpl", LOG_LEVEL_ALL);
		//LogComponentEnable("UdpClient", LOG_LEVEL_ALL);
		//LogComponentEnable("UdpServer", LOG_LEVEL_ALL);
		LogComponentEnable("LocalClock", LOG_LEVEL_ALL);
		LogComponentEnable("ClockModel", LOG_LEVEL_ALL);
		LogComponentEnable("PerfectClockModelImpl", LOG_LEVEL_ALL);
		LogComponentEnable("LocalTimeSimulatorImpl", LOG_LEVEL_ALL);	
		
		//LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
		//LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);	
	}
	GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
	Time sendPeriod,scheduleDuration,simulationDuration;
	simulationDuration = 2*scheduleDuration*NUMBER_OF_SCHEDULE_ENTRYS;
	sendPeriod = MilliSeconds(250);
	CallbackValue timeSource = MakeCallback(&callbackfunc);

	//Activate FTM IF you need it
	//Config::SetDefault ("ns3::RegularWifiMac::FTM_Enabled", BooleanValue(true));

	/**
	* Next, create all devices
 	*/	
	//Create devices	
	
	//TODO: dynamic topology, where setup of devices is performed via arrays
	NodeContainer wireStaNodeContainer;
	wireStaNodeContainer.Create(nWire);
	NodeContainer switchNodeContainer;
	switchNodeContainer.Create(nSwitch); 
	NodeContainer AP1NodeContainer,AP2NodeContainer; //For separate SSID config
	AP1NodeContainer.Create(1);//AP1
	AP2NodeContainer.Create(1);//AP2
	NodeContainer wifi1StaNodeContainer,wifi2StaNodeContainer;
    wifi1StaNodeContainer.Create (2);
	wifi2StaNodeContainer.Create (1);

	
	/* This scenario does not rely on the switching part. So these aspects can be commented out
	 * Reason to have it here is due to having this scenario based on the TSNWifi scenario
 	*/	
	CsmaHelper csmaX;
	csmaX.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csmaX.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (10560)));

	NetDeviceContainer switchFullLink;
	switchFullLink = csmaX.Install (switchNodeContainer);

	// Create container for switch0 and the link to switch1
	NetDeviceContainer switch0Link;
	switch0Link.Add(switchFullLink.Get(0));

	// Create links between APs and Switches. Only required for the purpose of wired to wireless communication via UDP
	CsmaHelper csmaY;
	csmaY.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csmaY.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
	NetDeviceContainer switchAPLink,switchAP2Link;
	switchAPLink = csmaY.Install (NodeContainer(switchNodeContainer.Get (0), AP1NodeContainer)); // Wired link to AP1

	switchAP2Link = csmaY.Install (NodeContainer(switchNodeContainer.Get (0), AP2NodeContainer));// Wired link to AP2
	
	// Create container for the link and devices
	NetDeviceContainer APDevices,AP2Devices;
	APDevices.Add(switchAPLink.Get(1)); //link to switch0 and AP1
	switch0Link.Add(switchAPLink.Get(0)); // link to AP1 and switch0

	AP2Devices.Add(switchAP2Link.Get(1)); //link to switch0 and AP2
	switch0Link.Add(switchAP2Link.Get(0)); // link to AP2 and switch0

	// Create the CSMA links, from each terminal to the switch1 
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
	NodeContainer AllNodeContainer =NodeContainer(wireStaNodeContainer,wifi2StaNodeContainer);
	
	int numnodes = AllNodeContainer.GetN();
	//NS_LOG_UNCOND("\nTotal number of Nodes: " << numnodes);
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
		//Ptr<Node> n = AllNodeContainer.Get (i);
		AllNodeContainer.Get (i)->AggregateObject (clock[i]);
	}

	Time init_offset_0 = Seconds (0);
	Time init_offset_1 = Seconds (100);
	Time init_offset_2 = Seconds (0);
	Time init_offset_3 = Seconds (0);

	Ptr<PerfectClockModelImpl> clockImpl0 = CreateObject <PerfectClockModelImpl> ();  /* Clock Model for AP1 */
	Ptr<PerfectClockModelImpl> clockImpl1 = CreateObject <PerfectClockModelImpl> ();  /* Clock Model for Switch */
	Ptr<PerfectClockModelImpl> clockImpl2 = CreateObject <PerfectClockModelImpl> ();  /* Clock Model for AP2 */
	Ptr<PerfectClockModelImpl> clockImpl3 = CreateObject <PerfectClockModelImpl> ();  /* Clock Model for STA1 */

	clockImpl0->SetAttribute ("Frequency", DoubleValue (freq));
	clockImpl1->SetAttribute ("Frequency", DoubleValue (freq));
	clockImpl2->SetAttribute ("Frequency", DoubleValue (freq));
	clockImpl0->SetAttribute ("Offset", TimeValue (init_offset_0));
	clockImpl1->SetAttribute ("Offset", TimeValue (init_offset_1));
	clockImpl2->SetAttribute ("Offset", TimeValue (init_offset_2));
	clockImpl3->SetAttribute ("Frequency", DoubleValue (freq));
	clockImpl3->SetAttribute ("Offset", TimeValue (init_offset_3));

	Ptr<LocalClock> clock0 = CreateObject<LocalClock> ();   /* Local Clock Objects for AP1 */
	Ptr<LocalClock> clock1 = CreateObject<LocalClock> ();	/* Local Clock Objects for Switch */
	Ptr<LocalClock> clock2 = CreateObject<LocalClock> ();   /* Local Clock Objects for AP2 */
	Ptr<LocalClock> clock3 = CreateObject<LocalClock> ();   /* Local Clock Objects for STA1 */

	clock0->SetAttribute ("ClockModel", PointerValue (clockImpl0));
	clock1->SetAttribute ("ClockModel", PointerValue (clockImpl1));
	clock2->SetAttribute ("ClockModel", PointerValue (clockImpl2));
	clock3->SetAttribute ("ClockModel", PointerValue (clockImpl3));

	/* Aggregration of Local clock to their respective nodes */
	AP1NodeContainer.Get(0)->AggregateObject (clock0);
	switchNodeContainer.Get(0)->AggregateObject (clock1);
	AP2NodeContainer.Get(0)->AggregateObject (clock2);
	wifi1StaNodeContainer.Get(0)->AggregateObject (clock3);

/********************************************************************************************
 * END OF CLOCK MODEL: only required for time sync purposes
 ********************************************************************************************/


/********************************************************************************************
 * AP and STA positioning via the ns-3 Mobility Helper
 ********************************************************************************************/
	
        MobilityHelper mobility;

		// TODO: improve the setup of distance parameters, via a more dynamic approach, e.g.:
        //int dw3,dw2,dw1;

        //dw3 = 15; //distance of w3 from AP1 (reference point AP1);
        //dw2 = 50; //distance of w2 from AP1;
        //dw1 = 90; //distance of w1 from AP1;

        Ptr<ListPositionAllocator> positionAllocAP1 = CreateObject<ListPositionAllocator> ();
        positionAllocAP1->Add (Vector (40, 0, 0)); //Position of AP1 (at 40,0,0)
        mobility.SetPositionAllocator (positionAllocAP1);
        mobility.Install(AP1NodeContainer);

        Ptr<ListPositionAllocator> positionAllocAP2 = CreateObject<ListPositionAllocator> ();
        positionAllocAP2->Add (Vector (80, 0, 0));//Position of AP2 20m from AP1
        mobility.SetPositionAllocator (positionAllocAP2);
        mobility.Install(AP2NodeContainer);

		// Code to verify distances set
        //int checkdist = mobility.GetDistanceSquaredBetween(APNodeContainer.Get(0),wifiStaNodeContainer.Get(1));
        //NS_LOG_UNCOND("\n Distance between APs =" << checkdist);


        Ptr<ListPositionAllocator> positionAllocstaw3 = CreateObject<ListPositionAllocator> ();
        positionAllocstaw3->Add (Vector (35, 0.0, 0.0)); //Position of W3, close to AP1 (served by AP1)
        mobility.SetPositionAllocator (positionAllocstaw3);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (wifi1StaNodeContainer.Get(0));

        Ptr<ListPositionAllocator> positionAllocstaw1 = CreateObject<ListPositionAllocator> ();
        positionAllocstaw1->Add (Vector (75, 0.0, 0.0)); //Position of W1, close to AP2 (served by AP2)
        mobility.SetPositionAllocator (positionAllocstaw1);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (wifi2StaNodeContainer);

        Ptr<ListPositionAllocator> positionAllocstaw2 = CreateObject<ListPositionAllocator> ();
        positionAllocstaw2->Add (Vector (60, 0.0, 0.0)); //Position of W2, betweeen the two APs
        mobility.SetPositionAllocator (positionAllocstaw2);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (wifi1StaNodeContainer.Get(1));



	/********************************************************************************************
     * Setting up of WiFi devices, SSIDs, channels, etc
     ********************************************************************************************/
	
	NetDeviceContainer ap1Devices, sta1Devices,ap2Devices,sta2Devices;
	std::string ssidname={"AP1"};//SSID of AP1
	WifiSetup wifi,wifi2;

	// Specifying channel width and channel number
	//Arguments are (AP,stations,SSID,channel index,channel number, channel width)

	wifi.ConfigureDevices(AP1NodeContainer,wifi1StaNodeContainer,ssidname,0,38,40);
	ap1Devices = wifi.GetNetDeviceContainerAP();
	sta1Devices = wifi.GetNetDeviceContainerSTA();
	ssidname={"AP2"};//SSID of AP2
	wifi2.ConfigureDevices(AP2NodeContainer,wifi2StaNodeContainer,ssidname,0,38,40);
	ap2Devices = wifi2.GetNetDeviceContainerAP();
	sta2Devices = wifi2.GetNetDeviceContainerSTA();


	//Creating ad-hoc link between APs to enable communication

	NetDeviceContainer ad_hoc = AdHocNet(AP1NodeContainer,AP2NodeContainer);
	wnd = ap1Devices.Get (0)->GetObject<WifiNetDevice> ();
	wnd2 = ap2Devices.Get (0)->GetObject<WifiNetDevice> ();

	// ======================================================================
	// Install internet stack
	// ----------------------------------------------------------------------
	InternetStackHelper stack;
	stack.Install(wireStaNodeContainer);
	stack.Install(switchNodeContainer);
	//stack.Install(APNodeContainer);	
	//stack.Install(wifiStaNodeContainer);


	/******************************************************************************
     *
    * This next part of the code is used to set up the wired transmission with TAS
    **************************************************************************************/
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

   // refer to the TSNWiFi example, traffic priorities setting up. Here, 3 Ethernet nodes have the same priority.
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


	/******************************************************************************
     *
     * Assign IPs
     **************************************************************************************/

	Ipv4AddressHelper address;

	//Assigning addresses to wired devices
	address.SetBase ("192.168.5.0", "255.255.255.0");
	Ipv4InterfaceContainer wireInterfaces; 
	wireInterfaces = address.Assign (wireDevices);
	
	// wire between AP1 and switch0
	Ipv4InterfaceContainer APInterfaces,AP2Interfaces;
	APInterfaces = address.Assign (APDevices);
	
	// wire between AP2 and switch0
	AP2Interfaces = address.Assign (AP2Devices); 
	
	
	// BSS 1 Addresses, 192.168.1.0/24, served by AP1
	Ipv4AddressHelper address1;
	address1.SetBase ("192.168.1.0", "255.255.255.0");
	Ipv4InterfaceContainer AP1WifiInterfaces,AP2WifiInterfaces; 

	// AP1
	AP1WifiInterfaces = address1.Assign (ap1Devices);
	
	// Stations
	Ipv4InterfaceContainer sta1Interfaces,sta2Interfaces;
	sta1Interfaces = address1.Assign (sta1Devices);
	
	// BSS2 IP addresses, 12.0.0.0/24. served by AP2
	address1.SetBase ("12.0.0.0", "255.255.255.0");
	
	// AP2
	AP2WifiInterfaces=address1.Assign (ap2Devices);

	// Stations
	sta2Interfaces=address1.Assign (sta2Devices); 
	
	
	
	
	/******************************************************************************************
      * Set up the bridging on switches
      ****************************************************************************************/
	BridgeHelper bridge;
	
	NetDeviceContainer switch0DeviceContainer;
	NetDeviceContainer switch1DeviceContainer;
	
	switch0DeviceContainer = bridge.Install(switchNodeContainer.Get(0), switch0Link);
	switch1DeviceContainer = bridge.Install(switchNodeContainer.Get(1), switch1Link);
	
	Ipv4InterfaceContainer switchIpInterfaces;
	switchIpInterfaces = address.Assign(switch0DeviceContainer);	


	// ======================================================================
	// Iperf Setup correspond to iperf-setup.cc in DCE
	// ----------------------------------------------------------------------
	/**
	 * @brief DL WifiSTA0 <--- WireSTA0 
	 * 
	 */
	/*
	int runTime = 2;
	trafficNo = 1;

	Time StartTime = Seconds(1);
	Time StopTime = Seconds(4);
	ClientSetup client;
	ServerSetup server;
	client.install(wireStaNodeContainer.Get(0), staInterfaces.Get (0), runTime, trafficNo, StartTime, StopTime);
	server.install(wifiStaNodeContainer.Get(0));
	*/
	
	/* Socket Creation and adding Callback function to send 
	 * and receive time from switch to AP */
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	Ptr<Socket> recvSink = Socket::CreateSocket (AP1NodeContainer.Get(0), tid);
	InetSocketAddress local = InetSocketAddress (APInterfaces.GetAddress (0), 80);
	recvSink->Bind (local);
	recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
	AP1NodeContainer.Get(0)->AggregateObject (recvSink);

	Ptr<Socket> source = Socket::CreateSocket (switchNodeContainer.Get(0), tid);
	switchNodeContainer.Get(0)->AggregateObject (source);
	InetSocketAddress remote = InetSocketAddress (Ipv4Address::GetAny (), 80);
	source->Connect (remote);
	Ptr<Ipv4> ipv4get = switchNodeContainer.Get(0)->GetObject<Ipv4> ();

	/* FTM - Setting FTM params to start a session 
	 * NOT REQUIRED if you are not using FTM
	 */
	Ptr<NetDevice> ap_ftm = ap1Devices.Get(0);
	Ptr<NetDevice> sta_ftm = sta1Devices.Get(0);
	Address recvAddr_ftm = ap_ftm->GetAddress();
	
	// //convert net device to wifi net device
	Ptr<WifiNetDevice> wifi_ap_ftm = ap_ftm->GetObject<WifiNetDevice>();
	Ptr<WifiNetDevice> wifi_sta_ftm = sta_ftm->GetObject<WifiNetDevice>();	

	/* FTM - Setting FTM params to start a session 
	 * Setting default FTM parameters - can be commented out if not using FTM
	 */
	
	FtmParams ftm_params;
	ftm_params.SetNumberOfBurstsExponent(2); //2 bursts
	ftm_params.SetBurstDuration(7); //8 ms burst duration, this needs to be larger due to long processing delay until transmission

	ftm_params.SetMinDeltaFtm(10); //1000 us between frames
	ftm_params.SetPartialTsfNoPref(true);
	ftm_params.SetAsap(true);
	ftm_params.SetFtmsPerBurst(2);
	ftm_params.SetBurstPeriod(10); //1000 ms between burst periods

	// /* Scheduling of FTM session */
	Simulator::Schedule (Seconds(8),&GenerateTraffic, wifi_ap_ftm, wifi_sta_ftm, recvAddr_ftm, ftm_params, clock0, clock3);

	// //FTM: set time resolution to pico seconds for the time stamps, as default is in nano seconds. IMPORTANT
	Time::SetResolution(Time::PS);
	Simulator::Stop (Seconds (simulationTime));

/******************************************************************************************
 * UDP Flow setup
 ****************************************************************************************/	

	ApplicationContainer serverApp1;
	UdpServerHelper server1 (9);///checking with 7
	serverApp1 = server1.Install (AP1NodeContainer.Get (0));
	serverApp1.Start (Seconds (0));
	serverApp1.Stop (Seconds (simulationTime + 1));
	NS_LOG_UNCOND("\nUDP server ");

  // Setting up multiple flows from client to same server
  int flows = 10;
  for (int i =0;i<flows;i++){
  UDPFlows(AP1WifiInterfaces,wifi1StaNodeContainer);
	}


	/******************************************************************************************
      * Switch trace application
      ****************************************************************************************/
	Ptr<SwitchTraceApplication> trace1 = CreateObject<SwitchTraceApplication>();
	trace1->SetNetDevice(switch1DeviceContainer.Get(0));
	trace1->SetStartTime (Seconds (15.2));
    	//trace1->SetStopTime (Seconds (simulationTime));
	//switchNodeContainer.Get(1)->AddApplication(trace1);
 
	Ptr<SwitchTraceApplication> trace2 = CreateObject<SwitchTraceApplication>();
	trace2->SetNetDevice(switch0DeviceContainer.Get(0));
	trace2->SetStartTime (Seconds (15.2));
    	//trace2->SetStopTime (Seconds (simulationTime));
	//switchNodeContainer.Get(0)->AddApplication(trace2);

		
	Simulator::Schedule (Seconds (0), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);
	
	FlowMonitorHelper flowmon;
  	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	Simulator::Stop (Seconds (simulationTime));
	
	/*Schedule the sending of time from switch to AP */
	for (int z = 0; z < 1 ; z += 1)
	{
		Simulator::ScheduleWithContext (0, Seconds (z), &sendtime, source, AP1NodeContainer.Get(0)->GetObject<Ipv4>()->GetAddress (1,0).GetLocal(), 80, clock0, clock1);
	}
	
	/* Schedule the printing of local clock values of AP, STA and Switch */
	for (int iz = 0; iz < simulationTime; iz += 1)
        {
                Simulator::ScheduleWithContext (0, Seconds (iz), &printTimes, clock0, clock3, clock1);
	}




	/**
	 * Saving output as .pcap file
	*/
	//csma.EnablePcap ("TSNWifi-wire", APDevices.Get (0), true);
	//csma.EnablePcap ("TSNWifi-wire", wireDevices.Get (0), true);
	//csma.EnablePcap ("TSNWifi-Switch1-AP", switchFullLink.Get (1), true);
	//csma.EnablePcap ("TSNWifi-Switch1-Node0", switch1Link.Get (0), false);
	//csma.EnablePcap ("TSNWifi-Switch1-Switch0", switch1Link.Get (3), true);
	//csma.EnablePcap ("TSNWifi-Switch0-Switch1", switchFullLink.Get (0), false);
Simulator::Schedule(Seconds (3), &SimValSetup, AP1NodeContainer, AP2NodeContainer, ad_hoc);
Simulator::Run ();
	
	// ======================================================================
	// Additional information such as delay, jitter and packet loss etc.
	// ----------------------------------------------------------------------


	uint32_t SentPackets = 0;
	uint32_t ReceivedPackets = 0;
	uint32_t LostPackets = 0;
	int j=0;
	float AvgThroughput = 0;
	float packetloss;
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
	packetloss = float((LostPackets*100))/float(SentPackets);
	
	
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


	// Writing output to text file	
	AsciiTraceHelper asciiTraceHelper;
	std::stringstream fname_exp;
	std::string pktsize,numflows,datarate;

	// Below parameters are based on traffic profiles
	// Packet size
	pktsize = "small";

	// Number of flows
	numflows = "1flow";

	// Data Rate
	datarate= "low";

  fname_exp<< pktsize << numflows <<datarate<< ".txt";
  Ptr <OutputStreamWrapper> outputstats = asciiTraceHelper.CreateFileStream(fname_exp.str());

 *outputstats->GetStream() <<"Total sent packets= " << SentPackets<<"\n"<<"Total Received Packets =" << ReceivedPackets<<"\n" "Average Throughput(kbps)= " << AvgThroughput <<"\n"<<"Delay(ns)= "<<Delay << "\n" << "Jitter(ns)= "<<Jitter<<"\n"<<"Packet Loss(%)= "<<packetloss;



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

// in src/tsnwifi/helper/ftm_sync.cc in 3.34
void sendtime(Ptr<Socket> socket, Ipv4Address &ipv4address, uint32_t port, Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_1)
{
	TimestampTag timestamp;
        Ptr<Packet> packet = Create<Packet>(1024);
        timestamp.SetTimestamp (clock_1->GetLocalTime());
        packet->AddByteTag (timestamp);
	/* Send the time of switch to AP as a byte tag*/
        socket->SendTo (packet, 0, InetSocketAddress (ipv4address, port));
}

// in src/tsnwifi/helper/ftm_sync.cc in 3.34
void printTimes(Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_3, Ptr<LocalClock> clock_1)
{
        NS_LOG_UNCOND("*****Print AP and STA Time*********");
        NS_LOG_UNCOND(" AP " << clock_0->GetLocalTime().GetPicoSeconds() << " STA " << clock_3->GetLocalTime().GetPicoSeconds() << " diff " << clock_0->GetLocalTime() - clock_3->GetLocalTime() << " Switch " << clock_1->GetLocalTime().GetPicoSeconds());
        NS_LOG_UNCOND("*********************************");

}

// in src/tsnwifi/helper/ftm_sync.cc in 3.34
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

