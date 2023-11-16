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

#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"

namespace ns3
{
/** \brief This is a "utility class".
 */
  class WifiSetup
  {
    public:
      WifiSetup ();
      virtual ~WifiSetup ();

      /**
       * @brief install IEEE802.1AX wifi on all the nodes
       * 
       * @param ap the node that will be AP
       * @param sta all the nodes that will be the station
       */

      void ConfigureDevices (NodeContainer& ap, NodeContainer& sta,std::string ssid_text,int index,int channelnumber,int channelwidth);
      NetDeviceContainer GetNetDeviceContainerAP();
      NetDeviceContainer GetNetDeviceContainerSTA();
      NodeContainer GetNodeContainer();
      void SetChannelWidth(int width);
      void SetMCS(int mcs);
      void SetGuardInterval(int gi);
      void SetExtendedBlockAck(bool use);
      void SetFrequency(double freq);
	int m_channelWidth = 40;
	int index;
    private:
      int m_mcs = 9;
	    double m_frequency = 2.4; //whether 2.4, 5 or 6 GHz
	    uint8_t m_maxChannelWidth = m_frequency == 2.4 ? 40 : 160;
      
      int m_gi = 800;
      bool m_useExtendedBlockAck =false;
      NodeContainer m_nodeContainer;
      NetDeviceContainer m_netDeviceContainerAP;
      NetDeviceContainer m_netDeviceContainerSTA;
  };

// class TimestampTag : public Tag {
// public:
//   static TypeId GetTypeId (void);
//   virtual TypeId GetInstanceTypeId (void) const;

//   virtual uint32_t GetSerializedSize (void) const;
//   virtual void Serialize (TagBuffer i) const;
//   virtual void Deserialize (TagBuffer i);

//   // these are our accessors to our tag structure
//   void SetTimestamp (Time time);
//   Time GetTimestamp (void) const;

//   void Print (std::ostream &os) const;

// private:
//   Time m_timestamp;

//   // end class TimestampTag
// };


}

#endif 
