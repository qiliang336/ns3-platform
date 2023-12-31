/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/packet.h"
#include "regular-wifi-mac.h"
#include "wifi-phy.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "msdu-aggregator.h"
#include "mpdu-aggregator.h"
#include "mgt-headers.h"
#include "amsdu-subframe-header.h"
#include "wifi-net-device.h"
#include "ns3/ht-configuration.h"
#include "ns3/vht-configuration.h"
#include "ns3/he-configuration.h"
#include <algorithm>
#include <cmath>
#include "ns3/he-frame-exchange-manager.h"
#include "channel-access-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RegularWifiMac");

NS_OBJECT_ENSURE_REGISTERED (RegularWifiMac);

RegularWifiMac::RegularWifiMac ()
  : m_qosSupported (0),
    m_erpSupported (0),
    m_dsssSupported (0)
{
  NS_LOG_FUNCTION (this);
  m_rxMiddle = Create<MacRxMiddle> ();
  // m_rxMiddle->SetForwardCallback (MakeCallback (&RegularWifiMac::Receive, this));

  m_txMiddle = Create<MacTxMiddle> ();

  m_channelAccessManager = CreateObject<ChannelAccessManager> ();

  m_txop = CreateObject<Txop> ();
  m_txop->SetChannelAccessManager (m_channelAccessManager);
  m_txop->SetTxMiddle (m_txMiddle);
  // m_txop->SetDroppedMpduCallback (MakeCallback (&DroppedMpduTracedCallback::operator(),
  //                                               &m_droppedMpduCallback));

  //Construct the EDCAFs. The ordering is important - highest
  //priority (Table 9-1 UP-to-AC mapping; IEEE 802.11-2012) must be created
  //first.
  SetupEdcaQueue (AC_VO);
  SetupEdcaQueue (AC_VI);
  SetupEdcaQueue (AC_BE);
  SetupEdcaQueue (AC_BK);
  
  m_ftm_enabled = false;
  m_ftm_enable_later = false;
  m_ftm_manager = 0;

}

RegularWifiMac::~RegularWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
RegularWifiMac::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  m_txop->Initialize ();

  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->Initialize ();
    }
}

void
RegularWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_rxMiddle = 0;
  m_txMiddle = 0;

  m_phy = 0;
  m_stationManager = 0;
  if (m_feManager != 0)
    {
      m_feManager->Dispose ();
    }
  m_feManager = 0;

  m_txop->Dispose ();
  m_txop = 0;

  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->Dispose ();
      i->second = 0;
    }

  m_channelAccessManager->Dispose ();
  m_channelAccessManager = 0;
 
  m_ftm_manager = 0;

  WifiMac::DoDispose ();
}

void
RegularWifiMac::SetupFrameExchangeManager (void)
{
  NS_LOG_FUNCTION (this);

  if (GetHeSupported ())
    {
      m_feManager = CreateObject<HeFrameExchangeManager> ();
    }
  else if (GetVhtSupported ())
    {
      m_feManager = CreateObject<VhtFrameExchangeManager> ();
    }
  else if (GetHtSupported ())
    {
      m_feManager = CreateObject<HtFrameExchangeManager> ();
    }
  else if (GetQosSupported ())
    {
      m_feManager = CreateObject<QosFrameExchangeManager> ();
    }
  else
    {
      m_feManager = CreateObject<FrameExchangeManager> ();
    }

  m_feManager->SetWifiMac (this);
  m_feManager->SetMacTxMiddle (m_txMiddle);
  m_feManager->SetMacRxMiddle (m_rxMiddle);
  m_feManager->SetAddress (GetAddress ());
  m_feManager->SetBssid (GetBssid (0));
  // m_feManager->GetWifiTxTimer ().SetMpduResponseTimeoutCallback (MakeCallback (&MpduResponseTimeoutTracedCallback::operator(),
  //                                                                              &m_mpduResponseTimeoutCallback));
  m_feManager->GetWifiTxTimer ().SetPsduResponseTimeoutCallback (MakeCallback (&PsduResponseTimeoutTracedCallback::operator(),
                                                                               &m_psduResponseTimeoutCallback));
  m_feManager->GetWifiTxTimer ().SetPsduMapResponseTimeoutCallback (MakeCallback (&PsduMapResponseTimeoutTracedCallback::operator(),
                                                                                  &m_psduMapResponseTimeoutCallback));
  // m_feManager->SetDroppedMpduCallback (MakeCallback (&DroppedMpduTracedCallback::operator(),
  //                                                    &m_droppedMpduCallback));
  // m_feManager->SetAckedMpduCallback (MakeCallback (&MpduTracedCallback::operator(),
  //                                                  &m_ackedMpduCallback));
  m_channelAccessManager->SetupFrameExchangeManager (m_feManager);
  if (GetQosSupported ())
    {
      for (const auto& pair : m_edca)
        {
          pair.second->SetQosFrameExchangeManager (DynamicCast<QosFrameExchangeManager> (m_feManager));
        }
    }
}

