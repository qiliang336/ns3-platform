/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Chitiphat Chongaraemsang, fortiss GmbH
* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * ns-3 application t to model the TWT sleep-awake cycle
*/

#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/sleep-cycle-application.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/mac-rx-middle.h"
#include "ns3/regular-wifi-mac.h"
#include "ns3/mgt-headers.h"

#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define END_CODE "\033[0m"

/**
 * Wifiphy always start with sleep. 
 * If you want the wifiphy to awake at the beginning of the cycle, use waketime = 0. 
 * If you want the wifiphy to sleep at the beginning of the cycle, use waketime != 0.
 * 
 * Example: WakeInterval = 100ms, WakeDuration = 90ms, WakeTime = 10ms
 * 0ms     10ms                 100ms      110ms                     200ms
 * |--------|---------------------|---------|-------------------------|
 *   Sleep          Awake            Sleep            Awake
*/

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("SleepCycleApplication");
  NS_OBJECT_ENSURE_REGISTERED(SleepCycleApplication);

TypeId SleepCycleApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SleepCycleApplication")
                .SetParent <Application> ()
                .AddConstructor<SleepCycleApplication> ()
/*                .AddAttribute ("ID", "Id of the station that will have this Service period",
                      UintegerValue (0),
                      MakeUintegerAccessor (&SleepCycleApplication::m_sta_id),
                      MakeTimeChecker()
                      )
                .AddAttribute ("Interval", "Cycle Interval",
                      TimeValue (MilliSeconds(100)),
                      MakeTimeAccessor (&SleepCycleApplication::m_interval_time),
                      MakeTimeChecker()
                      )
                .AddAttribute ("Offset", "Offset of simulation's start time",
                      TimeValue (MilliSeconds(0)),
                      MakeTimeAccessor (&SleepCycleApplication::m_duration_time),
                      MakeTimeChecker()
                      )
                .AddAttribute ("Sleep", "Sleep time in a cycle",
                      TimeValue (MilliSeconds(0)),
                      MakeTimeAccessor (&SleepCycleApplication::m_wake_time),
                      MakeTimeChecker()
                      )*/
                ;
    return tid;
}

TypeId SleepCycleApplication::GetInstanceTypeId() const
{
    return SleepCycleApplication::GetTypeId();
}

SleepCycleApplication::SleepCycleApplication()
    : m_sendEvent (),
      m_sta_id (0)
      
{
    /**Initialized default values*/
    //m_interval_time[0] = MilliSeconds (100); //every 100ms
    //m_duration_time[0] = Seconds(0);
    //m_wake_time[0] = Seconds(0);
}
SleepCycleApplication::~SleepCycleApplication()
{

}
void
SleepCycleApplication::StartApplication()
{
    NS_LOG_FUNCTION (this);
    
    Ptr<Node> n = GetNode ();

    //This loop is for the multiple devices that will have the same sleep cycle.
    for (uint32_t i = 0; i < n->GetNDevices (); i++)
    {
        
        Ptr<NetDevice> dev = n->GetDevice (i);

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
    //Set callback from mac layer for action frame
    m_wifiMac->SetVSACallback(MakeCallback (&SleepCycleApplication::Receive, this));
          
}
void
SleepCycleApplication::StopApplication()
{
    NS_LOG_FUNCTION (this);
    //m_sendEvent.Cancel ();
    //Simulator::Cancel (m_sendEvent);
}

void 
SleepCycleApplication::Receive(Ptr<WifiMacQueueItem> mpdu)
{
    NS_LOG_FUNCTION(this << "At time " << Simulator::Now ().As (Time::S));
    m_activated = true;
    const WifiMacHeader* hdr = &mpdu->GetHeader ();
    Ptr<Packet> packet = mpdu->GetPacket ()->Copy ();
    
    Mac48Address from = hdr->GetAddr2 ();
    MgtTWTReqHeader reqHdr;
    packet->PeekHeader (reqHdr);
    NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " Station " << m_sta_id << " received action frame from: " << from << " For Sta's ID: " << reqHdr.GetStaID());
    //NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " From:" << from << " WakeTime: " << waketime << " WakeDuration: " << wakeduration << " WakeInterval: " << wakeinterval);
    uint16_t sp_id = reqHdr.GetSpID();
    
    NS_LOG_INFO ("Content:" << " Service Period ID: " << sp_id << " WakeTime: " << reqHdr.GetWakeTime() << " WakeDuration: " << reqHdr.GetWakeDuration() << " WakeInterval: " << reqHdr.GetWakeInterval());
    uint16_t waketime = reqHdr.GetWakeTime();
    uint16_t wakeinterval = reqHdr.GetWakeInterval();
    uint16_t wakeduration = reqHdr.GetWakeDuration();

    /*Check the changes in service period parameters*/
    if(m_running[sp_id] == true && m_sta_id == reqHdr.GetStaID()){
        if(MilliSeconds(waketime) != m_wake_time[sp_id] || MilliSeconds(wakeinterval) != m_interval_time[sp_id] || MilliSeconds(wakeduration) != m_duration_time[sp_id]){
            m_running[sp_id] = false;
            m_sendEvent[sp_id].Cancel();
            NS_LOG_INFO("...Changes in service id " << sp_id << " m_running[sp_id] " << m_running[sp_id]);
        }
    }
    //NS_LOG_INFO("...Changes in service id " << sp_id << " m_running[sp_id] " << m_running[sp_id] << " m_sta_id " << reqHdr.GetStaID());
    if(m_activated == true && m_running[sp_id] == false && m_sta_id == reqHdr.GetStaID()){
        
        SetWakeTime(MilliSeconds(waketime), sp_id);
        SetWakeInterval(MilliSeconds(wakeinterval), sp_id);
        SetWakeDuration(MilliSeconds(wakeduration), sp_id);

        Time now = Simulator::Now (); //This is in nanosecond
        int64_t time = now.GetMicroSeconds();
        int64_t remain = 100*1000 - time%(100*1000); //remaining time to be full 100 millisecond, wakeinterval=100ms

        if(wakeinterval > 100){
            remain = 1000*1000 - time%(1000*1000);
        }

        if(m_remain == false){
            Simulator::Schedule (MicroSeconds(remain), &SleepCycleApplication::SetWifiPhySleep, this, sp_id);
            m_running[sp_id] = true;
            NS_LOG_INFO("Time " << time << " Wake Interval " << wakeinterval << " Remain " << remain);
            Simulator::Schedule (m_wake_time[sp_id] + MicroSeconds(remain), &SleepCycleApplication::SetWifiPhySleepResume, this, sp_id);
        }
        else if (m_running[sp_id] == false && m_remain == true){
            NS_LOG_INFO("Time " << time << " Wake Interval " << wakeinterval << " Remain " << remain);
            m_running[sp_id] = true;
            Simulator::Schedule (m_wake_time[sp_id] + MicroSeconds(remain), &SleepCycleApplication::SetWifiPhySleepResume, this, sp_id);
        }
    }
    return; 
}

