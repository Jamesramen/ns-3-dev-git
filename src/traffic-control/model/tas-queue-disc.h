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

namespace ns3 {

typedef std::array<bool,TOTAL_QOS_TAGS> QostagsMap; // Maps witch Queues schuld be opend on the Qos Tag

typedef struct TasSchedule{
  Time duration;
  QostagsMap qostagsMap;
  Time startOffset;
  Time stopOffset;
  Time offset;
  TasSchedule(Time duration, QostagsMap qostagsMap,  Time startOffset = Time(0), Time stopOffset = Time(0)){
    this->duration = duration;
    for(unsigned int i = 0; i < TOTAL_QOS_TAGS; i++){
      this->qostagsMap[i] = qostagsMap[i];
    }
    if(startOffset.IsPositive() && startOffset < duration)
    {
      this->startOffset = startOffset;
      if(stopOffset.IsPositive() && stopOffset < duration - startOffset)
      {
        this->startOffset = stopOffset;
      }
      else
      {
        this->stopOffset = Time(0);
      }
    }
    else
    {
      this->startOffset = Time(0);
      this->stopOffset = Time(0);
    }
    this->offset = Time(0);
  }
}TasSchedule;

typedef struct TasConfig{
  std::vector<TasSchedule> scheduleList;
  Time cycleLength;
  TasConfig(){
    this->cycleLength = Time(0);
    this->scheduleList.clear();
  }
  TasConfig(std::vector<TasSchedule> scheduleList){
    this->cycleLength = Time(0);
    this->scheduleList.clear();
    for(unsigned int i = 0; i < scheduleList.size(); i++){
      this->scheduleList.push_back(scheduleList.at(i));
      this->scheduleList.at(i).offset = this->cycleLength;
      this->cycleLength += scheduleList.at(i).duration;
    }
  }
  void addSchedule(TasSchedule schedule){
    if(!schedule.duration.IsZero()){
       this->scheduleList.push_back(schedule);
       this->cycleLength += schedule.duration;
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

  std::pair<int32_t, Time>GetNextInternelQueueToOpen();

  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);

  virtual Time GetDeviceTime(); //TODO connect Node device Time to Queue Disc
  virtual Time TimeUntileQueueOpens(int qostag); //TODO callc packed transmissionTime

  void SchudleRun(uint32_t queue);

  std::array<EventId,TOTAL_QOS_TAGS> m_eventSchudlerPlan;
  std::array<lookUpElement,TOTAL_QOS_TAGS> m_queueOpenLookUp;

  template<typename T>
  int32_t GetPositionInSortedVector(std::vector<T> vector, T data);

  TasConfig m_tasConfig;
  bool m_trustQostag;

  Callback <Time> m_getNow;
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
