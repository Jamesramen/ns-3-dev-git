/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) YEAR COPYRIGHTHOLDER
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
 * Author: Luca Wendling <lwendlin@rhrk.uni-kl.de>
 */

#ifndef SRC_TRAFFIC_CONTROL_HELPER_TIME_SENSETIVE_NETWORK_HELPER_H_
#define SRC_TRAFFIC_CONTROL_HELPER_TIME_SENSETIVE_NETWORK_HELPER_H_

#include "ns3/traffic-control-helper.h"

namespace ns3 {

class TimeSensitiveNetworkHelper: public TrafficControlHelper
{
public:
  TimeSensitiveNetworkHelper();
  virtual ~TimeSensitiveNetworkHelper();
};

}
#endif /* SRC_TRAFFIC_CONTROL_HELPER_TIME_SENSETIVE_NETWORK_HELPER_H_ */
