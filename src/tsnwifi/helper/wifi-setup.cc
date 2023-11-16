/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Sugandh Huthanahally Mohan – huthanahally@fortiss.org
 * @author Ahmed Ansari, fortiss GmbH
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * 
 */



#include "wifi-setup.h"
#include "ns3/internet-module.h"
#include "ns3/ssid.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/wifi-module.h"


namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("WifiSetup");

WifiSetup::WifiSetup(){}
WifiSetup::~WifiSetup () {}

void WifiSetup::ConfigureDevices (NodeContainer& ap, NodeContainer& sta, std::string ssid_text, int index,int channelnumber, int channelwidth)
{
	std::string dlAckSeqType {"MU-BAR"};

	if (dlAckSeqType == "ACK-SU-FORMAT")
	{
	Config::SetDefault ("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
						EnumValue (WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE));
	}
   	else if (dlAckSeqType == "MU-BAR")
	{
	Config::SetDefault ("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
						EnumValue (WifiAcknowledgment::DL_MU_TF_MU_BAR));
	}
   	else if (dlAckSeqType == "AGGR-MU-BAR")
	{
	Config::SetDefault ("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
						EnumValue (WifiAcknowledgment::DL_MU_AGGREGATE_TF));
	}
	NetDeviceContainer apDevices, staDevices;
	WifiHelper wifi;
	//wifi.SetStandard (WIFI_STANDARD_80211ax_5GHZ);
	wifi.SetStandard (WIFI_STANDARD_80211ax);

	//printf("check point 0.3\n");

	std::ostringstream oss;	
	oss << "HeMcs" << m_mcs;
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()),
								"ControlMode", StringValue (oss.str ()));
	
	//printf("check point 0.4\n");

	WifiMacHelper mac;
	//To specify different SSIDs
	Ssid ssid = Ssid (ssid_text); 
	Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
	SpectrumWifiPhyHelper phy;
	phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

	//Defining Propagation loss model explicitly

	//printf("check point 0.5\n");

	Ptr<FriisPropagationLossModel> lossModel
          = CreateObject<FriisPropagationLossModel> ();
	lossModel->SetFrequency (5.180e9);
          spectrumChannel->AddPropagationLossModel (lossModel);
 	Ptr<ConstantSpeedPropagationDelayModel> delayModel
            = CreateObject<ConstantSpeedPropagationDelayModel> ();
  spectrumChannel->SetPropagationDelayModel (delayModel);


	phy.SetChannel (spectrumChannel);

	mac.SetType ("ns3::StaWifiMac", "MaxMissedBeacons", UintegerValue (10000),
				"Ssid", SsidValue (ssid));

	//printf("check point 0.6\n");
	
	//Using Tuple to define channel parameters to avoid conflict

	TupleValue<UintegerValue, UintegerValue, EnumValue, UintegerValue> value;
	value.Set (WifiPhy::ChannelTuple {channelnumber, channelwidth, WIFI_PHY_BAND_5GHZ,index}); //channel number,width,band
	
	phy.Set ("ChannelSettings", value);

		
	//else if (m_frequency == 2.4)
		//wifi.SetStandard (WIFI_STANDARD_80211ax_2_4GHZ);

	
	//printf("check point 0.7\n");


	
	staDevices = wifi.Install (phy, mac, sta);

	//printf("check point 0.8\n");

	// Disable A-MPDU
	//Ptr<NetDevice> dev = sta.Get (0)->GetDevice (0);
	//Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice> (dev);
	//wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));

	m_netDeviceContainerSTA = staDevices;
	//phy.EnablePcap ("Station", m_netDeviceContainerSTA.Get (0));

	mac.SetMultiUserScheduler ("ns3::RrMultiUserScheduler",
							"EnableUlOfdma", BooleanValue (true),
							"EnableBsrp", BooleanValue (true),
							"UseCentral26TonesRus", BooleanValue (true));

	//printf("check point 0.9\n");
	
	mac.SetType ("ns3::ApWifiMac",
				"EnableBeaconJitter", BooleanValue (false),
				"Ssid", SsidValue (ssid));

	apDevices = wifi.Install (phy, mac, ap);

	//printf("check point 0.10\n");

	m_netDeviceContainerAP = apDevices;
	//phy.EnablePcap ("AccessPoint", m_netDeviceContainerAP.Get (0));
	//RngSeedManager::SetSeed (1);
	//RngSeedManager::SetRun (1);
	//int64_t streamNumber = 100;
	//streamNumber += wifi.AssignStreams (apDevices, streamNumber);
	//streamNumber += wifi.AssignStreams (staDevices, streamNumber);

		// Set guard interval and MPDU buffer size
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval", TimeValue (NanoSeconds (m_gi)));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/MpduBufferSize", UintegerValue (256)); //m_useExtendedBlockAck ? 256 : 64

	//printf("check point 0.11\n");

	InternetStackHelper stack;
	stack.Install(ap);	
	stack.Install(sta);
	//printf("check point 0.12\n");
}

NetDeviceContainer WifiSetup::GetNetDeviceContainerAP()
{
	return m_netDeviceContainerAP;
}
NetDeviceContainer WifiSetup::GetNetDeviceContainerSTA()
{
	return m_netDeviceContainerSTA;
}

NodeContainer WifiSetup::GetNodeContainer()
{
	return m_nodeContainer;
}

void WifiSetup::SetChannelWidth(int width)
{
	if(width > m_maxChannelWidth)
	{
		m_channelWidth = m_maxChannelWidth;
	}
	else
	{
		m_channelWidth = width;
	}
}
void WifiSetup::SetMCS(int mcs)
{
	m_mcs = mcs;
}

void WifiSetup::SetGuardInterval(int gi)
{
	m_gi = gi;
}

void WifiSetup::SetExtendedBlockAck(bool use)
{
	m_useExtendedBlockAck = use;
}

void WifiSetup::SetFrequency(double freq)
{
	m_frequency = freq;

	m_maxChannelWidth = m_frequency == 2.4 ? 40 : 160;
}
// TypeId
// TimestampTag::GetTypeId (void)
// {
//   static TypeId tid = TypeId ("TimestampTag")
//     .SetParent<Tag> ()
//     .AddConstructor<TimestampTag> ()
//     .AddAttribute ("Timestamp",
//                    "Some momentous point in time!",
//                    EmptyAttributeValue (),
//                    MakeTimeAccessor (&TimestampTag::GetTimestamp),
//                    MakeTimeChecker ())
//   ;
//   return tid;
// }
// TypeId
// TimestampTag::GetInstanceTypeId (void) const
// {
//   return GetTypeId ();
// }

// uint32_t
// TimestampTag::GetSerializedSize (void) const
// {
//   return 8;
// }
// void
// TimestampTag::Serialize (TagBuffer i) const
// {
//   int64_t t = m_timestamp.GetNanoSeconds ();
//   i.Write ((const uint8_t *)&t, 8);
// }
// void
// TimestampTag::Deserialize (TagBuffer i)
// {
//   int64_t t;
//   i.Read ((uint8_t *)&t, 8);
//   m_timestamp = NanoSeconds (t);
// }


// void
// TimestampTag::SetTimestamp (Time time)
// {
//   m_timestamp = time;
// }
// Time
// TimestampTag::GetTimestamp (void) const
// {
//   return m_timestamp;
// }

// void
// TimestampTag::Print (std::ostream &os) const
// {
//   os << "t=" << m_timestamp;
// }



}
