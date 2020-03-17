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
#include <array>
#include <vector>

namespace ns3 {

typedef std::array<bool,TOTAL_QOS_TAGS> QostagsMap; // Maps witch Queues schuld be opend on the Qos Tag

typedef struct TasSchedule{
  Time duration;
  QostagsMap qostagsMap;
  Time startOffset;
  Time stopOffset;
  TasSchedule(Time duration, QostagsMap qostagsMap,  Time startOffset = Time(0), Time stopOffset = Time(0)){
    this->duration = duration;
    for(unsigned int i = 0; i < TOTAL_QOS_TAGS; i++){
      this->qostagsMap[i] = qostagsMap[i];
    }
    this->startOffset = startOffset;
    this->stopOffset = stopOffset;
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
    TasSchedule schedule(duration,gatemap,startOffset,stopOffset);
    if(!schedule.duration.IsZero()){
         this->scheduleList.push_back(schedule);
         this->cycleLength += schedule.duration;
       }
    }
}TasConfig;

/**
 * \ingroup traffic-control
 *
 * .
 *
 */

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
   * Creates a queue with a depth of 1000 packets by default
   */
  TasQueueDisc ();

  virtual ~TasQueueDisc();

  // Reasons for dropping packets
  static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";  //!< Packet dropped due to queue disc limit exceeded

private:

  virtual void InitializeParams (void);

  virtual bool CheckConfig (void);
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);

  virtual int GetNextInternelQueueToOpen();

  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);

  virtual Time GetDeviceTime();//TODO connect Node device Time to Queue Disc
  virtual Time TimeUntileQueueOpens(int qostag);//TODO callc packed transmissionTime

  void SchudleRun(uint32_t queue);
  void SchudleCallBack(uint32_t queue);

  std::array<EventId,TOTAL_QOS_TAGS> queuesToBeOpend;

  TasConfig m_tasConfig;
  bool m_trustQostag;

  Callback <Time> m_getNow;
};

/**
 * Serialize the TasConfig to the given ostream
 *
 * \param os
 * \param priomap
 *
 * \return std::ostream
 */
std::ostream &operator << (std::ostream &os, const QostagsMap &qostagsMap);
std::ostream &operator << (std::ostream &os, const TasSchedule &tasSchedule);
std::ostream &operator << (std::ostream &os, const TasConfig &tasConfig);

/**
 * Serialize from the given istream to this TasConfig.
 *
 * \return std::istream
 */
std::istream &operator >> (std::istream &is, QostagsMap &qostagsMap);
std::istream &operator >> (std::istream &is, TasSchedule &tasSchedule);
std::istream &operator >> (std::istream &is, TasConfig &tasConfig);

ATTRIBUTE_HELPER_HEADER (TasConfig);
};// namespace ns3

#endif /* SRC_TRAFFIC_CONTROL_MODEL_TAS_QUEUE_DISC_H_ */
