#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/string.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/node.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

/**
 *
 *           Network Topology
 *          Wifi 192.168.1.0/24
 *      *    *    *     *    *     *
 *      |    |    |     |    |     |
 *      n4   n3   n2   nAP   n0    n1
 *
 *
 *
 *  Fatto da: Team 25
 *  matricole:
 *      - 1946083
 *      - 1962183
 *      - 1931976
 *      - 1943235
 *
 * ---
 * Best RngRun option is 303. This way no packets will get lost due to distance.
 * ---
 *
 *
 *  In this network there are:
 *  -  UDPEcho Server on n0, port 21
 *  -  UDPEcho Client on n3, sends 2 packets to n0 ad 2s and 4s
 *  -  UDPEcho Client on n4, sends 2 packets to n0 at 1s and 4s
 *     - Packet trace active on node n4
 *
 *  (packet size 512bytes)
 *
 **/

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE("HW2_Task2_Team_25");

int main(int argc, char* argv[]){
    bool useRtsCts = false, useNetAnim = false, verbose = false;
    
    // default SSID
    string ssid_name = "TLC2022";

    CommandLine cmd(__FILE__);
    cmd.AddValue("useRtsCts", "Forces the Network to use Handshake RTS/CTS", useRtsCts);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("useNetAnim", "Generates the NetAnim files", useNetAnim);
    cmd.AddValue("ssid", "Set the SSID of the Network, by default is TLC2022.\nDefault settings for team 25: \"7783477\"", ssid_name);

    cmd.Parse(argc, argv);

    if (verbose){
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    //node container for the 5 station node
    NodeContainer StationContainer; StationContainer.Create(5);

    NodeContainer ApContainer; ApContainer.Create(1);

    Ptr<Node> nAP = ApContainer.Get(0);
    Ptr<Node> n0 = StationContainer.Get(0);
    Ptr<Node> n1 = StationContainer.Get(1);
    Ptr<Node> n2 = StationContainer.Get(2);
    Ptr<Node> n3 = StationContainer.Get(3);
    Ptr<Node> n4 = StationContainer.Get(4);

    string state = "off";
    if(useRtsCts){
        // With this threshold set to 0, every packet will use Rts and Cts
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("100"));
        state = "on";
    }


    YansWifiChannelHelper channel = YansWifiChannelHelper::Default(); //channel helper
    YansWifiPhyHelper phy; //physical helper
    phy.SetChannel(channel.Create());  //installing channel on physical layer

    
    WifiHelper wifi; 
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    wifi.SetStandard(ns3::WifiStandard::WIFI_STANDARD_80211g);

    WifiMacHelper mac; //mac helper
    /**
     * Create and initialize the service set identifier
     * The name will be its Wifi identifyer for the terminals connecting
    **/
    Ssid ssid = Ssid(ssid_name);

    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(true));
    apDevices = wifi.Install(phy, mac, ApContainer);  

    NetDeviceContainer staDevices;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false)); //set mac for stationNodes
    staDevices = wifi.Install(phy, mac, StationContainer); 
    /**
     *
     *      MAC addresses:
     *  nAp 00:00:00:00:00:01
     *  n0  00:00:00:00:00:02
     *  n1  00:00:00:00:00:03
     *  n2  00:00:00:00:00:04
     *  n3  00:00:00:00:00:05
     *  n4  00:00:00:00:00:06
     *
    **/

    //setting mobility

    MobilityHelper Mobility;
   
    Mobility.SetPositionAllocator("ns3::GridPositionAllocator", 
                                "MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0),
                                "DeltaX", DoubleValue(5.0), "DeltaY", DoubleValue(10.0),
                                "GridWidth", UintegerValue(3), "LayoutType", StringValue("RowFirst"));

    Mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(-90, 90, -90, 90)));

    Mobility.Install(StationContainer);

    Mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Mobility.Install(ApContainer);

    //installing the Stackhelper on every node

    InternetStackHelper stack;
    stack.Install(NodeContainer::GetGlobal());
    /**
     * Same as:
     * stack.Install(StationContainer);
     * stack.Install(ApContainer);
    **/

    //Ipv4
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "/24");
    address.Assign(apDevices); address.Assign(staDevices);

    Ipv4InterfaceContainer StaIpv4 = address.Assign(staDevices);
    Ipv4InterfaceContainer ApIpv4 = address.Assign(apDevices);
    /**
     *      IPs:
     *  nAp 192.168.1.1
     *  n0  192.168.1.2
     *  n1  192.168.1.3
     *  n2  192.168.1.4
     *  n3  192.168.1.5
     *  n4  192.168.1.6
     *
    */

    Ipv4Address n0_address = StaIpv4.GetAddress(0);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t portnumber = 21;
    UdpEchoServerHelper echoServer(portnumber);

    ApplicationContainer serverApps = echoServer.Install(n0); //installing server on node n0
    // Since start and stop of the server it's not given, the entire simulation time will be used
    serverApps.Start(Seconds(0.0)); serverApps.Stop(Seconds(7.0));

    UdpEchoClientHelper n3_Client(n0_address, portnumber);      // n3 sends to n0 (by its ipv4 address)
    n3_Client.SetAttribute("MaxPackets",UintegerValue(2));
    n3_Client.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    n3_Client.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer n3_ClientApp = n3_Client.Install(n3);
    n3_ClientApp.Start(Seconds(2.0));
    n3_ClientApp.Stop(Seconds(5.0));

    UdpEchoClientHelper n4_Client(n0_address, portnumber);      // n4 sends to n0 (by its ipv4 address)
    n4_Client.SetAttribute("MaxPackets",UintegerValue(2));
    n4_Client.SetAttribute("Interval", TimeValue(Seconds(3.0)));
    n4_Client.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer n4_ClientApp = n4_Client.Install(n4);
    n4_ClientApp.Start(Seconds(1.0)); n4_ClientApp.Stop(Seconds(5.0));
    
    //configuring pcap

    string pcap_name="task2-" + state + "-" + to_string(StationContainer.Get(4)->GetId()) + ".pcap";
    phy.EnablePcap(pcap_name, staDevices.Get(3), true, true);  //pcap for n4
    pcap_name="task2-" + state + "-" + to_string(ApContainer.Get(0)->GetId()) + ".pcap";
    phy.EnablePcap(pcap_name, apDevices.Get(0), true, true);  //pcap for AP (Id=5)
    
    //netanim

    AnimationInterface anim("wireless-task2-rts-"+ state + ".xml");
    if(useNetAnim) {
        //N0 EchoServer
        string n0_Anim_Name = "SRV-" + to_string(StationContainer.Get(0)->GetId());
        anim.UpdateNodeDescription (n0, n0_Anim_Name);
        anim.UpdateNodeColor (n0, 255, 0, 0);   //RED

        //N3 Echoclient
        string n3_Anim_Name = "CLI-" + to_string(StationContainer.Get(3)->GetId());
        anim.UpdateNodeDescription (n3, n3_Anim_Name);
        anim.UpdateNodeColor (n3, 0, 255, 0); //GREEN

        //N4 Echoclient
        string n4_Anim_Name = "CLI-" + to_string(StationContainer.Get(4)->GetId());
        anim.UpdateNodeDescription (n4, n4_Anim_Name);
        anim.UpdateNodeColor (n4, 0, 255, 0); //GREEN

        //N1 node
        string n1_Anim_Name = "STA-" + to_string(StationContainer.Get(1)->GetId());
        anim.UpdateNodeDescription (n1, n1_Anim_Name);
        anim.UpdateNodeColor (n1, 0, 0, 255); //BLUE

        //N2 node
        string n2_Anim_Name = "STA-" + to_string(StationContainer.Get(2)->GetId());
        anim.UpdateNodeDescription (n2, n2_Anim_Name);
        anim.UpdateNodeColor (n2, 0, 0, 255); //BLUE

        //AP
        anim.UpdateNodeDescription(nAP, "AP");
        anim.UpdateNodeColor(nAP, 66, 49, 137);  //DARK PURPLE

        anim.EnableQueueCounters(Seconds(0),Seconds(7)); // Useful since the delay matters in the queue
        anim.EnablePacketMetadata(); // Optional
        anim.EnableIpv4RouteTracking("task2-" + state + "-routingtable-wireless.xml",
            Seconds(0),     // Start
            Seconds(7),     // Finish
            Seconds(0.25)); // Interval
        anim.EnableWifiMacCounters(Seconds(0), Seconds(7)); 
        anim.EnableWifiPhyCounters(Seconds(0), Seconds(7));
    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(7.0));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