Ptr<FrameExchangeManager>
RegularWifiMac::GetFrameExchangeManager (void) const
{
  return m_feManager;
}

void
RegularWifiMac::SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_stationManager = stationManager;
  m_txop->SetWifiRemoteStationManager (stationManager);
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetWifiRemoteStationManager (stationManager);
    }
}

Ptr<WifiRemoteStationManager>
RegularWifiMac::GetWifiRemoteStationManager (uint8_t linkId) const
{
  return GetLink(linkId).stationManager;
}

ExtendedCapabilities
RegularWifiMac::GetExtendedCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  ExtendedCapabilities capabilities;
  capabilities.SetHtSupported (GetHtSupported ());
  capabilities.SetVhtSupported (GetVhtSupported ());
  //TODO: to be completed
  return capabilities;
}

HtCapabilities
RegularWifiMac::GetHtCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  HtCapabilities capabilities;
  if (GetHtSupported ())
    {
      Ptr<HtConfiguration> htConfiguration = GetHtConfiguration ();
      bool sgiSupported = htConfiguration->GetShortGuardIntervalSupported ();
      capabilities.SetHtSupported (1);
      capabilities.SetLdpc (htConfiguration->GetLdpcSupported ());
      capabilities.SetSupportedChannelWidth (m_phy->GetChannelWidth () >= 40);
      capabilities.SetShortGuardInterval20 (sgiSupported);
      capabilities.SetShortGuardInterval40 (m_phy->GetChannelWidth () >= 40 && sgiSupported);
      // Set Maximum A-MSDU Length subfield
      uint16_t maxAmsduSize = std::max ({m_voMaxAmsduSize, m_viMaxAmsduSize,
                                         m_beMaxAmsduSize, m_bkMaxAmsduSize});
      if (maxAmsduSize <= 3839)
        {
          capabilities.SetMaxAmsduLength (3839);
        }
      else
        {
          capabilities.SetMaxAmsduLength (7935);
        }
      uint32_t maxAmpduLength = std::max ({m_voMaxAmpduSize, m_viMaxAmpduSize,
                                           m_beMaxAmpduSize, m_bkMaxAmpduSize});
      // round to the next power of two minus one
      maxAmpduLength = (1ul << static_cast<uint32_t> (std::ceil (std::log2 (maxAmpduLength + 1)))) - 1;
      // The maximum A-MPDU length in HT capabilities elements ranges from 2^13-1 to 2^16-1
      capabilities.SetMaxAmpduLength (std::min (std::max (maxAmpduLength, 8191u), 65535u));

      capabilities.SetLSigProtectionSupport (true);
      uint64_t maxSupportedRate = 0; //in bit/s
      for (const auto & mcs : m_phy->GetMcsList (WIFI_MOD_CLASS_HT))
        {
          capabilities.SetRxMcsBitmask (mcs.GetMcsValue ());
          uint8_t nss = (mcs.GetMcsValue () / 8) + 1;
          NS_ASSERT (nss > 0 && nss < 5);
          uint64_t dataRate = mcs.GetDataRate (m_phy->GetChannelWidth (), sgiSupported ? 400 : 800, nss);
          if (dataRate > maxSupportedRate)
            {
              maxSupportedRate = dataRate;
              NS_LOG_DEBUG ("Updating maxSupportedRate to " << maxSupportedRate);
            }
        }
      capabilities.SetRxHighestSupportedDataRate (static_cast<uint16_t> (maxSupportedRate / 1e6)); //in Mbit/s
      capabilities.SetTxMcsSetDefined (m_phy->GetNMcs () > 0);
      capabilities.SetTxMaxNSpatialStreams (m_phy->GetMaxSupportedTxSpatialStreams ());
      //we do not support unequal modulations
      capabilities.SetTxRxMcsSetUnequal (0);
      capabilities.SetTxUnequalModulation (0);
    }
  return capabilities;
}

