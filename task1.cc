#include "ns3/basic-energy-source.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
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
 *          Network Topology 
 * 
 *          Wifi 192.168.1.0
 *      *    *    *    *     *
 *      |    |    |    |     |
 *      n4   n3   n2   n0    n1
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
 * 
 * ---
 * Best RngRun option is 002. This way no packets will get lost due to distance.
 * ---
 * 
 * 
 *  In this network there are:
 *  -  UDPEcho Server on n0, port 20
 *  -  UDPEcho Client on n3, sends 2 packets to n0 ad 2s and 4s
 *  -  UDPEcho Client on n4, sends 2 packets to n0 at 1s and 2s
 *     - Packet trace active on node n2
 * 
 *  (packet size 512bytes)
 * 
 **/

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE("HW2_Task1_Team_25");

int main(int argc, char* argv[]){
    bool useRtsCts = false;
    bool useNetAnim = true;
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("useRtsCts", "Forces the Network to use Handshake RTS/CTS", useRtsCts);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("useNetAnim", "Generates the NetAnim files", useNetAnim);
    cmd.Parse(argc, argv);

    NodeContainer WifiContainer;  // Node container for the 5 terminals
    WifiContainer.Create(5);

    Ptr<Node> n0 = WifiContainer.Get(0);
    Ptr<Node> n1 = WifiContainer.Get(1);
    Ptr<Node> n2 = WifiContainer.Get(2);
    Ptr<Node> n3 = WifiContainer.Get(3);
    Ptr<Node> n4 = WifiContainer.Get(4);

    // Installing the StackHelper on every node
    InternetStackHelper stack;
    stack.Install(WifiContainer); // or stack.Install(NodeContainer::GetGlobal());

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default(); //channel helper
    YansWifiPhyHelper phy; //physical helper
    phy.SetChannel(channel.Create());  //installing channel on physical layer

    // Using WifiHelper
    WifiHelper wifi = WifiHelper();
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    wifi.SetStandard(ns3::WifiStandard::WIFI_STANDARD_80211g);

    // MAC helper
    WifiMacHelper mac; mac.SetType("ns3::AdhocWifiMac");
    /**
     *      MAC addresses:
     *  n0  00:00:00:00:00:01
     *  n1  00:00:00:00:00:02
     *  n2  00:00:00:00:00:03
     *  n3  00:00:00:00:00:04
     *  n4  00:00:00:00:00:05
     *
    **/

    // Installing wifi on the netcontainer
    NetDeviceContainer WifiDevices = wifi.Install(phy, mac, WifiContainer);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=90]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=90]")
    );
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(-90, 90, -90, 90))
    );
    mobility.Install(WifiContainer);

    //Ipv4 
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "/24");
    address.Assign(WifiDevices);

    Ipv4InterfaceContainer Ipv4Interface = address.Assign(WifiDevices);
    /**
     *
     * Ipv4 addresses:
     * n0  192.168.1.1
     * n1  192.168.1.2
     * n2  192.168.1.3
     * n3  192.168.1.4
     * n4  192.168.1.5
     *
     **/

    Ipv4Address n0_address = Ipv4Interface.GetAddress(0);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 20;
    UdpEchoServerHelper echoServer(port); 

    ApplicationContainer serverApps = echoServer.Install(n0);
    // Since start and stop are not given, it is being set as the entire simulation time
    serverApps.Start(Seconds(0.0)); serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper n3_Client(n0_address, port);            // n3 sends to n0 (by its ipv4 address)
    n3_Client.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    n3_Client.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer n3_ClientApp = n3_Client.Install(n3);
    n3_ClientApp.Start(Seconds(2.0)); n3_ClientApp.Stop(Seconds(5.0));

    UdpEchoClientHelper n4_Client(n0_address, port);            // n4 sends to n0 (by its ipv4 address)
    n4_Client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    n4_Client.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer n4_ClientApp = n4_Client.Install(n4);
    //Second packets gets sent and then stops (before third one)
    n4_ClientApp.Start(Seconds(1.0)); n4_ClientApp.Stop(Seconds(2.5));

    string state = "off";
    if(useRtsCts){
        // With this threshold set to 0, every packet will use Rts and Cts
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("0"));
        state = "on";
    }
    
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.EnablePcap("task1-n2.pcap", WifiDevices.Get(2),true,true);

    if(!useNetAnim){    
        NS_LOG_INFO("Run Simulation.");
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        Simulator::Destroy();
        NS_LOG_INFO("Done.");
    } else{
        string pack_name = "wireless-task1-rts-" + state + ".xml";
        AnimationInterface anim(pack_name);
        uint32_t ids[5];
        for(int i=0; i<5;i++) ids[i] = WifiContainer.Get(i)->GetId();

        //N0 EchoServer
        string n0_Anim_Name = "SRV-" + to_string(ids[0]);
        anim.UpdateNodeDescription(n0, n0_Anim_Name);
        anim.UpdateNodeColor(n0, 255, 0, 0);   //RED
    
        //N3 Echoclient
        string n3_Anim_Name= "CLI-" + to_string(ids[3]);
        anim.UpdateNodeDescription(n3, n3_Anim_Name);
        anim.UpdateNodeColor(n3, 0, 255, 0); // GREEN

        //N4 Echoclient
        string n4_Anim_Name= "CLI-" + to_string(ids[4]);
        anim.UpdateNodeDescription (n4, n4_Anim_Name);
        anim.UpdateNodeColor (n4, 0, 255, 0); //GREEN

        //N1 node
        string n1_Anim_Name= "HOC-" + to_string(ids[1]);
        anim.UpdateNodeDescription (n1, n1_Anim_Name);
        anim.UpdateNodeColor (n1, 0, 0, 255); // BLUE

        //N2 node
        string n2_Anim_Name= "HOC-" + to_string(ids[2]);
        anim.UpdateNodeDescription (n2, n2_Anim_Name);
        anim.UpdateNodeColor (n2, 0, 0, 255); //BLUE

        anim.EnablePacketMetadata(); // Optional
        anim.EnableIpv4RouteTracking("routingtable-wireless.xml",
            Seconds(0),     // Start
            Seconds(5),     // Finish
            Seconds(0.25)); // Interval
        anim.EnableWifiMacCounters(Seconds(0), Seconds(10)); // Optional
        anim.EnableWifiPhyCounters(Seconds(0), Seconds(10)); // Optional
        
        NS_LOG_INFO("Run Simulation.");
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        Simulator::Destroy();
        NS_LOG_INFO("Done.");
    }

    return 0;
}
