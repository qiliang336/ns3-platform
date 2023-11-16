/*
 * Copyright (c) 2020, 2021, 2022 fortiss GmbH
 * @author: Sugandh Huthanally Mohan (huthanally@fortiss.org) 
 *
  * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 */

/*
 * Provides an example on how to use FTM for 1 AP and 1 STA
 */

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/ftm-header.h"
#include "ns3/mgt-headers.h"
#include "ns3/ftm-error-model.h"
#include "ns3/pointer.h"
#include "ns3/ftm-session.h"

#include "ns3/regular-wifi-mac.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FtmExample");

int rss;
void SessionOver (FtmSession session)
{
printf("RTT: %ld\n",session.GetMeanRTT ());  
//NS_LOG_UNCOND ("RTT: " << session.GetMeanRTT ());
}

void OffsetManager (Ptr<FtmSession::FtmDialog> dialog, FtmSession session)
{
printf("OffsetMgmt \n");
NS_LOG_UNCOND("dialog->t1 " << dialog->t1);
}


Ptr<WirelessFtmErrorModel::FtmMap> map;

static void GenerateTraffic (Ptr<WifiNetDevice> ap, Ptr<WifiNetDevice> sta, Address recvAddr, FtmParams ftm_params)
{

  Ptr<RegularWifiMac> sta_mac = sta->GetMac()->GetObject<RegularWifiMac>();

  Mac48Address to = Mac48Address::ConvertFrom (recvAddr);

  Ptr<FtmSession> session = sta_mac->NewFtmSession(to);
  if (session == 0)
    {
      NS_FATAL_ERROR ("ftm not enabled");
    }

  //create the wired error model
  //Ptr<WiredFtmErrorModel> wired_error = CreateObject<WiredFtmErrorModel> ();
  //wired_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);

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

  session->SetSessionOverCallback(MakeCallback(&SessionOver));
  session->SetOffsetCorrectionCallback(MakeCallback(&OffsetManager));
  session->SessionBegin();
}

int main (int argc, char *argv[])
{
  double rss = -80;  // -dBm
  int ftms=0;
  float burst_exponent=0;
  bool asap=false;
  int burst_duration=0;
  int burst_period=0;
  int dist=0;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("dist", "Distance between nodes", dist);
  cmd.AddValue ("ftms", "Number of FTMs", ftms);
  cmd.AddValue ("burst_exponent", "Number of bursts exponent", burst_exponent);
  cmd.AddValue ("asap", "asap value", asap);
  cmd.AddValue ("burst_duration", "Burst Duration", burst_duration);
  cmd.AddValue ("burst_period", "Burst Period", burst_period);
  cmd.Parse (argc, argv);


   NS_LOG_UNCOND("ftms " << ftms << " exp " << burst_exponent << " asap " << asap << " period " << burst_period << " duration " << burst_duration);
  //enable FTM through attribute system
  Config::SetDefault ("ns3::RegularWifiMac::FTM_Enabled", BooleanValue(true));

  NodeContainer c;
  c.Create (2);

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211n_2_4GHZ);

  YansWifiPhyHelper wifiPhy;
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");

  // Setup the rest of the mac
  Ssid ssid = Ssid ("wifi-default");
  // setup sta.
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid));
//  wifiMac.SetType ("ns3::StaWifiMac");
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, c.Get (0));
  NetDeviceContainer devices = staDevice;
  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, c.Get (1));
  devices.Add (apDevice);

  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (2.0, 0, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install(c.Get (0));

  Ptr<ListPositionAllocator> positionAlloc2 = CreateObject<ListPositionAllocator> ();
  positionAlloc2->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc2);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c.Get(1));

  Ptr<NetDevice> ap = apDevice.Get(0);
  Ptr<NetDevice> sta = staDevice.Get(0);
  Address recvAddr = ap->GetAddress();

  //convert net device to wifi net device
  Ptr<WifiNetDevice> wifi_ap = ap->GetObject<WifiNetDevice>();
  Ptr<WifiNetDevice> wifi_sta = sta->GetObject<WifiNetDevice>();
  //enable FTM through the MAC object
//  Ptr<RegularWifiMac> ap_mac = wifi_ap->GetMac()->GetObject<RegularWifiMac>();
//  Ptr<RegularWifiMac> sta_mac = wifi_sta->GetMac()->GetObject<RegularWifiMac>();
//  ap_mac->EnableFtm();
//  sta_mac->EnableFtm();


  // Tracing
  wifiPhy.EnablePcap ("ftm-example", devices);

  //set the default FTM params through the attribute system
  FtmParams ftm_params;
  ftm_params.SetNumberOfBurstsExponent(burst_exponent); //2 bursts
  ftm_params.SetBurstDuration(9); //8 ms burst duration, this needs to be larger due to long processing delay until transmission

  ftm_params.SetMinDeltaFtm(10); //1000 us between frames
  ftm_params.SetPartialTsfNoPref(true);
  ftm_params.SetAsap(true);
  ftm_params.SetFtmsPerBurst(ftms);
  ftm_params.SetBurstPeriod(10); //1000 ms between burst periods
  Simulator::ScheduleNow (&GenerateTraffic, wifi_ap, wifi_sta, recvAddr, ftm_params);
  //set time resolution to pico seconds for the time stamps, as default is in nano seconds. IMPORTANT
  Time::SetResolution(Time::PS);

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
