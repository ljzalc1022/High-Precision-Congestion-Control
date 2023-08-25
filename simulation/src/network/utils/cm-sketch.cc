#include "ns3/cm-sketch.h"
#include "ns3/core-module.h"
#include "ns3/custom-header.h"
#include "ns3/assert.h"

//#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("CmSketch");

CmSketch::CmSketch (uint32_t d, uint32_t w) {
    NS_LOG_FUNCTION (this << d << w);
    depth = d, width = w;
    counter_split = new uint64_t**[granularity];

    for (uint32_t i = 0; i < granularity; ++i) {
        counter_split[i] = new uint64_t* [depth];
        for (uint32_t j = 0; j < depth; ++j) {
            counter_split[i][j] = new uint64_t[width];
            for (uint32_t k = 0; k < width; ++k) {
                counter_split[i][j][k] = 0;
            }
        }
    }

    // init packet count
    total_bytes = 0;
    split_bytes = new uint64_t[granularity];
    for (uint32_t i = 0; i < granularity; i++)
    {
        split_bytes[i] = 0;
    }

    counter = new uint64_t *[depth];
    for (uint32_t i = 0; i < depth; ++i) {
        counter[i] = new uint64_t[width];
        for (uint32_t j = 0; j < width; ++j) {
            counter[i][j] = 0;
        }
    }
    pointer = 0;

    Simulator::Schedule(m_windowSize, &CmSketch::ClearWindow, this);

    m_card = 0;
}

CmSketch::CmSketch()
{
    counter = nullptr;
    counter_split = nullptr;
    std::cout << "Wrong!" << std::endl;
}

CmSketch::~CmSketch () {
    // NS_LOG_FUNCTION (this);xi
    for(uint32_t i = 0; i < granularity; ++i) {
        for (uint32_t j = 0; j < depth; ++j) {
            delete counter_split[i][j];
        }
        delete counter_split[i];
    }
    delete counter_split;

    for (uint32_t i = 0; i < depth; ++i) {
        delete counter[i];
    }
    delete counter;
}

void 
CmSketch::UpdateCounter (Ptr<Packet> item) {
    // NS_LOG_FUNCTION (this << item);
    uint32_t sz = item->GetSize();
    for (uint32_t i = 0; i < depth; ++i) {
        // compute hash
        uint32_t pos = Hash(item, i) % width;

        // update the split counter
        counter_split[pointer][i][pos] += sz;

        // update flow distribution and the total counter
        uint32_t old_v = counter[i][pos];
        uint32_t new_v = old_v + sz;
        if (!i) UpdateDistribution(old_v, new_v);
        counter[i][pos] = new_v;
    }
}

void
CmSketch::UpdateTxBytes(Ptr<Packet> item)
{
    uint32_t sz = item->GetSize();
    total_bytes += sz;
    split_bytes[pointer] += sz;
}

bool CmSketch::GetHeavyHitters (Ptr<Packet> item) {
    NS_LOG_FUNCTION (this << item);
    uint32_t result = 0xffffffff;
    for (uint32_t i = 0; i < depth; ++i) {
        uint32_t pos = Hash(item, i) % width;
        if (result > counter[i][pos]) {
            result = counter[i][pos];
        }
    }

    uint32_t alpha = GetCard();
    if(alpha == 0)
    {
        return true;
    }
    // std::cout << "result = " << result << std::endl 
    //           << "total_bytes = " << total_bytes << std::endl 
    //           << "alpha = " << alpha << std::endl;
    if (result > total_bytes / alpha - m_offset) {
        NS_LOG_DEBUG("true "+std::to_string(result)+" "+std::to_string(total_bytes));
        return true;
    }
    else {
        NS_LOG_DEBUG("false "+std::to_string(result)+" "+std::to_string(total_bytes));
        return false;        
    }
}

void CmSketch::ClearWindow()
{
    // if (Simulator::Now().GetSeconds() > 2.01) {
    //     std::cout << GetCard() << " " << total_bytes << std::endl;
    // }
    // update the pointer
    pointer = (pointer + 1) % granularity;

    // update the number of packets
    total_bytes -= split_bytes[pointer];
    split_bytes[pointer] = 0;

    // update the counter
    for (uint32_t i = 0; i < depth; ++i) {
        for (uint32_t j = 0; j < width; ++j) {
            // reset the out-dated counter 
            uint32_t delta = counter_split[pointer][i][j];
            counter_split[pointer][i][j] = 0;

            // update the total counter
            uint32_t old_v = counter[i][j];
            uint32_t new_v = counter[i][j] - delta;
            if(!i) UpdateDistribution(old_v, new_v);
            counter[i][j] = new_v;
        }
    }

    Simulator::Schedule(m_windowSize, &CmSketch::ClearWindow, this);
}

uint32_t CmSketch::GetCard () {
    uint32_t cardinality = m_card;
    double counter_num = width;

    double rate = (counter_num - cardinality) / counter_num;
    return counter_num * log(1 / rate);
}

void
CmSketch::UpdateDistribution(uint32_t old_v, uint32_t new_v)
{
    if (old_v)
    {
        m_card--;
    }
    if (new_v)
    {
        m_card++;
    }
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

    hash ^= sip;
    hash += dip;
    hash ^= sport;
    hash ^= dport;
    hash ^= pg;
    hash ^= permutation;

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

}