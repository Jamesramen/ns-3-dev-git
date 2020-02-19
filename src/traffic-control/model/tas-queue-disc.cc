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
#include "ns3/simulator.h"
#include "ns3/tag.h"
#include "ns3/pointer.h"
#include "tas-queue-disc.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TasQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (TasQueueDisc);

ATTRIBUTE_HELPER_CPP (SchudlePlan);

std::ostream &
operator << (std::ostream &os, const QostagsMap &qostagsMap){

  if( (unsigned int) qostagsMap.size() != 8 ){
    NS_FATAL_ERROR ("Incomplete schudle QostagsMap specification (" << (unsigned int) qostagsMap.size() << " values provided, 8 required)");
  }

  for(int i = 0; i < 8; i++){
   os << qostagsMap[i] << " ";
  }

  return os;
}
std::ostream &
operator << (std::ostream &os, const TasSchudle &tasSchudle){
  os << tasSchudle.duration;
  os << tasSchudle.qostagsMap;
  os << tasSchudle.startOffset;
  os << tasSchudle.stopOffset;
  return os;
}
std::ostream &
operator << (std::ostream &os, const SchudlePlan &schudlePlan)
{

  if(!schudlePlan.plan.empty()){
    for(unsigned int i = 0; i < (unsigned int)schudlePlan.plan.size() -1; i++ ){
      os << schudlePlan.plan.at(i);
    }
  }
  os << 0; // End character
  return os;
}

std::istream &
operator >> (std::istream &is, QostagsMap &qostagsMap){
  for (int i = 0; i < 8; i++)
   {
     if (!(is >> qostagsMap[i]))
     {
       NS_FATAL_ERROR ("Incomplete QostagsMap specification (" << i << " values provided, 8 required)");
     }
   }
  return is;
}
std::istream &
operator >> (std::istream &is, TasSchudle &tasSchudle){
  is >> tasSchudle.duration;
  is >> tasSchudle.qostagsMap;
  is >> tasSchudle.startOffset;
  is >> tasSchudle.stopOffset;
  return is;
}
std::istream &
operator >> (std::istream &is, SchudlePlan &schudlePlan){

  unsigned int i = 0;
  schudlePlan.plan.clear();
  schudlePlan.length = Time(0);

  while(is.peek()){
    if(!(is >> schudlePlan.plan.at(i))){
      NS_FATAL_ERROR ("Unspecified fatal error schudlePlan input stream");
    }
    schudlePlan.length += schudlePlan.plan.at(i).duration;
  }

  if(i == 0){
    NS_FATAL_ERROR ("Incomplete schudlePlan specification (" << i << "values provided, 1 required");
  }

  return is;
}

TypeId TasQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TasQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<TasQueueDisc> ()
    .AddAttribute ("SchudlePlan",
                   "The SchudlePlan for the Time Aware Shaper",
                   SchudlePlanValue(),
                   MakeSchudlePlanAccessor(&TasQueueDisc::m_schudlePlan),
                   MakeSchudlePlanChecker()
                  )
    .AddAttribute ("TrustQostag",
                    "Defines if the Quality of Service should be trusted",
                    BooleanValue(false),
                    MakeBooleanAccessor(&TasQueueDisc::m_trustQostag),
                    MakeBooleanChecker()
                   )
   .AddAttribute ("MaxSize",
                   "The maximum number of packets accepted by this queue disc",
                   QueueSizeValue (QueueSize ("800p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ()
                   )
   .AddAttribute("TimeSource",
                 "function callback to get Current Time ",
                 CallbackValue (),
                 MakeCallbackAccessor(&TasQueueDisc::m_getNow),
                 MakeCallbackChecker()
             )
  ;
  return tid;
}

TasQueueDisc::TasQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES)
{
  NS_LOG_FUNCTION (this);
}

TasQueueDisc::~TasQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

bool
TasQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  int32_t ret = Classify(item);

  int32_t childqueue = 0; // 0 Best effort Queue

  if (ret == PacketFilter::PF_NO_MATCH)
   {
     NS_LOG_DEBUG ("No filter has been able to classify this packet, using priomap.");

     SocketPriorityTag qosTag;
     if (item->GetPacket ()->PeekPacketTag (qosTag))
       {
       childqueue = qosTag.GetPriority () & 0x07; // Max queue is 8
       }
     else
     {
       qosTag.SetPriority(childqueue);
       item->GetPacket()->AddPacketTag(qosTag);
     }
   }
 else
   {
     NS_LOG_DEBUG ("Packet filters returned " << ret);

     if (ret >= 0 && static_cast<uint32_t>(ret) < GetNInternalQueues () )
       {
       childqueue = ret;
       }
     else
     {
       SocketPriorityTag qosTag;
       qosTag.SetPriority(childqueue);
       item->GetPacket()->AddPacketTag(qosTag);
     }
   }

  bool retval = GetInternalQueue (childqueue)->Enqueue (item);

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (childqueue)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (childqueue)->GetNBytes ());

  return retval;
}

