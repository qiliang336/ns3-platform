/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Chitiphat Chongaraemsang, fortiss GmbH
* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * ns-3 application to model TWT Action Frames
*/
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/send-action-frame-application.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/mgt-headers.h"

#include "ns3/regular-wifi-mac.h"

#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define END_CODE "\033[0m"

/**
 * For now the unit of all the parameters is Millisecond.
*/


namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("SendActionFrameApplication");
  NS_OBJECT_ENSURE_REGISTERED(SendActionFrameApplication);

TypeId SendActionFrameApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SendActionFrameApplication")
                .SetParent <Application> ()
                .AddConstructor<SendActionFrameApplication> ()
                .AddAttribute ("ID", "Id of the station that will have this Service period",
                      UintegerValue (0),
                      MakeUintegerAccessor (&SendActionFrameApplication::m_sta_id),
                      MakeTimeChecker()
                      )
                .AddAttribute ("Interval", "Cycle Interval",
                      UintegerValue (100),
                      MakeUintegerAccessor (&SendActionFrameApplication::m_interval_time),
                      MakeTimeChecker()
                      )
                .AddAttribute ("Duration", "Duration of the Wake period",
                      UintegerValue (100),
                      MakeUintegerAccessor (&SendActionFrameApplication::m_duration_time),
                      MakeTimeChecker()
                      )
                .AddAttribute ("Wake", "Wake time in a cycle",
                      UintegerValue (0),
                      MakeUintegerAccessor (&SendActionFrameApplication::m_wake_time),
                      MakeUintegerChecker<uint32_t> ()
                      )
                .AddAttribute ("Frequency", "Action Frames sending frequency.",
                      TimeValue (MilliSeconds(100)),
                      MakeTimeAccessor (&SendActionFrameApplication::m_frequency),
                      MakeTimeChecker()
                      )
                ;
    return tid;
}

TypeId SendActionFrameApplication::GetInstanceTypeId() const
{
    return SendActionFrameApplication::GetTypeId();
}

SendActionFrameApplication::SendActionFrameApplication()
{
    /**Initialized default values*/
    m_sta_id = 0;
    m_sp_id = 0;
    m_interval_time = 100; //every 100ms
    m_duration_time = 100;
    m_wake_time = 0;
    m_frequency = MilliSeconds (100);
}
SendActionFrameApplication::~SendActionFrameApplication()
{

}
void
SendActionFrameApplication::StartApplication()
{
    NS_LOG_FUNCTION (this);

    Ptr<Node> n = GetNode ();
    //the node might have more than one netdevices, such as, access point
    for (uint32_t i = 0; i < n->GetNDevices (); i++)
    {
        
        Ptr<NetDevice> dev = n->GetDevice (i);
        //check if the netdevice is a wifinetdevice
        if (dev->GetInstanceTypeId () == WifiNetDevice::GetTypeId())
        {
            m_wifiDevice = DynamicCast <WifiNetDevice> (dev);
            break;
        } 
    }
    if (!m_wifiDevice)
    {
        NS_FATAL_ERROR ("There's no WifiNetDevice in your node");
    }
    Ptr<WifiPhy> phy = m_wifiDevice->GetPhy();
    Ptr<WifiMac> mac = m_wifiDevice->GetMac();
    
    if (mac->GetTypeId () == WifiMac::GetTypeId())
    {
        m_wifiMac = DynamicCast <RegularWifiMac> (mac);
    } 
    else
    {
        NS_FATAL_ERROR ("There's no RegularWifiMac in your node");
    }
    
    //m_wifiMac->SetWifiPhy(phy);
    m_txop = m_wifiMac->GetTxop();
    ConstructActionFrame();
}

void
SendActionFrameApplication::StopApplication()
{
    NS_LOG_FUNCTION (this);
    //m_Event.Cancel ();
    Simulator::Cancel (m_Event);
}

void 
SendActionFrameApplication::ConstructActionFrame()
{
    //NS_LOG_FUNCTION (this);

    WifiMacHeader hdr;
    hdr.SetType (WIFI_MAC_MGT_ACTION);
    hdr.SetAddr1 (Mac48Address::GetBroadcast ()); //to all the stations
    hdr.SetAddr2 (m_wifiMac->GetAddress ()); 
    hdr.SetAddr3 (m_wifiMac->GetBssid (0)); 
    hdr.SetDsNotFrom (); 
    hdr.SetDsNotTo ();
    
    //VendorSpecificActionHeader vsa;
    //vsa.SetOrganizationIdentifier (oi);
    //vsc->AddHeader (vsa);

    WifiActionHeader actionHdr;
    WifiActionHeader::ActionValue action;
    action.vsa = WifiActionHeader::TARGET_WAKE_TIME_REQUEST;
    actionHdr.SetAction (WifiActionHeader::VENDOR_SPECIFIC_ACTION, action);
    
    Ptr<Packet> packet = Create<Packet> ();
    
    MgtTWTReqHeader reqHdr;
    //reqHdr.SetTid (2);
    //reqHdr.SetBufferSize (0);
    reqHdr.SetStaID(m_sta_id);

    reqHdr.SetSpID(m_sp_id);
    
    reqHdr.SetWakeTime(m_wake_time);
    
    reqHdr.SetWakeDuration(m_duration_time);
    
    reqHdr.SetWakeInterval(m_interval_time);
    
    //NS_LOG_FUNCTION (this << reqHdr.GetWakeTime());
    packet->AddHeader (reqHdr);
    packet->AddHeader (actionHdr);
    
    NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " from AP: " << m_wifiMac->GetAddress () << ", Action Frame Send.");
    
    Simulator::Schedule (m_frequency+MilliSeconds(m_wake_time), &SendActionFrameApplication::SendActionFrame, this, packet, hdr);
}

void 
SendActionFrameApplication::SendActionFrame(Ptr<Packet> packet, WifiMacHeader hdr)
{
    NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " from AP: " << m_wifiMac->GetAddress () << ", Action Frame Send.");
    m_txop->Queue (packet, hdr);
    m_Event = Simulator::Schedule (m_frequency, &SendActionFrameApplication::SendActionFrame, this, packet, hdr);
}

void 
SendActionFrameApplication::SetStaID (uint16_t id)
{
    NS_LOG_FUNCTION (this << id);
    m_sta_id = id;
}

void 
SendActionFrameApplication::SetSpID (uint16_t id)
{
    NS_LOG_FUNCTION (this << id);
    m_sp_id = id;
}

void 
SendActionFrameApplication::SetWakeInterval (uint16_t interval)
{
    NS_LOG_FUNCTION (this << interval);
    m_interval_time = interval;
}
void 
SendActionFrameApplication::SetWakeDuration (uint16_t duration)
{
    NS_LOG_FUNCTION (this << duration);
    if(duration <= m_interval_time){
        m_duration_time = duration;
    }
    else{
        NS_LOG_INFO("Duration " << duration << " Interval " << m_interval_time);
        NS_FATAL_ERROR ("Wake Duration should not exceed Wake Interval");
    }
}
void 
SendActionFrameApplication::SetWakeTime (uint16_t wake)
{
    NS_LOG_FUNCTION (this << wake);
    m_wake_time = wake;
}

void 
SendActionFrameApplication::SetFrequency (Time freq)
{
    NS_LOG_FUNCTION(this << freq);
    m_frequency = freq;
}


}//end of ns3

