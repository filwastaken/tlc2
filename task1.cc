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
    Ptr<Node> n1 = WifiContainer.Get(0);
    Ptr<Node> n2 = WifiContainer.Get(1);
    Ptr<Node> n3 = WifiContainer.Get(2);
    Ptr<Node> n4 = WifiContainer.Get(3);

    // Installing the StackHelper on every node
    InternetStackHelper stack;
    stack.Install(WifiContainer); // or stack.Install(NodeContainer::GetGlobal());

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default(); //channel helper
    YansWifiPhyHelper phy; //physical helper
    phy.SetChannel(channel.Create());  //installing channel on physical layer

    // Using WifiHelper
    WifiHelper wifi = WifiHelper();
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    wifi.SetStandard(ns3::WifiStandard::WIFI_STANDARD_80211b);

    // MAC helper
    WifiMacHelper mac; mac.SetType("ns3::AdhocWifiMac");

    // Installing wifi on the netcontainer
    NetDeviceContainer WifiDevices = wifi.Install(phy, mac, WifiContainer);
    
    for(int i=0; i<5; i++) cout << "Mac nodo" << i << " e':"<< WifiDevices.Get(i)->GetAddress()<<endl;
    /**
     *
     *      MAC addresses:
     *  n0  00:00:00:00:00:02
     *  n1  00:00:00:00:00:03
     *  n2  00:00:00:00:00:04
     *  n3  00:00:00:00:00:05
     *  n4  00:00:00:00:00:06
     *
     *
    */

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
        "X", ("ns3::UniformRandomVariable[Min=-90|Max=90]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=-90|Max=90]")
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
     * nAp 192.168.1.1
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
    n3_Client.SetAttribute("MaxPackets", UintegerValue(1));
    n3_Client.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    n3_Client.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer n3_ClientApp = n3_Client.Install(n3);
    n3_ClientApp.Start(Seconds(2.0));
    n3_ClientApp.Stop(Seconds(5.0));

    UdpEchoClientHelper n4_Client(n0_address, port);            // n4 sends to n0 (by its ipv4 address)
    n4_Client.SetAttribute("MaxPackets", UintegerValue(1));
    n4_Client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    n4_Client.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer n4_ClientApp = n4_Client.Install(n4);
    n4_ClientApp.Start(Seconds(1.0));
    n4_ClientApp.Stop(Seconds(2.0)); //TODO: Check if the second packet gets sent


    // TODO : Check after this

    string rtslimit ="1000";//ogni pacchetto minore di questo valore sarà trasmesso senza rts/cts
    string state="off";
    if(useRtsCts){
        rtslimit="0"; //quindi se lo mettiamo a zero tutti i pacchetti saranno trasmessi con rts/cts
        state="on";
    }
    
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue(rtslimit));

    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.EnablePcap("task2-n4.pcap", WifiDevices.Get(3),true,true);

    if(!useNetAnim){    
        NS_LOG_INFO("Run Simulation.");
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        Simulator::Destroy();
        NS_LOG_INFO("Done.");
    }
    else{
        string pack_name="wireless-task1-rts-"+state+ ".xml";
        AnimationInterface anim(pack_name);
        uint32_t  nId_array[5];
        for(int i=0; i<5;i++){
            int n=WifiContainer.Get(i)->GetId();
            nId_array[i]=n;
        }

        //N0 EchoServer
        string n0_ID =to_string(nId_array[0]);
        string n0_Anim_Name="SRV-" + n0_ID;
    
        anim.UpdateNodeDescription (n0, n0_Anim_Name);
        anim.UpdateNodeColor (n0, 255, 0, 0);   //RED
    
        //N3 Echoclient
        string n3_ID =to_string(nId_array[3]);
        string n3_Anim_Name= "CLI-"+n3_ID;

        anim.UpdateNodeDescription (n3, n3_Anim_Name);
        anim.UpdateNodeColor (n3, 0, 255, 0); 

        //N4 Echoclient
        string n4_ID =to_string(nId_array[4]);
        string n4_Anim_Name= "CLI-"+n4_ID;

        anim.UpdateNodeDescription (n4, n4_Anim_Name);
        anim.UpdateNodeColor (n4, 0, 255, 0); //GREEN

        //N1 node
        string n1_ID =to_string(nId_array[1]);
        string n1_Anim_Name= "STA-"+n1_ID;

        anim.UpdateNodeDescription (n1, n1_Anim_Name);
        anim.UpdateNodeColor (n1, 0, 0, 255); 

        //N2 node
        string n2_ID =to_string(nId_array[2]);
        string n2_Anim_Name= "STA-"+n2_ID;

        anim.UpdateNodeDescription (n2, n2_Anim_Name);
        anim.UpdateNodeColor (n2, 0, 0, 255); //BLUE

        anim.EnablePacketMetadata(); // Optional
        anim.EnableIpv4RouteTracking("routingtable-wireless.xml",
                                 Seconds(0),  //inizio
                                 Seconds(5),   //fine
                                 Seconds(0.25));     //intervallo    
        anim.EnableWifiMacCounters(Seconds(0), Seconds(10)); // Optional
        anim.EnableWifiPhyCounters(Seconds(0), Seconds(10)); // Optional
        
        NS_LOG_INFO("Run Simulation.");
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        Simulator::Destroy();
        NS_LOG_INFO("Done.");
    }

    return 0;
    return 0;
}
