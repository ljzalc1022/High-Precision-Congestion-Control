/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
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
*/

#ifndef BROADCOM_EGRESS_H
#define BROADCOM_EGRESS_H

#include <queue>
#include "ns3/packet.h"
#include "queue.h"
#include "drop-tail-queue.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/event-id.h"
#include "ns3/bigflow-tag.h"
#include "ns3/my-sketch.h"

namespace ns3 {

	class TraceContainer;

	class BEgressQueue : public Queue {
	public:
		static TypeId GetTypeId(void);
		static const unsigned fCnt = 128; //max number of queues, 128 for NICs
		static const unsigned qCnt = 8; //max number of queues, 8 for switches
		BEgressQueue();
		virtual ~BEgressQueue();
		bool Enqueue(Ptr<Packet> p, uint32_t qIndex);
		Ptr<Packet> DequeueRR(bool paused[]);
		Ptr<Packet> DequeuePrio(bool paused[]);
		uint32_t GetNBytes(uint32_t qIndex) const;
		uint32_t GetNBytesTotal() const;
		uint32_t GetLastQueue();
		void SetRate(uint64_t rate);

		TracedCallback<Ptr<const Packet>, uint32_t> m_traceBeqEnqueue;
		TracedCallback<Ptr<const Packet>, uint32_t> m_traceBeqDequeue;

		// double GetSmallFlowPortion();

	private:
		bool DoEnqueue(Ptr<Packet> p, uint32_t qIndex);
		Ptr<Packet> DoDequeueRR(bool paused[]);
		Ptr<Packet> DoDequeuePrio(bool paused[]);
		uint32_t m_RRpointer{0};
		uint32_t m_RRseq[5] = {1, 1, 1, 1, 3}; // expD 80% -- 20%
		uint32_t m_RRseqLen{5};
		// uint32_t m_RRseq[10] = {1, 1, 1, 3, 1, 1, 3, 1, 1, 3}; // expE 70% -- 30%
		// uint32_t m_RRseqLen{10};		
		//for compatibility
		virtual bool DoEnqueue(Ptr<Packet> p);
		virtual Ptr<Packet> DoDequeue(void);
		virtual Ptr<const Packet> DoPeek(void) const;
		double m_maxBytes; //total bytes limit
		uint32_t m_bytesInQueue[fCnt];
		uint32_t m_bytesInQueueTotal;
		uint32_t m_rrlast;
		uint32_t m_qlast;
		std::vector<Ptr<Queue> > m_queues; // uc queues

		bool m_enableSketch;
		Ptr<MySketch> m_sketch;
		bool CheckCongestion() const;
		bool IsDataPacket(Ptr<Packet> p) const;
		uint64_t m_rate;
	};

} // namespace ns3

#endif /* DROPTAIL_H */
