/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Chitiphat Chongaraemsang, fortiss GmbH
* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * .h for the ns-3 application to support switch tracing , TAS aspects
*/

#ifndef TRACE_CUSTOM_APPLICATION_H
#define TRACE_CUSTOM_APPLICATION_H
#include "ns3/application.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/queue-disc-container.h"
#include <vector>

namespace ns3
{


    class SwitchTraceApplication : public ns3::Application
    {
        public: 
            
            static TypeId GetTypeId (void);
            virtual TypeId GetInstanceTypeId (void) const;

            SwitchTraceApplication();
            ~SwitchTraceApplication();
            
            bool ReceivePacket (Ptr<NetDevice> device,Ptr<const Packet> packet,uint16_t protocol, const Address &sender);
            bool PromiscRx (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender, const Address &receiver, ns3::NetDevice::PacketType packetType);

            void SetNetDevice (Ptr<NetDevice> device);


        private:
            /** \brief This is an inherited function. Code that executes once the application starts
             */
            void StartApplication();
            
            Ptr<NetDevice> m_netDevice; /**< A NetDevice that is attached to this node */  
            Ptr<Node> m_node;
            //You can define more stuff to record statistics, etc.
    };
}

#endif