int
TasQueueDisc::GetNextInternelQueueToOpen()
{
  int internelQueue = -1;
  QostagsMap queuesToOpen({0,0,0,0,0,0,0,0});
  int highestQueueToOpen = -1;

  for(unsigned int i = 0; i < 8; i++)
    {
      if(!GetInternalQueue (i)->IsEmpty())
        {
          queuesToOpen[i] = true;
          if(highestQueueToOpen < 0){
            highestQueueToOpen = i;
          }
        }
    }

  if(highestQueueToOpen < 0) // No items in queues
    {
      return internelQueue;
    }

  if(this->m_schudlePlan.plan.size())
  {
      Time simulationTime = GetDeviceTime();
      Time timeIndex( simulationTime.GetInteger() % this->m_schudlePlan.length.GetInteger() );
      Time currentTimeOffset(0);

      for(unsigned int i = 0; i < this->m_schudlePlan.plan.size(); i++)
        {
          currentTimeOffset += this->m_schudlePlan.plan.at(i).duration;
          bool forwardCheck = false;
          bool wrapCheck = false;

          if(timeIndex < currentTimeOffset)
            {
              forwardCheck = true;
            }
          else if(internelQueue < 0)
            {
              wrapCheck = true;
            }

          if(forwardCheck || wrapCheck)
            {
            for(unsigned int prio = GetNInternalQueues(); prio > 0; prio--)
              {
                if(this->m_schudlePlan.plan.at(i).qostagsMap[prio-1] && queuesToOpen[prio-1])
                 {
                 if(forwardCheck)
                   {
                     return prio-1;
                   }
                   internelQueue = prio-1;
                 }
              }
            }
        }
      return internelQueue;
  }

  return highestQueueToOpen;
}

Time
TasQueueDisc::TimeUntileQueueOpens(int qostag)
{

  if(qostag < 0 || qostag > 7){
    return Time(-1);
  }

  Time simulationTime = GetDeviceTime();
  Time timeIndex( simulationTime.GetInteger() % this->m_schudlePlan.length.GetInteger() );
  Time currentTimeOffset(0);
  Time wrappedOpen(0);

  for(unsigned int i = 0; i < this->m_schudlePlan.plan.size(); i++)
   {
      currentTimeOffset += this->m_schudlePlan.plan.at(i).duration;
      bool forwardCheck = false;
      bool wrapCheck = false;

      if(timeIndex < currentTimeOffset)
        {
          forwardCheck = true;
        }
      else if(wrappedOpen.IsZero())
        {
          wrapCheck = true;
        }

      if(forwardCheck || wrapCheck)
        {
           if(m_schudlePlan.plan.at(i).qostagsMap[qostag])
            {
             if(forwardCheck)
             {
               if(timeIndex < currentTimeOffset - m_schudlePlan.plan.at(i).stopOffset) // Check boundries
                 {
                   if(timeIndex >  currentTimeOffset - m_schudlePlan.plan.at(i).duration + m_schudlePlan.plan.at(i).startOffset)
                     {
                       return Time(0);
                     }
                   else // wait until start
                     {
                       return currentTimeOffset - timeIndex - m_schudlePlan.plan.at(i).duration + m_schudlePlan.plan.at(i).startOffset;
                     }
                 } //Queue Closed
              }
             else
               {
                 wrappedOpen = m_schudlePlan.length - timeIndex + m_schudlePlan.plan.at(i).startOffset + currentTimeOffset - m_schudlePlan.plan.at(i).duration ;
               }
            }
        }
   }
  return wrappedOpen;
}

Time
TasQueueDisc::GetDeviceTime(){

  if(!m_getNow.IsNull()){
    return m_getNow();
  }
  return Simulator::Now();
}

Ptr<QueueDiscItem>
TasQueueDisc::DoDequeue (void)
{

  NS_LOG_FUNCTION (this);

  int nextQueue = this->GetNextInternelQueueToOpen();

  if(nextQueue > -1){
    Time timeUntileQueueOpens = TimeUntileQueueOpens(nextQueue);

    if(timeUntileQueueOpens.IsZero())
      {
        return GetInternalQueue(nextQueue)->Dequeue();
      }
    else
      {
        Simulator::Schedule(timeUntileQueueOpens,&TasQueueDisc::Run,this);
      }
  }
  return 0;
}

Ptr<const QueueDiscItem>
TasQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  int nextQueue = this->GetNextInternelQueueToOpen();

  if(nextQueue > -1){
    return this->GetInternalQueue(nextQueue)->Peek();
  }else{
    NS_LOG_LOGIC ("Queue empty");
          return 0;
  }
}

bool
TasQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("TasQueueDisc cannot have classes");
      return false;
    }

  if (GetNInternalQueues () == 0)
      {

        QueueSize maxSize(GetMaxSize().GetUnit(),GetMaxSize().GetValue()/8);
        for(int i = 0; i < 8; i++)
            {
              AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> >
                              ("MaxSize", QueueSizeValue (maxSize)));
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
TasQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3



