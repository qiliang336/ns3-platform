/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Chitiphat Chongaraemsang, fortiss GmbH
* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * ns-3 application to support switch tracing , TAS aspects
*/
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/switch-trace-application.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/net-device.h"
#include "ns3/bridge-net-device.h"
#include "ns3/udp-header.h"
#include "ns3/socket.h"
#include "ns3/ipv4-header.h"
#include "ns3/traffic-control-layer.h"

#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define END_CODE "\033[0m"


namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("SwitchTraceApplication");
  NS_OBJECT_ENSURE_REGISTERED(SwitchTraceApplication);

TypeId SwitchTraceApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SwitchTraceApplication")
                .SetParent <Application> ()
                .AddConstructor<SwitchTraceApplication> ()
                
                ;
    return tid;
}

TypeId SwitchTraceApplication::GetInstanceTypeId() const
{
    return SwitchTraceApplication::GetTypeId();
}

SwitchTraceApplication::SwitchTraceApplication()
{
    /**Initialized default values*/

}
SwitchTraceApplication::~SwitchTraceApplication()
{

}
void
SwitchTraceApplication::StartApplication()
{
    NS_LOG_INFO (Now () << " StartApplication() : Node " << GetNode()->GetId());
    Ptr<Node> n = GetNode ();
    m_node = m_netDevice->GetNode();

    //This loop is for the multiple devices that will have the same sleep cycle.
    for (uint32_t i = 0; i < n->GetNDevices (); i++)
    {
        NS_LOG_INFO ("GetNDevices" << i);
        Ptr<NetDevice> dev = n->GetDevice (i);
        if(n!=m_node){
            NS_FATAL_ERROR ("Not the same node.");
        }

        if(m_netDevice->IsBridge()){
            NS_LOG_INFO ("This device is bridge");
            
            //m_netDevice->SetReceiveCallback (MakeCallback (&SwitchTraceApplication::ReceivePacket, this));
            m_netDevice->SetPromiscReceiveCallback (MakeCallback (&SwitchTraceApplication::PromiscRx, this));
        }
        else{
            NS_FATAL_ERROR ("The node is not a switch with a bridge.");
        }

        //dev->SetPromiscReceiveCallback (MakeCallback (&SwitchTraceApplication::PromiscRx, this));
        
        break;

        //Ptr<WifiPhy> phy = dev->GetPhys()[0]; //default, there's only one PHY in a WaveNetDevice
        //phy->TraceConnectWithoutContext ("MonitorSnifferRx", MakeCallback(&SwitchTraceApplication::PromiscRx, this));
    }
    
      
}

bool
SwitchTraceApplication::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,uint16_t protocol, const Address &sender)
{
    NS_LOG_DEBUG (device << packet << protocol << sender);
    /*
        Packets received here only have Application data, no WifiMacHeader. 
        We created packets with 1000 bytes payload, so we'll get 1000 bytes of payload.
    */
    NS_LOG_INFO ("ReceivePacket() : Node " << GetNode()->GetId() << " : Received a packet from " << sender << " Size:" << packet->GetSize());
    SocketPriorityTag qosTag;
    if (packet->PeekPacketTag (qosTag))
    {
        NS_LOG_DEBUG ("Tag found " << qosTag.GetPriority ());
    }
    return true;
}
bool 
SwitchTraceApplication::PromiscRx (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender, const Address &receiver, ns3::NetDevice::PacketType packetType)
{
    //This is a promiscous trace. It will pick broadcast packets, and packets not directed to this node's MAC address.
    /*
        Packets received here have MAC headers and payload.
        If packets are created with 1000 bytes payload, the size here is about 38 bytes larger. 
    */
    //NS_LOG_DEBUG (Now () << " PromiscRx() : Node " << GetNode()->GetId() << " Size: " << packet->GetSize());  
    NS_LOG_INFO (Now () << " PromiscRx() : Node " << GetNode()->GetId() << " : Received from " << sender << " : Size " << packet->GetSize()
                << " : To " << receiver << " : Type " << packetType);
    NS_LOG_INFO ("PromiscRx(): " << packet);
    SocketPriorityTag qosTag;
    
    if (packet->PeekPacketTag (qosTag))
    {
        NS_LOG_DEBUG ("Tag found " << qosTag.GetPriority ());
    }
    else{
        NS_LOG_DEBUG ("No Priority Tag found.");
    }
    
    Ipv4Header hrd;
    if (packet->PeekHeader (hrd))
    {
        //Let's see if this packet is intended to this node
        Ipv4Address destination = hrd.GetDestination();
        Ipv4Address source = hrd.GetSource();

        NS_LOG_INFO ("PromiscRx(): IPv4 Header From: " << source << " To: " << destination );
    }
    UdpHeader header;
    
    if (packet->PeekHeader (header))
    {
        //Let's see if this packet is intended to this node
        uint16_t destination = header.GetDestinationPort();
        uint16_t source = header.GetSourcePort();

        NS_LOG_INFO ("PromiscRx(): UDP Header From: " << source << " To: " << destination );
    }
    
    //packet->PrintPacketTags(std::cout);
    return true;  
    
}

void 
SwitchTraceApplication::SetNetDevice (Ptr<NetDevice> device)
{
    NS_LOG_FUNCTION (this << device);
    m_netDevice = device;
}

}//end of ns3

