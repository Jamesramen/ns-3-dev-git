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
 * TasConfig
 * params
 * duration:    how long this schedule is active
 * qosTagsMap:  Map of witch queues are open (active)
 * startOffset: delay to open any queue
 * stopOffset:  how much earlier the queue closes
 */
typedef struct TasConfig{
  /*
   * Map of active queues
   * true = active
   * false = deactive
   */
  typedef std::array<bool,TOTAL_QOS_TAGS> QostagsMap;
  struct QueueSchedule{
      std::vector<Time> openTimes;
      std::vector<Time> closesTimes;
      int myLastIndex;
      QueueSchedule()
      {
        myLastIndex = 0;
      }
      void add(Time opens, Time closes)
      {
        unsigned int vectorSize = closesTimes.size();

        if(vectorSize>0)
        {
          if(closesTimes.at(vectorSize-1) == opens)
          {
            closesTimes.at(vectorSize-1) = closes;
            return;
          }
        }
        openTimes.push_back(opens);
        closesTimes.push_back(closes);
      }
      void remove(uint32_t index)
      {
        if( index <  openTimes.size() && index < closesTimes.size())
        {
          openTimes.erase(openTimes.begin() + index);
          closesTimes.erase(closesTimes.begin() + index);
        }
      }
      void clear(void)
      {
        openTimes.clear();
        closesTimes.clear();
      }
    };
  std::array<QueueSchedule,TOTAL_QOS_TAGS> queueSchedulePlan;
  Time cycleLength;
  TasConfig(){
    for(int i = 0; i < TOTAL_QOS_TAGS; i++)
    {
      queueSchedulePlan[i].clear();
    }
    cycleLength = Time(0);
  }
  void addSchedule(Time duration, QostagsMap qostagsMap, Time startOffset = Time(0), Time stopOffset = Time(0)){
    Time timeWindowOpensAt =  cycleLength + startOffset;
    Time timeWindowClosesAt =  cycleLength + duration - stopOffset;

    for(unsigned int queueIndex = 0; queueIndex < TOTAL_QOS_TAGS; queueIndex++)
    {
     if(qostagsMap[queueIndex])
     {
       queueSchedulePlan[queueIndex].add(timeWindowOpensAt,timeWindowClosesAt);
     }
    }

    cycleLength += duration;
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

  /* variables stored by TAS Queue Disc */                                  //The total length of all schedules combined
  std::array<EventId,TOTAL_QOS_TAGS> m_eventSchedulePlan; //LookUpTable for all queues if an run event is planned
};

/**
 * Serialize the TasConfig to the given ostream
 */
std::ostream &operator << (std::ostream &os, const TasConfig::QostagsMap &qostagsMap);
std::ostream &operator << (std::ostream &os, const TasConfig::QueueSchedule &queueSchedule);
std::ostream &operator << (std::ostream &os, const TasConfig &tasConfig);

/**
 * Serialize from the given istream to this TasConfig.
 */
std::istream &operator >> (std::istream &is, TasConfig::QostagsMap &qostagsMap);
std::istream &operator >> (std::istream &is, TasConfig::QueueSchedule &queueSchedule);
std::istream &operator >> (std::istream &is, TasConfig &tasConfig);

ATTRIBUTE_HELPER_HEADER (TasConfig);
};// namespace ns3

#endif /* SRC_TRAFFIC_CONTROL_MODEL_TAS_QUEUE_DISC_H_ */
