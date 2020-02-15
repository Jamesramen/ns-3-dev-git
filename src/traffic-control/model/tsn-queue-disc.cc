/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 
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
 * Author: Luca Wenlding <lwendlin@rhrk.uni-kl.de>
 */


#include "ns3/queue.h"
#include "ns3/socket.h"

#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/drop-tail-queue.h"

#include "tsn-queue-disc.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TsnQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (TsnQueueDisc);

ATTRIBUTE_HELPER_CPP (SchudlePlan);

std::ostream &
operator << (std::ostream &os, const GateMap &gateMap){

  if( (unsigned int) gateMap.size() != 8 ){
    NS_FATAL_ERROR ("Incomplete schudle gateMap specification (" << (unsigned int) gateMap.size() << " values provided, 8 required)");
  }

  for(int i = 0; i < 8; i++){
   os << gateMap[i] << " ";
  }

  return os;
}
std::ostream &
operator << (std::ostream &os, const TsnSchudle &tsnSchudle){
  os << tsnSchudle.duration;
  os << tsnSchudle.gateMap;
  os << tsnSchudle.startOffset;
  os << tsnSchudle.stopOffset;
  return os;
}
std::ostream &
operator << (std::ostream &os, const SchudlePlan &schudlePlan)
{

  if(!schudlePlan.empty()){
    for(unsigned int i = 0; i < (unsigned int)schudlePlan.size() -1; i++ ){
      os << schudlePlan.at(1);
    }
  }

  os << 0; // End character

  return os;
}

std::istream &
operator >> (std::istream &is, GateMap &gateMap){
  for (int i = 0; i < 8; i++)
   {
     if (!(is >> gateMap[i]))
     {
       NS_FATAL_ERROR ("Incomplete gateMap specification (" << i << " values provided, 8 required)");
     }
   }
  return is;
}
std::istream &
operator >> (std::istream &is, TsnSchudle &TsnSchudle){
  is >> TsnSchudle.duration;
  is >> TsnSchudle.gateMap;
  is >> TsnSchudle.startOffset;
  is >> TsnSchudle.stopOffset;
  return is;
}
std::istream &
operator >> (std::istream &is, SchudlePlan &schudlePlan){

  unsigned int i = 0;

  while(is.peek()){
    if(!(is >> schudlePlan.at(i++))){
      NS_FATAL_ERROR ("Unspecified fatal error schudlePlan input stream");
    }
  }

  if(i == 0){
    NS_FATAL_ERROR ("Incomplete schudlePlan specification (" << i << "values provided, 1 required");
  }

  return is;
}

TypeId TsnQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TsnQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<TsnQueueDisc> ()
    .AddAttribute ("SchudlePlan",
                   "The SchudlePlan for the Time Aware Shaper",
                   SchudlePlanValue( SchudlePlan(1, TsnSchudle(ns3::Time(1000),GateMap{1,1,1,1,1,1,1,1}))),
                   MakeSchudlePlanAccessor(&TsnQueueDisc::m_schudlePlan),
                   MakeSchudlePlanChecker()
                  )
  ;
  return tid;
}

TsnQueueDisc::TsnQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES)
{
  NS_LOG_FUNCTION (this);
}

TsnQueueDisc::~TsnQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

bool
TsnQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  if (GetCurrentSize () + item > GetMaxSize ())
    {
      NS_LOG_LOGIC ("Queue full -- dropping pkt");
      DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP);
      return false;
    }

  bool retval = GetInternalQueue (0)->Enqueue (item);

  // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
  // internal queue because QueueDisc::AddInternalQueue sets the trace callback

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return retval;
}

Ptr<QueueDiscItem>
TsnQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item = GetInternalQueue (0)->Dequeue ();

  if (!item)
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  return item;
}

Ptr<const QueueDiscItem>
TsnQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<const QueueDiscItem> item = GetInternalQueue (0)->Peek ();

  if (!item)
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  return item;
}

bool
TsnQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("TsnQueueDisc cannot have classes");
      return false;
    }

  if (GetNInternalQueues () == 0)
      {
    for(int i = 0; i < 8; i++)
        {
          AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> >
                          ("MaxSize", QueueSizeValue (GetMaxSize ())));
        }
      }
  if (GetNInternalQueues () != 8)
    {
      NS_LOG_ERROR ("PieQueueDisc needs 8 internal queues");
      return false;
    }

  return true;
}

void
TsnQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3



