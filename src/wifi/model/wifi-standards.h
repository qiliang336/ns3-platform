/*
 * Copyright (c) 2007 INRIA
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

#ifndef WIFI_STANDARD_H
#define WIFI_STANDARD_H

#include "wifi-phy-band.h"

#include "ns3/abort.h"

#include <list>
#include <map>

namespace ns3
{
/**
 * \ingroup wifi
 * Identifies the PHY specification that a Wifi device is configured to use.
 */
enum WifiPhyStandard
{
  /** OFDM PHY (Clause 17) */
  WIFI_PHY_STANDARD_80211a,
  /** DSSS PHY (Clause 15) and HR/DSSS PHY (Clause 18) */
  WIFI_PHY_STANDARD_80211b,
  /** ERP-OFDM PHY (Clause 19, Section 19.5) */
  WIFI_PHY_STANDARD_80211g,
  /** OFDM PHY for the 5 GHz band (Clause 17 with 10 MHz channel bandwidth) */
  /** this value is NS_DEPRECATED_3_32 */
  WIFI_PHY_STANDARD_80211_10MHZ,
  /** OFDM PHY for the 5 GHz band (Clause 17 with 5 MHz channel bandwidth) */
  /** this value is NS_DEPRECATED_3_32 */
  WIFI_PHY_STANDARD_80211_5MHZ,
  /** OFDM PHY (Clause 17 - amendment for 10 MHz and 5 MHz channels) */
  WIFI_PHY_STANDARD_80211p,
  /** HT PHY for the 2.4 GHz band (clause 20) */
  /** this value is NS_DEPRECATED_3_32 */
  WIFI_PHY_STANDARD_80211n_2_4GHZ,
  /** HT PHY for the 5 GHz band (clause 20) */
  /** this value is NS_DEPRECATED_3_32 */
  WIFI_PHY_STANDARD_80211n_5GHZ,
  /** HT PHY  (clause 20) */
  WIFI_PHY_STANDARD_80211n,
  /** VHT PHY (clause 22) */
  WIFI_PHY_STANDARD_80211ac,
  /** HE PHY (clause 26) */
  WIFI_PHY_STANDARD_80211ax,
  /** Unspecified */
  WIFI_PHY_STANDARD_UNSPECIFIED
};

/**
* \brief Stream insertion operator.
*
* \param os the stream
* \param standard the PHY standard
* \returns a reference to the stream
*/
inline std::ostream& operator<< (std::ostream& os, WifiPhyStandard standard)
{
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211a:
      return (os << "802.11a");
    case WIFI_PHY_STANDARD_80211b:
      return (os << "802.11b");
    case WIFI_PHY_STANDARD_80211g:
      return (os << "802.11g");
    case WIFI_PHY_STANDARD_80211p:
      return (os << "802.11p");
    case WIFI_PHY_STANDARD_80211n:
      return (os << "802.11n");
    case WIFI_PHY_STANDARD_80211ac:
      return (os << "802.11ac");
    case WIFI_PHY_STANDARD_80211ax:
      return (os << "802.11ax");
    case WIFI_PHY_STANDARD_UNSPECIFIED:
    default:
      return (os << "UNSPECIFIED");
    }
}

/**
 * \ingroup wifi
 * Identifies the MAC specification that a Wifi device is configured to use.
 */
enum WifiMacStandard
{
  WIFI_MAC_STANDARD_80211,
  WIFI_MAC_STANDARD_80211n,
  WIFI_MAC_STANDARD_80211ac,
  WIFI_MAC_STANDARD_80211ax
};

/**
* \brief Stream insertion operator.
*
* \param os the stream
* \param standard the MAC standard
* \returns a reference to the stream
*/
inline std::ostream& operator<< (std::ostream& os, WifiMacStandard standard)
{
  switch (standard)
    {
    case WIFI_MAC_STANDARD_80211:
      return (os << "802.11");
    case WIFI_MAC_STANDARD_80211n:
      return (os << "802.11n");
    case WIFI_MAC_STANDARD_80211ac:
      return (os << "802.11ac");
    case WIFI_MAC_STANDARD_80211ax:
      return (os << "802.11ax");
    default:
      return (os << "UNSPECIFIED");
    }
}
/**
 * \ingroup wifi
 * Identifies the IEEE 802.11 specifications that a Wifi device can be configured to use.
 */