VhtCapabilities
RegularWifiMac::GetVhtCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  VhtCapabilities capabilities;
  if (GetVhtSupported ())
    {
      Ptr<HtConfiguration> htConfiguration = GetHtConfiguration ();
      Ptr<VhtConfiguration> vhtConfiguration = GetVhtConfiguration ();
      bool sgiSupported = htConfiguration->GetShortGuardIntervalSupported ();
      capabilities.SetVhtSupported (1);
      if (m_phy->GetChannelWidth () == 160)
        {
          capabilities.SetSupportedChannelWidthSet (1);
        }
      else
        {
          capabilities.SetSupportedChannelWidthSet (0);
        }
      // Set Maximum MPDU Length subfield
      uint16_t maxAmsduSize = std::max ({m_voMaxAmsduSize, m_viMaxAmsduSize,
                                         m_beMaxAmsduSize, m_bkMaxAmsduSize});
      if (maxAmsduSize <= 3839)
        {
          capabilities.SetMaxMpduLength (3895);
        }
      else if (maxAmsduSize <= 7935)
        {
          capabilities.SetMaxMpduLength (7991);
        }
      else
        {
          capabilities.SetMaxMpduLength (11454);
        }
      uint32_t maxAmpduLength = std::max ({m_voMaxAmpduSize, m_viMaxAmpduSize,
                                           m_beMaxAmpduSize, m_bkMaxAmpduSize});
      // round to the next power of two minus one
      maxAmpduLength = (1ul << static_cast<uint32_t> (std::ceil (std::log2 (maxAmpduLength + 1)))) - 1;
      // The maximum A-MPDU length in VHT capabilities elements ranges from 2^13-1 to 2^20-1
      capabilities.SetMaxAmpduLength (std::min (std::max (maxAmpduLength, 8191u), 1048575u));

      capabilities.SetRxLdpc (htConfiguration->GetLdpcSupported ());
      capabilities.SetShortGuardIntervalFor80Mhz ((m_phy->GetChannelWidth () == 80) && sgiSupported);
      capabilities.SetShortGuardIntervalFor160Mhz ((m_phy->GetChannelWidth () == 160) && sgiSupported);
      uint8_t maxMcs = 0;
      for (const auto & mcs : m_phy->GetMcsList (WIFI_MOD_CLASS_VHT))
        {
          if (mcs.GetMcsValue () > maxMcs)
            {
              maxMcs = mcs.GetMcsValue ();
            }
        }
      // Support same MaxMCS for each spatial stream
      for (uint8_t nss = 1; nss <= m_phy->GetMaxSupportedRxSpatialStreams (); nss++)
        {
          capabilities.SetRxMcsMap (maxMcs, nss);
        }
      for (uint8_t nss = 1; nss <= m_phy->GetMaxSupportedTxSpatialStreams (); nss++)
        {
          capabilities.SetTxMcsMap (maxMcs, nss);
        }
      uint64_t maxSupportedRateLGI = 0; //in bit/s
      for (const auto & mcs : m_phy->GetMcsList (WIFI_MOD_CLASS_VHT))
        {
          if (!mcs.IsAllowed (m_phy->GetChannelWidth (), 1))
            {
              continue;
            }
          if (mcs.GetDataRate (m_phy->GetChannelWidth ()) > maxSupportedRateLGI)
            {
              maxSupportedRateLGI = mcs.GetDataRate (m_phy->GetChannelWidth ());
              NS_LOG_DEBUG ("Updating maxSupportedRateLGI to " << maxSupportedRateLGI);
            }
        }
      capabilities.SetRxHighestSupportedLgiDataRate (static_cast<uint16_t> (maxSupportedRateLGI / 1e6)); //in Mbit/s
      capabilities.SetTxHighestSupportedLgiDataRate (static_cast<uint16_t> (maxSupportedRateLGI / 1e6)); //in Mbit/s
      //To be filled in once supported
      capabilities.SetRxStbc (0);
      capabilities.SetTxStbc (0);
    }
  return capabilities;
}

