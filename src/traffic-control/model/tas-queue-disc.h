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

#ifndef SRC_TRAFFIC_CONTROL_MODEL_TAS_QUEUE_DISC_H_
#define SRC_TRAFFIC_CONTROL_MODEL_TAS_QUEUE_DISC_H_

#define TOTAL_QOS_TAGS 8 //needs to be a power of 2

#include "ns3/queue-disc.h"
#include "ns3/data-rate.h"
#include <array>
#include <vector>
#include <math.h>
#include <utility>
#include <algorithm>
#include <stdexcept>

namespace ns3 {
/*
 * Map of active queues
 * true = active
 * false = deactive
 */
typedef std::array<bool,TOTAL_QOS_TAGS> QostagsMap;
/*
 * a tas schedule
 * params
 * duration:    how long this schedule is active
 * qosTagsMap:  Map of witch queues are open (active)
 * startOffset: delay to open any queue
 * stopOffset:  how much earlier the queue closes
 */
typedef struct TasSchedule{
  Time duration;
  QostagsMap qostagsMap;
  Time startOffset;
  Time stopOffset;
  TasSchedule(Time duration, QostagsMap qostagsMap, Time startOffset, Time stopOffset){

    this->duration = duration;
    this->startOffset = startOffset;
    this->stopOffset = stopOffset;

    for(unsigned int i = 0; i < TOTAL_QOS_TAGS; i++){
      this->qostagsMap[i] = qostagsMap[i];
    }

    if(startOffset>duration-stopOffset)
    {
      throw std::out_of_range("Start offset is greater then duration minus stop offset, resulting in an negative opening time");
    }
  }
}TasSchedule;
/*
 * a collection of tas schedules
 */
typedef struct TasConfig{
  std::vector<TasSchedule> list;
  TasConfig(){
    this->list.clear();
  }
  TasConfig(std::vector<TasSchedule> list){
    this->list.clear();
    for(unsigned int i = 0; i < list.size(); i++){
      this->list.push_back(list.at(i));
    }
  }
  void addSchedule(TasSchedule schedule){
    if(!schedule.duration.IsZero()){
       this->list.push_back(schedule);
     }
  }
  void addSchedule(Time duration, QostagsMap gatemap, Time startOffset = Time(0), Time stopOffset = Time(0)){
    this->addSchedule(TasSchedule(duration,gatemap,startOffset,stopOffset));
  }
}TasConfig;

class TasQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief TasQueueDisc constructor
   *
   */
  TasQueueDisc ();

  virtual ~TasQueueDisc();

  // Reasons for dropping packets
  static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";  //!< Packet dropped due to queue disc limit exceeded

private:

  struct lookUpElement{
    std::vector<Time> openTimes;
    std::vector<Time> closesTimes;
    std::vector<uint32_t> indexVector;
    int myLastIndex;
    lookUpElement()
    {
      myLastIndex = 0;
    }
    void add(Time opens, Time closes, uint32_t index)
    {
      openTimes.push_back(opens);
      closesTimes.push_back(closes);
      indexVector.push_back(index);
    }
    void remove(uint32_t index)
    {
      if( index <  openTimes.size() && index < closesTimes.size() && index < indexVector.size())
      {
        openTimes.erase(openTimes.begin() + index);
        closesTimes.erase(closesTimes.begin() + index);
        indexVector.erase(indexVector.begin() + index);
      }
    }
    void clear(void)
    {
      openTimes.clear();
      closesTimes.clear();
      indexVector.clear();
    }
  };

  virtual void InitializeParams (void);

  virtual bool CheckConfig (void);
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);

  /*
   * gets the next queue to dequeue
   */
  std::pair<int32_t, Time>GetDequeueQueue();
  /*
   * gets the current time from given timesource callback
   * if not provided returns simulation time
   */
  virtual Time GetDeviceTime(); //Gets the current time TODO connect Node device Time to Queue Disc
  /*
   * calculates the time until queue (x) opens
   * returns the Time and a negative Time on error
   */
  virtual Time TimeUntileQueueOpens(int qostag);
  /*
   * schedule run event for an queue
   */
  void ScheduleRun(uint32_t queue, Time eventTime); //
  /*
   * gets the biggest or equal entry to provided data
   */
  int32_t GetNextBiggerEntry(std::vector<Time> vector, Time data);

  /* parameters for the TAS Queue Disc */
  bool m_trustQostag;       //Trust the Qostag of incoming Packets or sort new
  DataRate m_linkBandwidth; //Link Data Rate
  TasConfig m_tasConfig;    //My Configuration
  Callback <Time> m_getNow; //Time source callback if not set uses simulation time

  /* variables stored by TAS Queue Disc */
  Time m_cycleLength;                                     //The total length of all schedules combined
  std::array<EventId,TOTAL_QOS_TAGS> m_eventSchedulePlan; //LookUpTable for all queues if an run event is planned
  std::array<lookUpElement,TOTAL_QOS_TAGS> m_queueLookUp; //LookUpTable for schedules includes 3 std vectors with open, close and index
};

/**
 * Serialize the TasConfig to the given ostream
 */
std::ostream &operator << (std::ostream &os, const QostagsMap &qostagsMap);
std::ostream &operator << (std::ostream &os, const TasSchedule &tasSchedule);
std::ostream &operator << (std::ostream &os, const TasConfig &tasConfig);

/**
 * Serialize from the given istream to this TasConfig.
 */
std::istream &operator >> (std::istream &is, QostagsMap &qostagsMap);
std::istream &operator >> (std::istream &is, TasSchedule &tasSchedule);
std::istream &operator >> (std::istream &is, TasConfig &tasConfig);

ATTRIBUTE_HELPER_HEADER (TasConfig);
};// namespace ns3

#endif /* SRC_TRAFFIC_CONTROL_MODEL_TAS_QUEUE_DISC_H_ */
