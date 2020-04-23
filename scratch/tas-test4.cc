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
 *
 * This simulation shows how the tas-queue-disc can be configured and used
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/tsn-module.h"
#include "ns3/traffic-control-module.h"

#include "stdio.h"
#include <array>
#include <string>
#include <chrono>

#define NUMBER_OF_NODES 10
#define DATA_PAYLOADE_SIZE 100
#define TRANSMISON_SPEED "50Mbps"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Tas-test-2");

Time callbackfunc();

int32_t ipv4PacketFilter(Ptr<QueueDiscItem> item);

int
main (int argc, char *argv[])
{
//  Config::SetDefault ("ns3::CsmaNetDevice::EncapsulationMode", StringValue ("Dix"));
  
  CommandLine cmd;
  cmd.Parse (argc, argv);
  NodeContainer nodes;
  nodes.Create (NUMBER_OF_NODES);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue (TRANSMISON_SPEED));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  NetDeviceContainer devices = csma.Install(nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Time::SetResolution (Time::NS);
  Time sendPeriod,scheduleDuration,simulationDuration;
  scheduleDuration = Seconds(1);
  simulationDuration = 6*10*scheduleDuration;
  sendPeriod = scheduleDuration;
  sendPeriod -= MicroSeconds(1);

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  CallbackValue timeSource = MakeCallback(&callbackfunc);

  TsnHelper tsnHelperServer,tsnHelperClient;
  TasConfig schedulePlanServer,schedulePlanClient;

  for (int i = 0; i < 10; i++)
  {
    schedulePlanClient.addSchedule(scheduleDuration,{1,1,1,1,1,1,1,1});
    schedulePlanClient.addSchedule(scheduleDuration,{0,0,0,0,0,0,0,0});

    schedulePlanServer.addSchedule(scheduleDuration,{0,0,0,0,0,0,0,0});
    schedulePlanServer.addSchedule(scheduleDuration,{1,1,1,1,1,1,1,1});
  }

  tsnHelperClient.SetRootQueueDisc("ns3::TasQueueDisc", "TasConfig", TasConfigValue(schedulePlanClient), "TimeSource", timeSource,"DataRate", StringValue (TRANSMISON_SPEED));
  tsnHelperClient.AddPacketFilter(0,"ns3::TsnIpv4PacketFilter","Classify",CallbackValue(MakeCallback(&ipv4PacketFilter)));

  tsnHelperServer.SetRootQueueDisc("ns3::TasQueueDisc", "TasConfig", TasConfigValue(schedulePlanServer), "TimeSource", timeSource,"DataRate", StringValue (TRANSMISON_SPEED));

  QueueDiscContainer qdiscsClient1 = tsnHelperClient.Install (devices.Get(0));
  QueueDiscContainer qdiscsServer = tsnHelperServer.Install (devices.Get(1));

  Ipv4AddressHelper address;

  address.SetBase ("0.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  Ipv4Address multicastSource ("10.1.1.1");
  Ipv4Address multicastGroup ("225.1.2.4");

  UdpEchoClientHelper echoClient1 (interfaces.GetAddress (1), 9);

  echoClient1.SetAttribute ("MaxPackets", UintegerValue (simulationDuration.GetInteger()/sendPeriod.GetInteger()));
  echoClient1.SetAttribute ("Interval", TimeValue (sendPeriod));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (DATA_PAYLOADE_SIZE));

  ApplicationContainer clientApps1 = echoClient1.Install (nodes.Get (0));

  clientApps1.Start (Seconds(0));
  clientApps1.Stop (simulationDuration);


  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));

  serverApps.Start (Seconds (0));
  serverApps.Stop (simulationDuration);


  csma.EnablePcapAll("tas-test4");

  std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
  Simulator::Run ();
  std::chrono::time_point<std::chrono::high_resolution_clock> stop = std::chrono::high_resolution_clock::now();
  Simulator::Destroy ();
  std::cout << 2 << " Nodes " << std::endl;
  std::cout << "Total simulated Time: "<< simulationDuration << std::endl;
  std::cout << "Expectated number of packages in pcap: " << 2*simulationDuration.GetInteger()/sendPeriod.GetInteger() << std::endl;
  std::cout << "Schedule duration: "<< scheduleDuration <<" Execution Time " << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count() << " ms" << std::endl;
  return 0;
}

int32_t ipv4PacketFilter(Ptr<QueueDiscItem> item){
  return 4;
}

Time callbackfunc(){
  return Simulator::Now();
}