HeCapabilities
RegularWifiMac::GetHeCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  HeCapabilities capabilities;
  if (GetHeSupported ())
    {
      Ptr<HtConfiguration> htConfiguration = GetHtConfiguration ();
      Ptr<HeConfiguration> heConfiguration = GetHeConfiguration ();
      capabilities.SetHeSupported (1);
      uint8_t channelWidthSet = 0;
      if ((m_phy->GetChannelWidth () >= 40) && (m_phy->GetPhyBand () == WIFI_PHY_BAND_2_4GHZ))
        {
          channelWidthSet |= 0x01;
        }
      if ((m_phy->GetChannelWidth () >= 80) && ((m_phy->GetPhyBand () == WIFI_PHY_BAND_5GHZ) || (m_phy->GetPhyBand () == WIFI_PHY_BAND_6GHZ)))
        {
          channelWidthSet |= 0x02;
        }
      if ((m_phy->GetChannelWidth () >= 160) && ((m_phy->GetPhyBand () == WIFI_PHY_BAND_5GHZ) || (m_phy->GetPhyBand () == WIFI_PHY_BAND_6GHZ)))
        {
          channelWidthSet |= 0x04;
        }
      capabilities.SetChannelWidthSet (channelWidthSet);
      capabilities.SetLdpcCodingInPayload (htConfiguration->GetLdpcSupported ());
      uint8_t gi = 0;
      if (heConfiguration->GetGuardInterval () <= NanoSeconds (1600))
        {
          //todo: We assume for now that if we support 800ns GI then 1600ns GI is supported as well
          gi |= 0x01;
        }
      if (heConfiguration->GetGuardInterval () == NanoSeconds (800))
        {
          gi |= 0x02;
        }
      capabilities.SetHeLtfAndGiForHePpdus (gi);
      uint32_t maxAmpduLength = std::max ({m_voMaxAmpduSize, m_viMaxAmpduSize,
                                           m_beMaxAmpduSize, m_bkMaxAmpduSize});
      // round to the next power of two minus one
      maxAmpduLength = (1ul << static_cast<uint32_t> (std::ceil (std::log2 (maxAmpduLength + 1)))) - 1;
      // The maximum A-MPDU length in HE capabilities elements ranges from 2^20-1 to 2^23-1
      capabilities.SetMaxAmpduLength (std::min (std::max (maxAmpduLength, 1048575u), 8388607u));

      uint8_t maxMcs = 0;
      for (const auto & mcs : m_phy->GetMcsList (WIFI_MOD_CLASS_HE))
        {
          if (mcs.GetMcsValue () > maxMcs)
            {
              maxMcs = mcs.GetMcsValue ();
            }
        }
      capabilities.SetHighestMcsSupported (maxMcs);
      capabilities.SetHighestNssSupported (m_phy->GetMaxSupportedTxSpatialStreams ());
    }
  return capabilities;
}

