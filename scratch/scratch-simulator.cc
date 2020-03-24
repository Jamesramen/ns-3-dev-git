/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/tsn-module.h"

#include "stdio.h"
#include <array>
#include <string>
#include <chrono>

#define NUMBER_OF_NODE_PAIRS 1000

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ScratchSimulator");

Time callbackfunc();

int32_t ipv4PacketFilter(Ptr<QueueDiscItem> item);

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::MS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  CallbackValue timeSource = MakeCallback(&callbackfunc);
  NodeContainer nodes;

  for(int i = 0; i < NUMBER_OF_NODE_PAIRS; i++){
    NodeContainer subNodes;
    subNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install (subNodes);

    InternetStackHelper stack;
    stack.Install (subNodes);

    TsnHelper tsnHelperServer,tsnHelperClient;
    TasConfig schedulePlanServer,schedulePlanClient;

    schedulePlanServer.addSchedule(Seconds(1),{1,1,1,1,1,1,1,1});
    schedulePlanServer.addSchedule(Seconds(1),{0,0,0,0,0,0,0,0});
    schedulePlanServer.addSchedule(Seconds(1),{1,1,1,1,1,1,1,1});
    schedulePlanServer.addSchedule(Seconds(1),{0,0,0,0,0,0,0,0});

    schedulePlanClient.addSchedule(Seconds(1),{0,0,0,1,0,0,0,0});
    schedulePlanClient.addSchedule(Seconds(1),{1,1,1,0,1,1,1,1});
    schedulePlanClient.addSchedule(Seconds(1),{0,0,0,1,0,0,0,0});
    schedulePlanClient.addSchedule(Seconds(1),{1,1,1,0,1,1,1,1});


    tsnHelperServer.SetRootQueueDisc("ns3::TasQueueDisc", "TasConfig", TasConfigValue(schedulePlanServer), "TimeSource", timeSource);
    tsnHelperClient.SetRootQueueDisc("ns3::TasQueueDisc", "TasConfig", TasConfigValue(schedulePlanClient), "TimeSource", timeSource);

    tsnHelperClient.AddPacketFilter(0,"ns3::TsnIpv4PacketFilter","Classify",CallbackValue(MakeCallback(&ipv4PacketFilter)));

    QueueDiscContainer qdiscsServer = tsnHelperServer.Install (devices.Get(0));
    QueueDiscContainer qdiscsClient = tsnHelperClient.Install (devices.Get(1));

    Ipv4AddressHelper address;
    char baseAdress[14];
    unsigned char a = 0,b = 0,c = i%255;

    if(i-c > 0){
      b=(i/255)%255;
    }

    if(i-b*255-c > 0){
      a = (i/65025)%255;
    }

    sprintf(baseAdress,"%i.%i.%i.0",a,b,c);
    address.SetBase (baseAdress, "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    UdpEchoServerHelper echoServer (9);
    UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);

    echoClient.SetAttribute ("MaxPackets", UintegerValue (99999));
    echoClient.SetAttribute ("Interval", TimeValue (MilliSeconds(200)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (subNodes.Get (0));
    clientApps.Start (Seconds (0));
    clientApps.Stop (Seconds (9));

    ApplicationContainer serverApps = echoServer.Install (subNodes.Get (1));
    serverApps.Start (Seconds (0));
    serverApps.Stop (Seconds (10.0));

    nodes.Add(subNodes);
  }

//  tch0.AddPacketFilter(0,"ns3::TsnIpv4PacketFilter","Classify",CallbackValue(MakeCallback(&ipv4PacketFilter)));
//  pointToPoint.EnablePcapAll ("scratch-simulator");
  std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
  Simulator::Run ();
  std::chrono::time_point<std::chrono::high_resolution_clock> stop = std::chrono::high_resolution_clock::now();
  Simulator::Destroy ();
  std::cout << NUMBER_OF_NODE_PAIRS*2 << " Nodes " <<"Execution Time " << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count() << " ms" << std::endl;
  return 0;
}

int32_t ipv4PacketFilter(Ptr<QueueDiscItem> item){
  return 4;
}

Time callbackfunc(){
  return Simulator::Now();
}
