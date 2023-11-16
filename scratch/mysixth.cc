/*
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
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"


// Default Network Topology
//
//   Wifi 10.1.4.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0                          10.1.2.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4 ----------------n8    n9    n10
//                   point-to-point  |    |    |    |  point-to-point
//                                   ================
//                                     LAN 10.1.3.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThirdScriptExample");

int
main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t first_nCsma = 2;
    uint32_t second_nCsma = 3;
    uint32_t nWifi = 3;
    bool tracing = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", first_nCsma);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", second_nCsma);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    // The underlying restriction of 18 is due to the grid position
    // allocator's configuration; the grid layout will exceed the
    // bounding box if more than 18 nodes are provided.
    if (nWifi > 18)
    {
        std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box"
                  << std::endl;
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    //创建两对p2p节点
    NodeContainer first_p2pNodes;
    first_p2pNodes.Create(2);

    NodeContainer second_p2pNodes;
    second_p2pNodes.Create(2);


    //配置p2p信道延迟及数据传输速率
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate",StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay",StringValue("2ms"));

    //安装p2p网卡到p2p网络节点
    NetDeviceContainer first_p2pDevices;
    first_p2pDevices = pointToPoint.Install(first_p2pNodes);

    NetDeviceContainer second_p2pDevices;
    second_p2pDevices = pointToPoint.Install(second_p2pNodes);
    
    //两个csma网络创建
    NodeContainer first_csmaNodes;
    first_csmaNodes.Add(first_p2pNodes.Get(1));
    first_csmaNodes.Add(second_p2pNodes.Get(0));
    first_csmaNodes.Create(first_nCsma);

    NodeContainer second_csmaNodes;
    second_csmaNodes.Add(second_p2pNodes.Get(1));
    second_csmaNodes.Create(second_nCsma);

    //配置csma节点的传输速率及延迟
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate",StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay",TimeValue(NanoSeconds(6560)));

    //安装csma网卡到两个csma网络中的节点
    NetDeviceContainer first_csmaDevices;
    first_csmaDevices = csma.Install(first_csmaNodes);

    NetDeviceContainer second_csmaDevices;
    second_csmaDevices = csma.Install(second_csmaNodes);

    //wifi网络Sta节点的创建
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    //wifi网络AP节点的选择
    NodeContainer wifiAPNode = first_p2pNodes.Get(1);
    
    //使用默认的PHY配置和通信信道
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    //MAC层配置
    WifiMacHelper mac;
    Ssid ssid = Ssid("ns3-ssid");

    //在wifi节点上安装wifi模型
    WifiHelper wifi;
    //为sta节点安装wifi模型
    NetDeviceContainer staDevices;
    mac.SetType("ns3::StaWifiMac","Ssid",SsidValue(ssid),"ActiveProbing",BooleanValue(false));
    staDevices = wifi.Install(phy,mac,wifiStaNodes);
    //为AP节点安装wifi模型
    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, wifiAPNode);

    //配置移动模型
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifiStaNodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiAPNode);

    //安装协议栈
    InternetStackHelper stack;
    stack.Install(first_p2pNodes.Get(0));
    stack.Install(second_csmaNodes);
    stack.Install(first_csmaNodes);
    stack.Install(wifiStaNodes);

    //为设备接口分配IP地址
    Ipv4AddressHelper address;
    //为两个10.1.1.0网段的p2p设备分配IP
    address.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer first_p2pInterfaces;
    first_p2pInterfaces = address.Assign(first_p2pDevices);

    //为两个10.1.2.0网段的p2p设备分配IP
    address.SetBase("10.1.2.0","255.255.255.0");
    Ipv4InterfaceContainer second_p2pInterfaces;
    second_p2pInterfaces = address.Assign(second_p2pDevices);

    //为4个10.1.3.0网段的csma(first)设备分配IP
    address.SetBase("10.1.3.0","255.255.255.0");
    Ipv4InterfaceContainer first_csmaInterfaces;
    first_csmaInterfaces = address.Assign(first_csmaDevices);

    //为4个10.1.4.0网段的csma(second)设备分配IP
    address.SetBase("10.1.4.0","255.255.255.0");
    Ipv4InterfaceContainer second_csmaInterfaces;
    second_csmaInterfaces = address.Assign(second_csmaDevices);

    //为4个10.1.5.0网段的wifi设备分配IP
    address.SetBase("10.1.5.0","255.255.255.0");
    Ipv4InterfaceContainer wifistaInterfaces;
    wifistaInterfaces = address.Assign(staDevices);//为何wifi设备不需要接口容器
    address.Assign(apDevices);

    //安装应用端
    //配置服务器端
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(wifiStaNodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));
    
    //配置客户端
    UdpEchoClientHelper echoClient(wifistaInterfaces.GetAddress(0),9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(wifiStaNodes.Get(1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(10.0));

    if (tracing)
    {
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        pointToPoint.EnablePcapAll("mysixth");
        phy.EnablePcap("mysixth", apDevices.Get(0));
        csma.EnablePcap("mysixth", first_csmaDevices.Get(0), true);
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;

}