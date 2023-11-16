/**
 * Copyright (C) 2021, 2022 fortiss GmbH
 * @author Chitiphat Chongaraemsang, fortiss GmbH
* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * .h for the ns-3 application t to model the TWT sleep-awake cycle
*/
#ifndef WAVETEST_CUSTOM_APPLICATION_H
#define WAVETEST_CUSTOM_APPLICATION_H
#include "ns3/application.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include <vector>
#include "ns3/regular-wifi-mac.h"
namespace ns3
{


    class SleepCycleApplication : public ns3::Application
    {
        public: 
            
            static TypeId GetTypeId (void);
            virtual TypeId GetInstanceTypeId (void) const;

            SleepCycleApplication();
            ~SleepCycleApplication();

            void SetStaID(uint16_t id);

            /** \brief Change cycle interval
             */ 
            void SetWakeInterval (Time interval, int sp);

            
            /** \brief Offset before start implementing sleep and resume. 
             *          Device start with resume.
             */ 
            void SetWakeDuration (Time duration, int sp);

            /** \brief Assign sleep time. Wake time will be cycle - sleep time.
             */ 
            void SetWakeTime (Time wake, int sp);

            /** \brief Set WifiPhy to sleep and resume
             */
            void SetWifiPhySleepResume(int sp);
            void SetWifiPhySleep(int sp);
            void SetWifiPhyResume(int sp);

            /**
             *  Phasing action frames
            */
            void Receive(Ptr<WifiMacQueueItem> mpdu);
        private:
            /** \brief This is an inherited function. Code that executes once the application starts
             */
            void StartApplication();
            void StopApplication();
            EventId m_sendEvent[100];
            uint16_t m_sta_id;
            Time m_interval_time[100] = {MilliSeconds (0)}; /**< One cycle time of sleep and resume */ 
            Time m_duration_time[100] = {MilliSeconds (0)}; /**< Offset at the start of simulation */ 
            Time m_wake_time[100] = {MilliSeconds (0)}; /**< Sleep time in one m_cycle_time>*/

            bool m_activated = false;
            bool m_running[100] = {false};
            bool m_remain = false;
            
            Ptr<WifiNetDevice> m_wifiDevice; /**< A WifiNetDevice that is attached to this device */  

            Ptr<RegularWifiMac> m_wifiMac;
            
            //You can define more stuff to record statistics, etc.
    };
}

#endif