void
RegularWifiMac::SetVoBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetVOQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetViBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetVIQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetBeBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetBEQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetBkBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetBKQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetVoBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetVOQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetViBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetVIQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetBeBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetBEQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetBkBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetBKQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetupEdcaQueue (AcIndex ac)
{
  NS_LOG_FUNCTION (this << ac);

  //Our caller shouldn't be attempting to setup a queue that is
  //already configured.
  NS_ASSERT (m_edca.find (ac) == m_edca.end ());

  Ptr<QosTxop> edca = CreateObject<QosTxop> ();
  edca->SetChannelAccessManager (m_channelAccessManager);
  edca->SetTxMiddle (m_txMiddle);
  // edca->GetBaManager ()->SetTxOkCallback (MakeCallback (&MpduTracedCallback::operator(),
  //                                                       &m_ackedMpduCallback));                                                      
  // edca->GetBaManager ()->SetTxFailedCallback (MakeCallback (&MpduTracedCallback::operator(),
  //                                                           &m_nackedMpduCallback));
  // edca->SetDroppedMpduCallback (MakeCallback (&DroppedMpduTracedCallback::operator(),
  //                                             &m_droppedMpduCallback));
  edca->SetAccessCategory (ac);

  m_edca.insert (std::make_pair (ac, edca));
}

void
RegularWifiMac::SetTypeOfStation (TypeOfStation type)
{
  NS_LOG_FUNCTION (this << type);
  m_typeOfStation = type;
}

TypeOfStation
RegularWifiMac::GetTypeOfStation (void) const
{
  return m_typeOfStation;
}

Ptr<Txop>
RegularWifiMac::GetTxop () const
{
  return m_txop;
}

Ptr<QosTxop>
RegularWifiMac::GetQosTxop (AcIndex ac) const
{
  return m_edca.find (ac)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetQosTxop (uint8_t tid) const
{
  return GetQosTxop (QosUtilsMapTidToAc (tid));
}

Ptr<QosTxop>
RegularWifiMac::GetVOQueue () const
{
  return m_edca.find (AC_VO)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetVIQueue () const
{
  return m_edca.find (AC_VI)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetBEQueue () const
{
  return m_edca.find (AC_BE)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetBKQueue () const
{
  return m_edca.find (AC_BK)->second;
}

void
RegularWifiMac::SetWifiPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  m_phy = phy;
  m_channelAccessManager->SetupPhyListener (phy);
  NS_ASSERT (m_feManager != 0);
  m_feManager->SetWifiPhy (phy);

  if (m_ftm_enable_later)
    {
      EnableFtm();
      m_ftm_enable_later = false;
    }
}

Ptr<WifiPhy>
RegularWifiMac::GetWifiPhy (uint8_t linkId) const
{
  NS_LOG_FUNCTION (this);
  return m_phy;
}

void
RegularWifiMac::ResetWifiPhy (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_feManager != 0);
  m_feManager->ResetPhy ();
  m_channelAccessManager->RemovePhyListener (m_phy);
  m_phy = 0;
}

void
RegularWifiMac::SetForwardUpCallback (ForwardUpCallback upCallback)
{
  NS_LOG_FUNCTION (this);
  m_forwardUp = upCallback;
}

void
RegularWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = linkUp;
}

void
RegularWifiMac::SetLinkDownCallback (Callback<void> linkDown)
{
  NS_LOG_FUNCTION (this);
  m_linkDown = linkDown;
}

void
RegularWifiMac::SetVSACallback (VsaCallback vsaCallback)
{
  NS_LOG_FUNCTION (this);
  m_vsaCallback = vsaCallback;
}

void
RegularWifiMac::SetQosSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_qosSupported = enable;
}

bool
RegularWifiMac::GetQosSupported () const
{
  return m_qosSupported;
}

bool
RegularWifiMac::GetHtSupported () const
{
  if (GetHtConfiguration ())
    {
      return true;
    }
  return false;
}

bool
RegularWifiMac::GetVhtSupported () const
{
  if (GetVhtConfiguration ())
    {
      return true;
    }
  return false;
}

bool
RegularWifiMac::GetHeSupported () const
{
  if (GetHeConfiguration ())
    {
      return true;
    }
  return false;
}

bool
RegularWifiMac::GetErpSupported () const
{
  return m_erpSupported;
}

void
RegularWifiMac::SetErpSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  if (enable)
    {
      SetDsssSupported (true);
    }
  m_erpSupported = enable;
}

void
RegularWifiMac::SetDsssSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_dsssSupported = enable;
}