enum WifiStandard
{
    WIFI_STANDARD_UNSPECIFIED,
    WIFI_STANDARD_80211a,
    WIFI_STANDARD_80211b,
    WIFI_STANDARD_80211g,
    WIFI_STANDARD_80211p,
    WIFI_STANDARD_80211n,
    WIFI_STANDARD_80211n_2_4GHZ,
    WIFI_STANDARD_80211n_5GHZ,
    WIFI_STANDARD_80211ac,
    WIFI_STANDARD_80211ad,
    WIFI_STANDARD_80211ax,
    WIFI_STANDARD_80211ax_2_4GHZ,
    WIFI_STANDARD_80211ax_5GHZ,
    WIFI_STANDARD_80211ax_6GHZ,
    WIFI_STANDARD_80211be
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param standard the standard
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, WifiStandard standard)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211a:
        return (os << "802.11a");
    case WIFI_STANDARD_80211b:
        return (os << "802.11b");
    case WIFI_STANDARD_80211g:
        return (os << "802.11g");
    case WIFI_STANDARD_80211p:
        return (os << "802.11p");
    case WIFI_STANDARD_80211n:
        return (os << "802.11n");
    case WIFI_STANDARD_80211n_2_4GHZ:
      return (os << "802.11n-2.4GHz");
    case WIFI_STANDARD_80211n_5GHZ:
      return (os << "802.11n-5GHz");
    case WIFI_STANDARD_80211ac:
        return (os << "802.11ac");
    case WIFI_STANDARD_80211ad:
        return (os << "802.11ad");
    case WIFI_STANDARD_80211ax:
        return (os << "802.11ax");
    case WIFI_STANDARD_80211ax_2_4GHZ:
        return (os << "802.11ax-2.4GHz");
    case WIFI_STANDARD_80211ax_5GHZ:
         return (os << "802.11ax-5GHz");
    case WIFI_STANDARD_80211ax_6GHZ:
        return (os << "802.11ax-6GHz");
    case WIFI_STANDARD_80211be:
        return (os << "802.11be");
    default:
        return (os << "UNSPECIFIED");
    }
}

/**
 * \brief map a given standard configured by the user to the allowed PHY bands
 */
const std::map<WifiStandard, std::list<WifiPhyBand>> wifiStandards = {
    {WIFI_STANDARD_80211a, {WIFI_PHY_BAND_5GHZ}},
    {WIFI_STANDARD_80211b, {WIFI_PHY_BAND_2_4GHZ}},
    {WIFI_STANDARD_80211g, {WIFI_PHY_BAND_2_4GHZ}},
    {WIFI_STANDARD_80211p, {WIFI_PHY_BAND_5GHZ}},
    {WIFI_STANDARD_80211n, {WIFI_PHY_BAND_2_4GHZ, WIFI_PHY_BAND_5GHZ}},
    {WIFI_STANDARD_80211ac, {WIFI_PHY_BAND_5GHZ}},
    {WIFI_STANDARD_80211ad, {WIFI_PHY_BAND_60GHZ}},
    {WIFI_STANDARD_80211ax, {WIFI_PHY_BAND_2_4GHZ, WIFI_PHY_BAND_5GHZ, WIFI_PHY_BAND_6GHZ}},
    {WIFI_STANDARD_80211be, {WIFI_PHY_BAND_2_4GHZ, WIFI_PHY_BAND_5GHZ, WIFI_PHY_BAND_6GHZ}},
};

/**
 * \ingroup wifi
 * \brief Enumeration of frequency channel types
 */
enum FrequencyChannelType : uint8_t
{
    WIFI_PHY_DSSS_CHANNEL = 0,
    WIFI_PHY_OFDM_CHANNEL,
    WIFI_PHY_80211p_CHANNEL
};

