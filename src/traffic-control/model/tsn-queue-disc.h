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

#ifndef SRC_TRAFFIC_CONTROL_MODEL_TSN_QUEUE_DISC_H_
#define SRC_TRAFFIC_CONTROL_MODEL_TSN_QUEUE_DISC_H_

#include "ns3/queue-disc.h"
#include <array>
#include <vector>

namespace ns3 {

typedef std::array<bool,8> QostagsMap; // Maps witch Queues schuld be opend on the Qos Tag

typedef struct TsnSchudle{
  Time duration;
  QostagsMap qostagsMap;
  Time startOffset;
  Time stopOffset;
  TsnSchudle(Time duration, QostagsMap qostagsMap,  Time startOffset = Time(0), Time stopOffset = Time(0)){
    this->duration = duration;
    for(unsigned int i = 0; i < 8; i++){
      this->qostagsMap[i] = qostagsMap[i];
    }
    this->startOffset = startOffset;
    this->stopOffset = stopOffset;
  }
}TsnSchudle;

typedef struct SchudlePlan{
  std::vector<TsnSchudle> plan;
  Time length;
  SchudlePlan(){
    this->length = Time(0);
      this->plan.clear();
  }
  SchudlePlan(std::vector<TsnSchudle> plan){
    this->length = Time(0);
    this->plan.clear();
    for(unsigned int i = 0; i < plan.size(); i++){
      this->plan.push_back(plan.at(i));
      this->length += plan.at(i).duration;
    }
  }
  void addSchudle(TsnSchudle schudle){
    if(!schudle.duration.IsZero()){
       this->plan.push_back(schudle);
       this->length += schudle.duration;
     }
  }
}SchudlePlan;

typedef std::function<Time(void)> TimeSourceCallback;

/**
 * \ingroup traffic-control
 *
 * .
 *
 */

class TsnQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief TsnQueueDisc constructor
   *
   * Creates a queue with a depth of 1000 packets by default
   */
  TsnQueueDisc ();

  virtual ~TsnQueueDisc();

  // Reasons for dropping packets
  static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";  //!< Packet dropped due to queue disc limit exceeded

private:

  virtual void InitializeParams (void);
  virtual void SetTimeSource(TimeSourceCallback func); //TODO

  virtual bool CheckConfig (void);
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);

  virtual int GetNextInternelQueueToOpen();

  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);

  virtual Time GetDeviceTime();//TODO
  virtual Time TimeUntileQueueOpens(int qostag);//TODO callc packed transmissionTime

  virtual TimeSourceCallback GetTimeSource();

  std::array<Time,8> m_dequeEventList;
  SchudlePlan m_schudlePlan;
  bool m_trustQostag;
  TimeSourceCallback m_timeSourceCallback;
};

/**
 * Serialize the SchudlePlan to the given ostream
 *
 * \param os
 * \param priomap
 *
 * \return std::ostream
 */
std::ostream &operator << (std::ostream &os, const QostagsMap &qostagsMap);
std::ostream &operator << (std::ostream &os, const TsnSchudle &tsnSchudle);
std::ostream &operator << (std::ostream &os, const SchudlePlan &schudlePlan);

/**
 * Serialize from the given istream to this SchudlePlan.
 *
 * \return std::istream
 */
std::istream &operator >> (std::istream &is, QostagsMap &qostagsMap);
std::istream &operator >> (std::istream &is, TsnSchudle &tsnSchudle);
std::istream &operator >> (std::istream &is, SchudlePlan &schudlePlan);

ATTRIBUTE_HELPER_HEADER (SchudlePlan);
ATTRIBUTE_HELPER_HEADER (TimeSourceCallback);
};// namespace ns3

#endif /* SRC_TRAFFIC_CONTROL_MODEL_TSN_QUEUE_DISC_H_ */
