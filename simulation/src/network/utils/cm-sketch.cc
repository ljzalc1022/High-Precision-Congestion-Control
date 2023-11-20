#include "ns3/cm-sketch.h"
#include "ns3/core-module.h"
#include "ns3/custom-header.h"
#include "ns3/assert.h"

//#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("CmSketch");

NS_OBJECT_ENSURE_REGISTERED(CmSketch);

TypeId CmSketch::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::CmSketch")
        .SetParent<Object>()
        .AddConstructor<CmSketch>()
        .AddAttribute("Depth",
            "The depth of sketch structure",
            UintegerValue(2),
            MakeUintegerAccessor(&CmSketch::m_depth),
            MakeUintegerChecker<uint32_t>())
        .AddAttribute("Width",
            "The width of sketch structure",
            UintegerValue(65537),
            MakeUintegerAccessor(&CmSketch::m_width),
            MakeUintegerChecker<uint32_t>())
        .AddAttribute("Offset",
            "The offset in heavy hitter judgement",
            UintegerValue(100),
            MakeUintegerAccessor(&CmSketch::m_offset),
            MakeUintegerChecker<uint64_t>())
        .AddAttribute("Granularity",
            "The number of subwindows",
            UintegerValue(5),
            MakeUintegerAccessor(&CmSketch::m_granularity),
            MakeUintegerChecker<uint32_t>())
        .AddAttribute("WindowSize",
            "The size of a subwindow",
            TimeValue(NanoSeconds(30120)),
            MakeTimeAccessor(&CmSketch::m_windowSize),
            MakeTimeChecker())
        .AddAttribute("HeavyhitterMode",
            "Different ways to set heavy hitter threshold",
            EnumValue(HHM_NAIVE),
            MakeEnumAccessor(&CmSketch::m_heavyhitterMode),
            MakeEnumChecker(HHM_NAIVE, "average threshold with fixed offset",
                             HHM_DYNAMIC, "dynamic offset for common CC algos",
                             HHM_DYNAMIC_HPCC, "dynamic offset specially designed for HPCC"))
        .AddAttribute("u",
            "The max proportion of uncontrolled flows in HHM_DYNAMIC",
            DoubleValue(0.45),
            MakeDoubleAccessor(&CmSketch::m_u),
            MakeDoubleChecker<double>(0, 1))    
        ;
    return tid;
}

CmSketch::CmSketch () :
    Object()
{
    NS_LOG_FUNCTION (this);
}

CmSketch::~CmSketch () {
    NS_LOG_FUNCTION (this);

    for(uint32_t i = 0; i < m_granularity; ++i) {
        for (uint32_t j = 0; j < m_depth; ++j) {
            delete counter_split[i][j];
        }
        delete counter_split[i];
    }
    delete counter_split;

    for (uint32_t i = 0; i < m_depth; ++i) {
        delete counter[i];
    }
    delete counter;
}

void
CmSketch::Init() {
    // Create counters for each subwindow, then zero them
    counter_split = new uint64_t**[m_granularity];
    for (uint32_t i = 0; i < m_granularity; ++i) {
        counter_split[i] = new uint64_t* [m_depth];
        for (uint32_t j = 0; j < m_depth; ++j) {
            counter_split[i][j] = new uint64_t[m_width];
            for (uint32_t k = 0; k < m_width; ++k) {
                counter_split[i][j][k] = 0;
            }
        }
    }

    // Init byte counters (set to 0)
    total_txbytes = 0;
    split_txbytes = new uint64_t[m_granularity];
    for (uint32_t i = 0; i < m_granularity; i++)
    {
        split_txbytes[i] = 0;
    }

    // Create a total counter (which summarizes all subwindows)
    counter = new uint64_t *[m_depth];
    for (uint32_t i = 0; i < m_depth; ++i) {
        counter[i] = new uint64_t[m_width];
        for (uint32_t j = 0; j < m_width; ++j) {
            counter[i][j] = 0;
        }
    }

    // Use subwindow 0 at the beginning
    pointer = 0;

    Simulator::Schedule(m_windowSize, &CmSketch::ClearWindow, this);
    m_card = 0;
}

void 
CmSketch::UpdateCounter (Ptr<Packet> item) {
    NS_LOG_FUNCTION (this << item);
    
    uint32_t sz = item->GetSize();
    for (uint32_t i = 0; i < m_depth; ++i) {
        // compute hash
        uint32_t pos = Hash(item, i) % m_width;

        // update the split counter
        counter_split[pointer][i][pos] += sz;

        // update cardinality and the total counter
        uint64_t old_v = counter[i][pos];
        uint64_t new_v = old_v + sz;
        if (!i) UpdateCardinality(old_v, new_v);
        counter[i][pos] = new_v;
    }
}