bool
RegularWifiMac::GetDsssSupported () const
{
  return m_dsssSupported;
}

void
RegularWifiMac::SetCtsToSelfSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_ctsToSelfSupported = enable;
}

void
RegularWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = address;
}

Mac48Address
RegularWifiMac::GetAddress (void) const
{
  return m_address;
}

void
RegularWifiMac::SetSsid (Ssid ssid)
{
  NS_LOG_FUNCTION (this << ssid);
  m_ssid = ssid;
}

Ssid
RegularWifiMac::GetSsid (void) const
{
  return m_ssid;
}

void
RegularWifiMac::SetBssid (Mac48Address bssid)
{
  NS_LOG_FUNCTION (this << bssid);
  m_bssid = bssid;
  if (m_feManager)
    {
      m_feManager->SetBssid (bssid);
    }
}

Mac48Address
RegularWifiMac::GetBssid (uint8_t linkId) const
{
  return m_bssid;
}

void
RegularWifiMac::SetPromisc (void)
{
  NS_ASSERT (m_feManager != 0);
  m_feManager->SetPromisc ();
}

void
RegularWifiMac::SetShortSlotTimeSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortSlotTimeSupported = enable;
}

bool
RegularWifiMac::GetShortSlotTimeSupported (void) const
{
  return m_shortSlotTimeSupported;
}

void
RegularWifiMac::Enqueue (Ptr<Packet> packet,
                         Mac48Address to, Mac48Address from)
{
  //We expect RegularWifiMac subclasses which do support forwarding (e.g.,
  //AP) to override this method. Therefore, we throw a fatal error if
  //someone tries to invoke this method on a class which has not done
  //this.
  NS_FATAL_ERROR ("This MAC entity (" << this << ", " << GetAddress ()
                                      << ") does not support Enqueue() with from address");
}

bool
RegularWifiMac::SupportsSendFrom (void) const
{
  return false;
}

void
RegularWifiMac::ForwardUp (Ptr<const Packet> packet, Mac48Address from, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from << to);
  m_forwardUp (packet, from, to);
}