/**
 * Get the type of the frequency channel for the given standard
 *
 * \param standard the standard
 * \return the type of the frequency channel for the given standard
 */
inline FrequencyChannelType
GetFrequencyChannelType(WifiStandard standard)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211b:
        return WIFI_PHY_DSSS_CHANNEL;
    case WIFI_STANDARD_80211p:
        return WIFI_PHY_80211p_CHANNEL;
    default:
        return WIFI_PHY_OFDM_CHANNEL;
    }
}

/**
 * Get the default channel width for the given PHY standard and band.
 *
 * \param standard the given standard
 * \param band the given PHY band
 * \return the default channel width (MHz) for the given standard
 */
inline uint16_t
GetDefaultChannelWidth(WifiStandard standard, WifiPhyBand band)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211b:
        return 22;
    case WIFI_STANDARD_80211p:
        return 10;
    case WIFI_STANDARD_80211ac:
        return 80;
    case WIFI_STANDARD_80211ad:
        return 2160;
    case WIFI_STANDARD_80211ax:
    case WIFI_STANDARD_80211be:
        return (band == WIFI_PHY_BAND_2_4GHZ ? 20 : 80);
    default:
        return 20;
    }
}

/**
 * Get the default PHY band for the given standard.
 *
 * \param standard the given standard
 * \return the default PHY band for the given standard
 */
inline WifiPhyBand
GetDefaultPhyBand(WifiStandard standard)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211p:
    case WIFI_STANDARD_80211a:
    case WIFI_STANDARD_80211ac:
    case WIFI_STANDARD_80211ax:
    case WIFI_STANDARD_80211be:
        return WIFI_PHY_BAND_5GHZ;
    case WIFI_STANDARD_80211ad:
        return WIFI_PHY_BAND_60GHZ;
    default:
        return WIFI_PHY_BAND_2_4GHZ;
    }
}

inline WifiPhyBand GetDefaultPhyBand (WifiPhyStandard standard)
{
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211p:
    case WIFI_PHY_STANDARD_80211a:
    case WIFI_PHY_STANDARD_80211ac:
    case WIFI_PHY_STANDARD_80211ax:
      return WIFI_PHY_BAND_5GHZ;
    default:
      return WIFI_PHY_BAND_2_4GHZ;
    }
}

inline uint16_t GetDefaultChannelWidth (WifiPhyStandard standard, WifiPhyBand band)
{
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211b:
      return 22;
    case WIFI_PHY_STANDARD_80211p:
      return 10;
    case WIFI_PHY_STANDARD_80211ac:
      return 80;
    case WIFI_PHY_STANDARD_80211ax:
      return (band == WIFI_PHY_BAND_2_4GHZ ? 20 : 80);
    default:
      return 20;
    }
}

inline FrequencyChannelType GetFrequencyChannelType (WifiPhyStandard standard)
{
  switch (standard)
    {
      case WIFI_PHY_STANDARD_80211b:
        return WIFI_PHY_DSSS_CHANNEL;
      case WIFI_PHY_STANDARD_80211p:
        return WIFI_PHY_80211p_CHANNEL;
      default:
        return WIFI_PHY_OFDM_CHANNEL;
    }
}

inline uint16_t GetMaximumChannelWidth (WifiPhyStandard standard)
{
  switch (standard)
    {
      case WIFI_PHY_STANDARD_80211b:
        return 22;
      case WIFI_PHY_STANDARD_80211p:
        return 10;
      case WIFI_PHY_STANDARD_80211a:
      case WIFI_PHY_STANDARD_80211g:
        return 20;
      case WIFI_PHY_STANDARD_80211n:
        return 40;
      case WIFI_PHY_STANDARD_80211ac:
      case WIFI_PHY_STANDARD_80211ax:
        return 160;
      default:
        NS_ABORT_MSG ("Unknown standard: " << standard);
        return 0;
    }
}

} // namespace ns3

#endif /* WIFI_STANDARD_H */
