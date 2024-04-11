#ifndef _MY_SKETCH_H
#define _MY_SKETCH_H

#include <algorithm>
#include <cassert>

#include "ns3/custom-header.h"
#include "ns3/int-tag.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

namespace ns3 {

class MySketch : public Object {
public:
    static TypeId GetTypeId();
    MySketch();
    ~MySketch() override;
    void Init();
    void Update(Ptr<Packet> packet, IntTag &tag);

private:
    // Rolling Window
    int m_curWindow;
    int m_cntWindows; // count the number of windows
    int m_nWindows;
    Time m_windowSize;
    Time m_startTime; // the time when the oldest window starts
    void NextWindow();

    int m_nRows;
    int m_nColumns;

    struct data {
        uint64_t n;
        int e;
        double t;
        data() {n = e = 0;}
        void update(int w, int q, int m)
        {
            if (q - e >= m) n = 0;
            if (w) {
                if (n == 0) t = Simulator::Now().GetSeconds();
                n += w, e = q;
            }
        }
    };
    using buckets = std::vector<data>;
    using sketch = std::vector<buckets>;
    std::vector<sketch> m_counters;

    struct FiveTuple {
        uint32_t sip, dip;
        uint16_t sport, dport;
        uint16_t pg;
        FiveTuple(Ptr<Packet> packet) {
            CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
            ch.getInt = 0;
            packet->PeekHeader(ch);
            NS_ASSERT(ch.l3Prot == 0x11);

            sip = ch.sip, dip = ch.dip;
            sport = ch.udp.sport, dport = ch.udp.dport;
            pg = ch.udp.pg;
        }
        bool operator < (const FiveTuple& other) const
        {
            if (sip != other.sip) return sip < other.sip;
            if (dip != other.dip) return dip < other.dip;
            if (sport != other.sport) return sport < other.sport;
            if (dport != other.dport) return dport < other.dport;
            return pg < other.pg;
        }
    };
    std::set<FiveTuple> m_flowKeys;

    uint64_t Query(const FiveTuple &flowkey);

    uint32_t Hash(const FiveTuple &item, uint32_t permutation) const;
    uint32_t HashFunction(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, 
                          uint16_t pg, uint32_t permutation) const;

private:
    uint64_t m_rate;
    uint64_t m_lastTh;
public:
    void setRate(uint64_t rate)
    {
        m_rate = rate;
        m_lastTh = m_rate;
    }
    uint32_t GetCard()
    {
        return - m_nColumns * log(double(m_nColumns - m_card) / m_nColumns);
    }
    uint64_t GetThAvg()
    {
        int card = GetCard();
        if (card == 0) return 0;
        return m_rate / card;
    }
    uint64_t GetThBinary(FiveTuple flowkey, IntTag &tag)
    {
        std::vector<uint64_t> flowRates, dist;

        m_flowKeys.insert(flowkey);
        for (auto it = m_flowKeys.begin(); it != m_flowKeys.end();)
        {
            uint64_t rate = Query(*it);
            if (rate == 0)
            {
                it = m_flowKeys.erase(it); // erase returns the iterator to the next element
                continue;
            }
            flowRates.push_back(rate);
            ++it; // Move to the next element
        }
        flowRates.push_back(0);
        std::sort(flowRates.begin(), flowRates.end());

        dist.push_back(0);
        for (unsigned int i = 1; i < flowRates.size(); i++)
        {
            dist.push_back(dist.back() + flowRates[i]);
        }
        if (dist.back() < m_rate) 
        { 
			int l = 1, r = dist.size() - 1, re = -1;
			while (l <= r)
			{
				int mid = (l + r) >> 1;
				if (flowRates[mid] >= flowRates.back() * m_h) r = mid - 1, re = mid; 
				else l = mid + 1;
			}
			tag.m_Rb = ((double)m_rate - dist[re - 1]) / (dist.size() - re);
	//		if (GetCard() == 2 && m_rate == 23750000000)
	//		{
	//			printf("rate: %lfGbps; target: %lfGbps; Th: %lfGbps\n", m_rate / 1e9, tag.m_Rb / 1e9, flowRates[re] / 1e9);
	//			for (auto r: flowRates) printf("%lfGbps ,", r / 1e9);
	//			printf("\n");
	//		}
	//		if (GetCard() != flowRates.size() - 1)
	//		{
	//			printf("%u %lu\n", GetCard(), flowRates.size());
	//			fflush(stdout);
	//		}
	//		assert(GetCard() == flowRates.size() - 1);
			return flowRates[re];
        }

        int l = 1, r = dist.size() - 1, re = -1;
        while (l <= r) 
        {
            int mid = (l + r) >> 1;
            if (((double)m_rate - dist[mid - 1]) / (dist.size() - mid) > flowRates[mid]) l = mid + 1;
            else r = mid - 1, re = mid;
        }
		tag.m_Rb = ((double)m_rate - dist[re - 1]) / (dist.size() - re);
		return flowRates[re];
    }

private:
    int m_card;
    std::vector<int> m_B; // B in Magic Sketch
    std::vector<int> m_cntB;

private:
    uint64_t m_totalBytes;
    std::vector<uint64_t> m_bytes;

private:
    double m_h; 
};

}

#endif // !_MY_SKETCH_H
