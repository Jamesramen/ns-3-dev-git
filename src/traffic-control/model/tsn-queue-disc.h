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

typedef std::array<bool,8> GateMap; // Maps witch Gates schuld be opend bool

typedef struct TsnSchudle{
  ns3::Time duration;
  GateMap gateMap;
  ns3::Time startOffset;
  ns3::Time stopOffset;
  TsnSchudle(ns3::Time duration, GateMap gateMap,  ns3::Time startOffset = ns3::Time(0), ns3::Time stopOffset = ns3::Time(0)){
    this->duration = duration;
    for(unsigned int i = 0; i < 8; i++){
      this->gateMap[i] = gateMap[i];
    }
    this->startOffset = startOffset;
    this->stopOffset = stopOffset;
  }
}TsnSchudle;

typedef std::vector<TsnSchudle> SchudlePlan;

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
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);

//  void SchudleDoDequeue();

//  ns3::Time GetDequeueOffset(Ptr<QueueDiscItem> item);

  SchudlePlan m_schudlePlan;
};

/**
 * Serialize the SchudlePlan to the given ostream
 *
 * \param os
 * \param priomap
 *
 * \return std::ostream
 */
std::ostream &operator << (std::ostream &os, const GateMap &gateMap);
std::ostream &operator << (std::ostream &os, const TsnSchudle &tsnSchudle);
std::ostream &operator << (std::ostream &os, const SchudlePlan &schudlePlan);

/**
 * Serialize from the given istream to this SchudlePlan.
 *
 * \return std::istream
 */
std::istream &operator >> (std::istream &is, GateMap &gateMap);
std::istream &operator >> (std::istream &is, TsnSchudle &tsnSchudle);
std::istream &operator >> (std::istream &is, SchudlePlan &schudlePlan);

ATTRIBUTE_HELPER_HEADER (SchudlePlan);

};// namespace ns3

#endif /* SRC_TRAFFIC_CONTROL_MODEL_TSN_QUEUE_DISC_H_ */
