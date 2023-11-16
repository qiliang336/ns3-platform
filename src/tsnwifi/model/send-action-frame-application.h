/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Chitiphat Chongaraemsang, fortiss GmbH
* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * .h for the ns-3 application to model TWT Action Frames
*/

#include "ns3/application.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include <vector>
#include "ns3/qos-txop.h"
#include "ns3/regular-wifi-mac.h"

namespace ns3
{


    class SendActionFrameApplication : public ns3::Application
    {
        public: 
            
            static TypeId GetTypeId (void);
            virtual TypeId GetInstanceTypeId (void) const;

            SendActionFrameApplication();
            ~SendActionFrameApplication();

            /** \brief Frequency of sending action frames
             */
            void SetStaID(uint16_t ID);

            /** \brief Frequency of sending action frames
             */
            void SetSpID(uint16_t ID);

            /** \brief Change cycle interval
             */ 
            void SetWakeInterval (uint16_t interval);

            
            /** \brief Offset before start implementing sleep and resume. 
             *          Device start with resume.
             */ 
            void SetWakeDuration (uint16_t duration);

            /** \brief Assign sleep time. Wake time will be cycle - sleep time.
             */ 
            void SetWakeTime (uint16_t wake);

            /** \brief Frequency of sending action frames
             */
            void SetFrequency(Time freq);

            /** \brief Send Action Frame
             */
            void SendActionFrame(Ptr<Packet> packet, WifiMacHeader hdr);

            /** \brief constructing action frames
             */
            void ConstructActionFrame();

        private:
            /** \brief This is an inherited function. Code that executes once the application starts
             */
            void StartApplication();
            void StopApplication();
            EventId m_Event;
            uint16_t m_sta_id; /**< ID of the station that will have this service period>*/
            uint16_t m_sp_id;
            uint16_t m_interval_time; /**< One cycle time of sleep and resume */ 
            uint16_t m_duration_time; /**< Offset at the start of simulation */ 
            uint16_t m_wake_time; /**< Sleep time in one m_interval_time>*/
            Time m_frequency;
            
            Ptr<WifiNetDevice> m_wifiDevice = NULL; /**< A WifiNetDevice that is attached to this device */  
            Ptr<Txop> m_txop; /**< Tx operation that will send the action frames */
            Ptr<RegularWifiMac> m_wifiMac; /**< Mac layer that will send the action frames >*/
            
            //You can define more stuff to record statistics, etc.
    };
}