void
CmSketch::UpdateTxBytes(Ptr<Packet> item)
{
    NS_LOG_FUNCTION (this << item);

    uint32_t sz = item->GetSize();
    total_txbytes += sz;
    split_txbytes[pointer] += sz;
}

bool 
CmSketch::GetHeavyHitters (Ptr<Packet> item) {
    NS_LOG_FUNCTION (this << item);

    uint64_t result = 0xffffffff;
    for (uint32_t i = 0; i < m_depth; ++i) {
        uint32_t pos = Hash(item, i) % m_width;
        if (result > counter[i][pos]) {
            result = counter[i][pos];
        }
    }

    uint32_t alpha = GetCard();
    if(alpha == 0)
        return true;

    uint64_t hh_thresh;
    switch (m_heavyhitterMode) {

        case HHM_NAIVE:
            // corner case: threshold < 0
            if (total_txbytes / alpha < m_offset) return true;

            hh_thresh = total_txbytes / alpha - m_offset;
            return (result >= hh_thresh);
        
        case HHM_DYNAMIC:
            // Threshold = u * C / N
            hh_thresh = m_u * total_txbytes / alpha;

            // if (Simulator::Now().GetSeconds() > 2.045 && Simulator::Now().GetSeconds() < 2.060) {
            //     std::cout << Simulator::Now().GetSeconds() << " " << GetCard() << " " << total_txbytes
            //               << " thresh:" << hh_thresh << std::endl;
            // }
            return (result >= hh_thresh);
        
        case HHM_DYNAMIC_HPCC:
            // Threshold = B / N - ((miu - eta) * B) / ((N - 1) * (1 - eta * B / R(t)))
            uint64_t duration = (m_granularity - 1) * m_windowSize.GetNanoSeconds() 
                                + (Simulator::Now().GetNanoSeconds() - m_lastWindowStartTime.GetNanoSeconds());
            uint64_t B = (double)m_rate / 8 * ((double)duration / 1e9);
            
            uint64_t dy_offset;
            if ((double)B / alpha < ((m_miu - m_eta) * B) / ((alpha - 1) * (1 - m_eta * B / total_txbytes)))
                dy_offset = 0;
            else
                dy_offset = (double)B / alpha - ((m_miu - m_eta) * B) / ((alpha - 1) * (1 - m_eta * B / total_txbytes));

            // corner case: threshold < 0
            if (B / alpha < dy_offset) return true;

            hh_thresh = B / alpha - dy_offset;
            return (result >= hh_thresh);
    }

}

void CmSketch::ClearWindow()
{
    // if (Simulator::Now().GetSeconds() > 2.0100 && Simulator::Now().GetSeconds() < 2.0102) {
    //     std::cout << Simulator::Now().GetSeconds() << " " << GetCard() << " " << total_txbytes << std::endl;
    // }

    // update the pointer
    pointer = (pointer + 1) % m_granularity;

    // update the number of packets
    total_txbytes -= split_txbytes[pointer];
    split_txbytes[pointer] = 0;

    // update the counter
    for (uint32_t i = 0; i < m_depth; ++i) {
        for (uint32_t j = 0; j < m_width; ++j) {
            // reset the out-dated counter 
            uint64_t delta = counter_split[pointer][i][j];
            counter_split[pointer][i][j] = 0;

            // update the total counter
            uint32_t old_v = counter[i][j];
            uint32_t new_v = counter[i][j] - delta;
            if(!i) UpdateCardinality(old_v, new_v);
            counter[i][j] = new_v;
        }
    }
    m_lastWindowStartTime = Simulator::Now();
    Simulator::Schedule(m_windowSize, &CmSketch::ClearWindow, this);
}

uint32_t CmSketch::GetCard () {
    double rate = (m_width - m_card) / m_width;
    return m_width * log(1 / rate);
}

void
CmSketch::UpdateCardinality(uint32_t old_v, uint32_t new_v)
{
    if (old_v) 
        m_card--;

    if (new_v)
        m_card++;
}

uint32_t
CmSketch::Hash(Ptr<Packet> p, uint32_t permutation) const
{
    CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
    ch.getInt = 0;
    p->PeekHeader(ch);
    NS_ASSERT(ch.l3Prot == 0x11);

    uint32_t hash = HashFunction(ch.sip, ch.dip, ch.udp.sport, ch.udp.dport, ch.udp.pg, permutation);

    return hash;
}

uint32_t
CmSketch::HashFunction(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, uint32_t permutation) const
{
    // this hash function is generated by chatgpt 3.5
    uint32_t hash = 0;

    hash += 13 * dip + 9 * sip + (dip >> 3) + (sip >> 2);
    hash ^= sport;
    hash ^= dport;
    hash ^= pg;
    hash ^= permutation;

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

void
CmSketch::setRate(uint64_t rate)
{
    m_rate = rate;
}

void
CmSketch::setQdiff(int32_t q)
{
    m_qdiff = (q > m_qt) ? (q - m_qt) : 0;
}

}