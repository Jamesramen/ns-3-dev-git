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

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  TsnHelper tch0,tch1;

  SchudlePlan schudlePlan0;

  schudlePlan0.addSchudle(Seconds(1),{1,1,1,1,0,1,1,1});
  schudlePlan0.addSchudle(Seconds(1),{0,0,0,0,1,0,0,0});
  schudlePlan0.addSchudle(Seconds(1),{1,1,1,1,1,1,1,1});
  schudlePlan0.addSchudle(Seconds(1),{0,0,0,0,0,0,0,0});

  SchudlePlan schudlePlan1;

  schudlePlan1.addSchudle(Seconds(1),{0,0,0,0,0,0,0,0});
  schudlePlan1.addSchudle(Seconds(1),{1,1,1,1,1,1,1,1});
  schudlePlan1.addSchudle(Seconds(1),{0,0,0,0,0,0,0,0});
  schudlePlan1.addSchudle(Seconds(1),{1,1,1,1,1,1,1,1});


  tch0.SetRootQueueDisc ("ns3::TasQueueDisc", "SchudlePlan", SchudlePlanValue(schudlePlan0));

  CallbackValue timeSource = MakeCallback(&callbackfunc);

  tch1.SetRootQueueDisc("ns3::TasQueueDisc",
      "SchudlePlan", SchudlePlanValue(schudlePlan1),
      "TimeSource", timeSource
      );

  tch0.AddPacketFilter(0,"ns3::TsnIpv4PacketFilter","Classify",CallbackValue(MakeCallback(&ipv4PacketFilter)));

  QueueDiscContainer qdiscs0 = tch0.Install (devices.Get(0));
  QueueDiscContainer qdiscs1 = tch1.Install (devices.Get(1));

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);


  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));

  serverApps.Start (Seconds (0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (99999));
  echoClient.SetAttribute ("Interval", TimeValue (MilliSeconds(200)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (0));
  clientApps.Stop (Seconds (9));

  pointToPoint.EnablePcapAll ("scratch-simulator");

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

int32_t ipv4PacketFilter(Ptr<QueueDiscItem> item){
  return 4;
}

Time callbackfunc(){
  return Simulator::Now();
}