void
RegularWifiMac::Receive (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);

  const WifiMacHeader* hdr = &mpdu->GetHeader ();
  Ptr<Packet> packet = mpdu->GetPacket ()->Copy ();
  Mac48Address to = hdr->GetAddr1 ();
  Mac48Address from = hdr->GetAddr2 ();

  //We don't know how to deal with any frame that is not addressed to
  //us (and odds are there is nothing sensible we could do anyway),
  //so we ignore such frames.
  //
  //The derived class may also do some such filtering, but it doesn't
  //hurt to have it here too as a backstop.
  if(to == Mac48Address::GetBroadcast ())
  {
    if (hdr->IsMgt () && hdr->IsAction ())
    {
      //NS_LOG_FUNCTION("both mgt and action");
      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
      {
        case WifiActionHeader::VENDOR_SPECIFIC_ACTION:

          switch (actionHdr.GetAction ().vsa)
            {
            case WifiActionHeader::TARGET_WAKE_TIME_REQUEST:
              {
                
                NS_LOG_FUNCTION("TARGET_WAKE_TIME_REQUEST");
                if (!m_vsaCallback.IsNull ()){
    
                  m_vsaCallback (mpdu);
                }
                else
                {
                  NS_LOG_FUNCTION("Missing Callback for TWT request action frames");
                }
                
                
                return;
              }
              case WifiActionHeader::TARGET_WAKE_TIME_RESPOND:
              {
                
                NS_LOG_FUNCTION("TARGET_WAKE_TIME_RESPOND");
                if (!m_vsaCallback.IsNull ())
                {  
                  m_vsaCallback (mpdu);
                }
                else
                {
                  NS_LOG_FUNCTION("Missing Callback for TWT response action frames");
                }
                
                return;
              }
              
            default:
              NS_FATAL_ERROR ("Unsupported Action field in Block Ack Action frame");
              return;
            }
        default:
          NS_FATAL_ERROR ("Unsupported Action frame received");
          return;
      }
    }
    
  }
  if (to != GetAddress ())
    {
      return;
    }

  if (hdr->IsMgt () && hdr->IsAction ())
    {
      //There is currently only any reason for Management Action
      //frames to be flying about if we are a QoS STA.
      //FTM does not need QoS to transmit its frames, moved to blockAck case
      //      NS_ASSERT (m_qosSupported);

      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
        {
        case WifiActionHeader::BLOCK_ACK:
	  {
	    NS_ASSERT (m_qosSupported);
          switch (actionHdr.GetAction ().blockAck)
            {
            case WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST:
              {
                MgtAddBaRequestHeader reqHdr;
                packet->RemoveHeader (reqHdr);

                //We've received an ADDBA Request. Our policy here is
                //to automatically accept it, so we get the ADDBA
                //Response on it's way immediately.
                NS_ASSERT (m_feManager != 0);
                Ptr<HtFrameExchangeManager> htFem = DynamicCast<HtFrameExchangeManager> (m_feManager);
                if (htFem != 0)
                  {
                    htFem->SendAddBaResponse (&reqHdr, from);
                  }
                //This frame is now completely dealt with, so we're done.
                return;
              }
            case WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE:
              {
                MgtAddBaResponseHeader respHdr;
                packet->RemoveHeader (respHdr);

                //We've received an ADDBA Response. We assume that it
                //indicates success after an ADDBA Request we have
                //sent (we could, in principle, check this, but it
                //seems a waste given the level of the current model)
                //and act by locally establishing the agreement on
                //the appropriate queue.
                AcIndex ac = QosUtilsMapTidToAc (respHdr.GetTid ());
                m_edca[ac]->GotAddBaResponse (respHdr, from);
                //This frame is now completely dealt with, so we're done.
                return;
              }
            case WifiActionHeader::BLOCK_ACK_DELBA:
              {
                MgtDelBaHeader delBaHdr;
                packet->RemoveHeader (delBaHdr);

                if (delBaHdr.IsByOriginator ())
                  {
                    //This DELBA frame was sent by the originator, so
                    //this means that an ingoing established
                    //agreement exists in HtFrameExchangeManager and we need to
                    //destroy it.
                    NS_ASSERT (m_feManager != 0);
                    Ptr<HtFrameExchangeManager> htFem = DynamicCast<HtFrameExchangeManager> (m_feManager);
                    if (htFem != 0)
                      {
                        htFem->DestroyBlockAckAgreement (from, delBaHdr.GetTid ());
                      }
                  }
                else
                  {
                    //We must have been the originator. We need to
                    //tell the correct queue that the agreement has
                    //been torn down
                    AcIndex ac = QosUtilsMapTidToAc (delBaHdr.GetTid ());
                    m_edca[ac]->GotDelBaFrame (&delBaHdr, from);
                  }
                //This frame is now completely dealt with, so we're done.
                return;
              }
            default:
              NS_FATAL_ERROR ("Unsupported Action field in Block Ack Action frame");
              return;
            }
	  }
	  case WifiActionHeader::PUBLIC_ACTION: //Take care of FTM frames.
          {
            NS_ASSERT_MSG (m_ftm_enabled, "Received Public Action frame but FTM is not enabled!");
            switch (actionHdr.GetAction().publicAction)
            {
              case WifiActionHeader::FTM_REQUEST:
                {
                  FtmRequestHeader ftm_req;
                  packet->RemoveHeader (ftm_req);
                  m_ftm_manager->ReceivedFtmRequest (from, ftm_req);
                  return;
                }
              case WifiActionHeader::FTM_RESPONSE:
                {
                  FtmResponseHeader ftm_res;
                  packet->RemoveHeader (ftm_res);
                  m_ftm_manager->ReceivedFtmResponse (from, ftm_res);
                  return;
                }
              default:
                NS_FATAL_ERROR("Unsupported Public Action frame received");
                return;
            }
	  }
        default:
          NS_FATAL_ERROR ("Unsupported Action frame received");
          return;
        }
    }
  NS_FATAL_ERROR ("Don't know how to handle frame (type=" << hdr->GetType ());
}

