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

ATTRIBUTE_HELPER_CPP (TasConfig);

std::ostream &
operator << (std::ostream &os, const QostagsMap &qostagsMap){

  if( (unsigned int) qostagsMap.size() != TOTAL_QOS_TAGS ){
    NS_FATAL_ERROR ("Incomplete schedule QostagsMap specification (" << (unsigned int) qostagsMap.size() << " values provided, " << TOTAL_QOS_TAGS << " required)");
  }

  for(int i = 0; i < TOTAL_QOS_TAGS; i++){
   os << qostagsMap[i] << " ";
  }

  return os;
}
std::ostream &
operator << (std::ostream &os, const TasSchedule &tasSchedule){
  os << tasSchedule.duration;
  os << tasSchedule.qostagsMap;
  os << tasSchedule.startOffset;
  os << tasSchedule.stopOffset;
  return os;
}
std::ostream &
operator << (std::ostream &os, const TasConfig &tasConfig)
{

  if(!tasConfig.list.empty()){
    for(unsigned int i = 0; i < (unsigned int)tasConfig.list.size() -1; i++ ){
      os << tasConfig.list.at(i);
    }
  }
  os << 0; // End character
  return os;
}

std::istream &
operator >> (std::istream &is, QostagsMap &qostagsMap){
  for (int i = 0; i < TOTAL_QOS_TAGS; i++)
   {
     if (!(is >> qostagsMap[i]))
     {
       NS_FATAL_ERROR ("Incomplete QostagsMap specification (" << i << " values provided, " << TOTAL_QOS_TAGS << " required)");
     }
   }
  return is;
}
std::istream &
operator >> (std::istream &is, TasSchedule &tasSchedule){
  is >> tasSchedule.duration;
  is >> tasSchedule.qostagsMap;
  is >> tasSchedule.startOffset;
  is >> tasSchedule.stopOffset;
  return is;
}
std::istream &
operator >> (std::istream &is, TasConfig &tasConfig){

  unsigned int i = 0;
  tasConfig.list.clear();

  while(is.peek()){
    if(!(is >> tasConfig.list.at(i))){
      NS_FATAL_ERROR ("Unspecified fatal error tasConfig input stream");
    }
  }

  if(i == 0){
    NS_FATAL_ERROR ("Incomplete tasConfig specification (" << i << "values provided, 1 required");
  }

  return is;
}

