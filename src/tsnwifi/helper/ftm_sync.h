/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Sugandh Huthanahally Mohan – huthanahally@fortiss.org
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *

 * This file holds the declarations of all the functions required for time synchronization using FTM.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/perfect-clock-model-impl.h"
#include "ns3/local-clock.h"
#include "ns3/ftm-header.h"
#include "ns3/mgt-headers.h"
#include "ns3/ftm-error-model.h"
#include "ns3/pointer.h"


using namespace ns3;

/**
 * sendtime - Send time from switch to AP.
 *
 * \param socket socket to send time from switch
 * \param ipv4address send to this address
 * \param port send to this port
 * \param clock_0 local clock of the AP
 * \param clock_1 local clock of the switch
 */
void sendtime(Ptr<Socket> socket, Ipv4Address &ipv4address, uint32_t port, Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_1);

/**
 * printTimes - Prints the local clock values of AP, STA and the Switch.
 *
 * \param clock_0 local clock of AP
 * \param clock_3 local clock of STA
 * \param clock_1 local clock of Switch
 */
void printTimes(Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_3, Ptr<LocalClock> clock_1);

/**
 * SessionOver - Callback when FTM session is completed.
 *
 * \param session FTM Session
 */
void SessionOver (FtmSession session);

/**
 * OffsetManager - Callback to manage the offset correction of STA clock.
 *
 * \param dialog dialog holding the timestamps
 * \param session FTM session
 */
void OffsetManager (Ptr<FtmSession::FtmDialog> dialog, FtmSession session);

/**
 * PropagationDelayManager - Callback to manage the propagation delay correction of STA clock.
 *
 * \param session FTM session
 */
void PropagationDelayManager (FtmSession session);

/**
 * ReceivePacket - Receive the time sent from switch to AP.
 *
 * \param socket socket to receive.
 */
void ReceivePacket (Ptr<Socket> socket);

/**
 * GenerateTraffic - Start FTM process.
 *
 * \param ap AP net device.
 * \param sta STA net device.
 * \param recvAddr MAC address of the AP.
 * \param ftm_params FTM parameters.
 * \param clock_0 local clock of AP.
 * \param clock_3 local clock of STA.
 */
void GenerateTraffic (Ptr<WifiNetDevice> ap, Ptr<WifiNetDevice> sta, Address recvAddr, FtmParams ftm_params, Ptr<LocalClock> clock_0, Ptr<LocalClock> clock_3);
