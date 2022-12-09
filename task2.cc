#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/basic-energy-source.h"
#include "ns3/node.h"
#include "string.h"
// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *    *     *
//  |    |    |    |    |     |
// n4   n3   n2   nAP    n0    n1
//                   
/*



*/
using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE("HW2_Task2_Team_25");

int main(int argc, char* argv[])
{
    bool verbose = false;
    bool useRtsCts = false;
    bool useNetAnim = true; //default false
    string ssid_name= "TLC2022";

    CommandLine cmd(__FILE__);
    cmd.AddValue("useRtsCts", "Forces the Network to use Handshake RTS/CTS", useRtsCts);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("useNetAnim", "Generates the NetAnim files", useNetAnim);
    cmd.AddValue("ssid", "Set the SSID of the Network", ssid_name);

    cmd.Parse(argc, argv);

    if (verbose){
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
    

    NodeContainer StationContainer;  //node container for the 5 station node
    StationContainer.Create(5);

    NodeContainer ApContainer;
    ApContainer.Create(1);

    Ptr<Node> nAP = ApContainer.Get(0);
    Ptr<Node> n0 = StationContainer.Get(0);
    Ptr<Node> n1 = StationContainer.Get(1);
    Ptr<Node> n2 = StationContainer.Get(2);
    Ptr<Node> n3 = StationContainer.Get(3);
    Ptr<Node> n4 = StationContainer.Get(4);
    
    InternetStackHelper stack;
    stack.Install(StationContainer);
    stack.Install(ApContainer);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default(); //channel helper
    YansWifiPhyHelper phy; //physical helper
    phy.SetChannel(channel.Create());  //installing channel on physical layer

    WifiHelper wifi;  
    wifi.SetStandard(ns3::WifiStandard::WIFI_STANDARD_80211g);

    WifiMacHelper mac; //mac helper
    Ssid ssid=Ssid(ssid_name);  //create and initialize the service set identifier, the name that identifies a Wifi connection with its users
    
    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(true));
    apDevices = wifi.Install(phy, mac, ApContainer);  //installa il mac sull'access point

    NetDeviceContainer staDevices;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false)); //set mac for stationNodes
    staDevices = wifi.Install(phy, mac, StationContainer); //installa nel contenitore di dispositivi, lo strato fisico e il mac sul contenitore di
    //nodi
    /*
        mac addresses:
        nAp 00:00:00:00:00:01
        n0  00:00:00:00:00:02
        n1  00:00:00:00:00:03
        n2  00:00:00:00:00:04
        n3  00:00:00:00:00:05
        n4  00:00:00:00:00:06
    */
    
    //Ipv4 
    Ipv4AddressHelper address;

    address.SetBase("192.168.1.0", "/24");
    address.Assign(apDevices);
    address.Assign(staDevices);
    Ipv4InterfaceContainer StaIpv4=address.Assign(staDevices);
    Ipv4InterfaceContainer ApIpv4=address.Assign(apDevices);

    /*
       ips: 
       nAp 192.168.1.1
       n0  192.168.1.2
       n1  192.168.1.3
       n2  192.168.1.4
       n3  192.168.1.5
       n4  192.168.1.6
    */

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t portnumber=21;
    UdpEchoServerHelper echoServer(portnumber); 

    ApplicationContainer serverApps = echoServer.Install(n0); //installing server on node n0
    serverApps.Start(Seconds(0.0)); //non è specificato
    serverApps.Stop(Seconds(7.0)); //non è specificato

    UdpEchoClientHelper n3_Client(StaIpv4.GetAddress(0), portnumber); //n3 -> n0
    n3_Client.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    n3_Client.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer n3_ClientApp = n3_Client.Install(n3);
    n3_ClientApp.Start(Seconds(2.0));
    n3_ClientApp.Stop(Seconds(5.0));

    UdpEchoClientHelper n4_Client(StaIpv4.GetAddress(0), portnumber); //n4 -> n0
    n4_Client.SetAttribute("Interval", TimeValue(Seconds(3.0)));
    n4_Client.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer n4_ClientApp = n4_Client.Install(n4);
    n4_ClientApp.Start(Seconds(1.0));
    n4_ClientApp.Stop(Seconds(6.0));

    
    //RIVEDERE

    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=90]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=90]"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(-90, 90, -90, 90)));
    mobility.Install(StationContainer);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ApContainer);
    

    string rtslimit ="1000";//ogni pacchetto minore di questo valore sarà trasmesso senza rts/cts
    string state="off";
    if(useRtsCts){
        rtslimit="0"; //quindi se lo mettiamo a zero tutti i pacchetti saranno trasmessi con rts/cts
        state="on";
    }
    
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue(rtslimit));

    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.EnablePcap("task2-nAP.pcap", apDevices.Get(0), true, true);
    phy.EnablePcap("task2-n4.pcap", staDevices.Get(3),true,true);

    if(!useNetAnim){    
        NS_LOG_INFO("Run Simulation.");
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        Simulator::Destroy();
        NS_LOG_INFO("Done.");
    }
    else{
        AnimationInterface::SetConstantPosition(nAP, 45, 45);
        string pack_name="wireless-task2-rts-"+state+ ".xml";
        AnimationInterface anim(pack_name);
        uint32_t  nId_array[5];
        for(int i=0; i<5;i++){
            int n=StationContainer.Get(i)->GetId();
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
        anim.UpdateNodeColor (n3, 0, 255, 0); //GREEN

        //N4 Echoclient
        string n4_ID =to_string(nId_array[4]);
        string n4_Anim_Name= "CLI-"+n4_ID;

        anim.UpdateNodeDescription (n4, n4_Anim_Name);
        anim.UpdateNodeColor (n4, 0, 255, 0); //GREEN

        //N1 node
        string n1_ID =to_string(nId_array[1]);
        string n1_Anim_Name= "STA-"+n1_ID;

        anim.UpdateNodeDescription (n1, n1_Anim_Name);
        anim.UpdateNodeColor (n1, 0, 0, 255); //BLUE

        //N2 node
        string n2_ID =to_string(nId_array[2]);
        string n2_Anim_Name= "STA-"+n2_ID;

        anim.UpdateNodeDescription (n2, n2_Anim_Name);
        anim.UpdateNodeColor (n2, 0, 0, 255); //BLUE

        //AP
        anim.UpdateNodeDescription(nAP, "AP"); 
        anim.UpdateNodeColor(nAP, 66, 49, 137);  //DARK PURPLE

    
        anim.EnableQueueCounters(Seconds(0),Seconds(10)); //serve per il ritardo perché conta la coda
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
}