void
RegularWifiMac::DeaggregateAmsduAndForward (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);
  for (auto& msduPair : *PeekPointer (mpdu))
    {
      ForwardUp (msduPair.first, msduPair.second.GetSourceAddr (),
                 msduPair.second.GetDestinationAddr ());
    }
}

TypeId
RegularWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RegularWifiMac")
    .SetParent<WifiMac> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("FTM_Enabled",
		    "Used to enable support of FTM exchanges.",
		    BooleanValue (false),
		    MakeBooleanAccessor (&RegularWifiMac::SetFtmEnabled,
		   			 &RegularWifiMac::GetFtmEnabled),
		    MakeBooleanChecker ())
  ;
  return tid;
}

void
RegularWifiMac::ConfigureStandard (WifiStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  uint32_t cwmin = 0;
  uint32_t cwmax = 0;
  switch (standard)
    {
    case WIFI_STANDARD_80211n_5GHZ:
    case WIFI_STANDARD_80211ac:
    case WIFI_STANDARD_80211ax_5GHZ:
    case WIFI_STANDARD_80211ax_6GHZ:
      {
        SetQosSupported (true);
        cwmin = 15;
        cwmax = 1023;
        break;
      }
    case WIFI_STANDARD_80211ax_2_4GHZ:
    case WIFI_STANDARD_80211n_2_4GHZ:
      {
        SetQosSupported (true);
      }
    case WIFI_STANDARD_80211g:
      SetErpSupported (true);
    case WIFI_STANDARD_80211a:
    case WIFI_STANDARD_80211p:
      cwmin = 15;
      cwmax = 1023;
      break;
    case WIFI_STANDARD_80211b:
      SetDsssSupported (true);
      cwmin = 31;
      cwmax = 1023;
      break;
    default:
      NS_FATAL_ERROR ("Unsupported WifiPhyStandard in RegularWifiMac::FinishConfigureStandard ()");
    }

  SetupFrameExchangeManager ();
  ConfigureContentionWindow (cwmin, cwmax);
}

void
RegularWifiMac::ConfigureContentionWindow (uint32_t cwMin, uint32_t cwMax)
{
  bool isDsssOnly = m_dsssSupported && !m_erpSupported;
  //The special value of AC_BE_NQOS which exists in the Access
  //Category enumeration allows us to configure plain old DCF.
  ConfigureDcf (m_txop, cwMin, cwMax, isDsssOnly, AC_BE_NQOS);

  //Now we configure the EDCA functions
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      ConfigureDcf (i->second, cwMin, cwMax, isDsssOnly, i->first);
    }
}

void
RegularWifiMac::SetFtmEnabled (bool enabled)
{
  if (enabled)
    {
      if (GetWifiPhy(0) == 0)
        {
          m_ftm_enable_later = true;
          return;
        }
      EnableFtm ();
    }
  else if (!enabled)
    {
      DisableFtm ();
    }
}

bool
RegularWifiMac::GetFtmEnabled (void) const
{
  return m_ftm_enabled;
}

void
RegularWifiMac::EnableFtm (void)
{
  NS_LOG_FUNCTION (this);
  m_ftm_enabled = true;
  m_ftm_manager = CreateObject<FtmManager> (GetWifiPhy (0), GetTxop ());
  m_ftm_manager->SetMacAddress(GetAddress());
  Time::Unit resolution = Time::GetResolution();
  if (resolution != Time::PS && resolution != Time::FS)
    {
      NS_LOG_WARN ("FTM enabled but time resolution not set to PS or FS, FTM results will be less accurate!");
    }
}

void
RegularWifiMac::DisableFtm (void)
{
  NS_LOG_FUNCTION (this);
  m_ftm_enabled = false;
  m_ftm_manager = 0;
}

Ptr<FtmSession>
RegularWifiMac::NewFtmSession (Mac48Address partner)
{
  NS_LOG_FUNCTION (this << partner);
  if (m_ftm_enabled)
    {
      return m_ftm_manager->CreateNewSession(partner, FtmSession::FTM_INITIATOR);
    }
  return 0;
}


} //namespace ns3