void
SleepCycleApplication::SetWifiPhySleepResume(int sp)
{
    NS_LOG_FUNCTION (this);
    
    if(m_running[sp] == true){
        Simulator::Schedule (Seconds(0), &SleepCycleApplication::SetWifiPhyResume, this, sp);
        Simulator::Schedule (m_duration_time[sp], &SleepCycleApplication::SetWifiPhySleep, this, sp);
        m_sendEvent[sp] = Simulator::Schedule (m_interval_time[sp], &SleepCycleApplication::SetWifiPhySleepResume, this, sp);
    } 
}

void
SleepCycleApplication::SetWifiPhySleep(int sp)
{   if(m_remain == false)
        m_remain = true;
    NS_LOG_FUNCTION (this);
    Ptr<WifiPhy> phy = m_wifiDevice->GetPhy();
    //Address myMacAddress = m_wifiDevice->GetAddress();
    if(!phy->IsStateSleep()){
    //if(!phy->IsStateOff()){
        phy->SetSleepMode();
        //phy->SetOffMode();
        NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " station " << m_sta_id << " with service id " <<
                    sp << " WifiPhy Sleep--------------------------------------------------------------");
    }
    else{
        NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " already in sleep state... ");
    }
}

void
SleepCycleApplication::SetWifiPhyResume(int sp)
{
    NS_LOG_FUNCTION (this);

    Time resume_time = m_interval_time[sp] - m_wake_time[sp];
    Ptr<WifiPhy> phy = m_wifiDevice->GetPhy();
    //Address myMacAddress = m_wifiDevice->GetAddress();
    
    if(phy->IsStateSleep() && resume_time != Seconds(0)){
    //if(phy->IsStateOff() && resume_time != Seconds(0)){
        phy->ResumeFromSleep();
        //phy->ResumeFromOff();
        NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " station " << m_sta_id << " with service id " <<
                    sp << " WifiPhy Resume--------------------------------------------------------------");
        
    }
    else{
        NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " already in resume state... ");
    }
}
void 
SleepCycleApplication::SetStaID (uint16_t id)
{
    NS_LOG_FUNCTION (this << id);
    m_sta_id = id;
}
void 
SleepCycleApplication::SetWakeInterval (Time interval, int sp)
{
    NS_LOG_FUNCTION (this << interval);
    m_interval_time[sp] = interval;
}
void 
SleepCycleApplication::SetWakeDuration (Time duration, int sp)
{
    NS_LOG_FUNCTION (this << duration);
    if(duration <= m_interval_time[sp]){
        m_duration_time[sp] = duration;
    }
    else{
        NS_LOG_INFO("Duration " << duration << " Interval " << m_interval_time[sp]);
        NS_FATAL_ERROR ("Wake Duration should not exceed Wake Interval. Duration ");
    }
    
}
void 
SleepCycleApplication::SetWakeTime (Time wake, int sp)
{
    NS_LOG_FUNCTION (this << wake);
    m_wake_time[sp] = wake;
}


}//end of ns3