TypeId TasQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TasQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<TasQueueDisc> ()
    .AddAttribute ("TasConfig",
                   "The TasConfig for the Time Aware Shaper",
                   TasConfigValue(),
                   MakeTasConfigAccessor(&TasQueueDisc::m_tasConfig),
                   MakeTasConfigChecker()
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
   .AddAttribute ("DataRate",
                  "DataRate of conected link",
                  DataRateValue (DataRate ("1.5Mbps")),
                  MakeDataRateAccessor (&TasQueueDisc::m_linkBandwidth),
                  MakeDataRateChecker ())
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

Ptr<const QueueDiscItem>
TasQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  std::pair <uint32_t, Time> nextQueue = this->GetDequeueQueue();

  if(nextQueue.second.IsPositive()){
    return this->GetInternalQueue(nextQueue.first)->Peek();
  }
  else
  {
    NS_LOG_LOGIC ("Queue empty");
    return 0;
  }
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
     childqueue = qosTag.GetPriority () & (TOTAL_QOS_TAGS*2-1); // Max queue is TOTAL_QOS_TAGS
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

Ptr<QueueDiscItem>
TasQueueDisc::DoDequeue (void)
{

  NS_LOG_FUNCTION (this);

  std::pair<int32_t, Time> nextQueue = this->GetDequeueQueue();

  if(nextQueue.second.IsZero())
  {
    return GetInternalQueue(nextQueue.first)->Dequeue(); ;
  }
  else if(nextQueue.first > -1)
  {
    ScheduleRun(nextQueue.first, nextQueue.second);
  }
  return 0;
}

std::pair<int32_t, Time>
TasQueueDisc::GetDequeueQueue()
{
  //No valid schedule configured
  if(m_cycleLength.IsZero())
  {
    for(int i = TOTAL_QOS_TAGS-1; i > -1 ; i--)
    {
      if(! (GetInternalQueue (i)->IsEmpty()) )
      {
        return std::make_pair(i,Time(0));
      }
    }
  }

  Time tempTimeIndex,foundTimeIndex = m_cycleLength;

  std::vector<Time> fastestOpenTimes;
  int32_t queueToOpen = -1;

  //Goes through all Tags and gets the next execution time
  for(int i = TOTAL_QOS_TAGS-1; i > -1 ; i--)
  {
    if(! (GetInternalQueue (i)->IsEmpty()) )
    {
      tempTimeIndex = TimeUntileQueueOpens(i);
      if(tempTimeIndex.IsPositive() && foundTimeIndex > tempTimeIndex)
      {
        foundTimeIndex = tempTimeIndex;
        queueToOpen = i;
      }
    }
  }
  //Returns the next queue to open and when
  return std::make_pair(queueToOpen,foundTimeIndex);
}

Time
TasQueueDisc::TimeUntileQueueOpens(int qostag)
{

  if(qostag < 0 || qostag > TOTAL_QOS_TAGS-1){
    return Time(-1);
  }

  if(m_tasConfig.list.size() == 0)
   {
     return Time(0); // No schudles configured
   }

   if(m_queueLookUp[qostag].openTimes.size() == 0 || m_queueLookUp[qostag].closesTimes.size() == 0)
   {
     return Time(-1); //Queue will never Open
   }

  Time simulationTime = GetDeviceTime();

  //Calculate Transmison time result is in Picoseconds
  double bitRate(1.0/m_linkBandwidth.GetBitRate());
  int paketBitSize = 8*GetInternalQueue(qostag)->Peek()->GetSize();
  double result = bitRate*paketBitSize*1000*1000*1000;

  Time transmitionTime = PicoSeconds(result);
  Time relativeNow( simulationTime.GetInteger() % m_cycleLength.GetInteger()); // Relative Time in schedule

  relativeNow += transmitionTime; // to compensate the transmission time
  int32_t vectorPosition = GetNextBiggerEntry(m_queueLookUp[qostag].closesTimes, relativeNow);
  relativeNow -= transmitionTime;

  if(vectorPosition < 0)
  {
    return Time(-1);
  }

  m_queueLookUp[qostag].myLastIndex = vectorPosition;

  if(m_queueLookUp[qostag].openTimes[vectorPosition] <= relativeNow)
  {
    if(!(vectorPosition == 0 && m_queueLookUp[qostag].closesTimes[vectorPosition] < relativeNow + transmitionTime))
    {
      return Time(0);
    }
    return m_queueLookUp[qostag].openTimes[vectorPosition] - relativeNow + m_cycleLength;
  }

  return m_queueLookUp[qostag].openTimes[vectorPosition] - relativeNow;
}

int32_t
TasQueueDisc::GetNextBiggerEntry(std::vector<Time> vector, Time data){

  if(vector.size() == 0)
  {
    return -1;
  }

  typename std::vector<Time>::iterator itr = vector.begin();

  for(; itr < vector.end(); itr++)
  {
    if( *(itr) > data)
    {
      return itr - vector.begin();
    }
  }

  return 0;
}

Time
TasQueueDisc::GetDeviceTime(){

  if(!m_getNow.IsNull()){
    return m_getNow();
  }
  return Simulator::Now();
}

void
TasQueueDisc::ScheduleRun(uint32_t queue, Time eventTime)
{
  if(m_eventSchedulePlan[queue].IsExpired())
  {
    m_eventSchedulePlan[queue] = Simulator::Schedule(eventTime,&TasQueueDisc::Run, this);
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

        QueueSize maxSize(GetMaxSize().GetUnit(),GetMaxSize().GetValue()/TOTAL_QOS_TAGS);
        for(int i = 0; i < TOTAL_QOS_TAGS; i++)
            {
              AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> >
                              ("MaxSize", QueueSizeValue (maxSize)));
            }
      }
  if (GetNInternalQueues () != TOTAL_QOS_TAGS)
    {
      NS_LOG_ERROR ("PieQueueDisc needs " << TOTAL_QOS_TAGS << "  internal queues");
      return false;
    }

  return true;
}

void
TasQueueDisc::InitializeParams (void)
{
  //Creating look up table
  std::vector<TasSchedule>::iterator itr;

  Time currentOffset = Time(0);
  for(itr = m_tasConfig.list.begin(); itr < m_tasConfig.list.end(); itr++)
  {
    Time timeWindowOpensAt =  currentOffset + itr->startOffset;
    Time timeWindowClosesAt =  currentOffset + itr->duration - itr->stopOffset;
    currentOffset += itr->duration;

    for(unsigned int queueIndex = 0; queueIndex < TOTAL_QOS_TAGS; queueIndex++)
    {
      if(itr->qostagsMap[queueIndex])
      {
        m_queueLookUp[queueIndex].add(timeWindowOpensAt,timeWindowClosesAt,itr - m_tasConfig.list.begin());
      }
    }
  }
  m_cycleLength = currentOffset;
  NS_LOG_FUNCTION (this);
}

} // namespace ns3
