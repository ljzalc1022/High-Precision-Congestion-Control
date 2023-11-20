#include "./cm-sketch-hp.h"
#include "ns3/core-module.h"
#include "ns3/custom-header.h"
#include "ns3/assert.h"

//#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("CmSketchHP");

CmSketchHP::CmSketchHP (uint32_t d, uint32_t w, uint64_t rate) {
    NS_LOG_FUNCTION (this << d << w);
    depth = d, width = w;
    m_rate = rate;
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

    Simulator::Schedule(m_windowSize, &CmSketchHP::ClearWindow, this);

    m_card = 0;
}

CmSketchHP::CmSketchHP()
{
    counter = nullptr;
    counter_split = nullptr;
    std::cout << "Wrong!" << std::endl;
}

CmSketchHP::~CmSketchHP () {
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
CmSketchHP::UpdateCounter (Ptr<Packet> item) {
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
CmSketchHP::UpdateTxBytes(Ptr<Packet> item)
{
    uint32_t sz = item->GetSize();
    total_bytes += sz;
    split_bytes[pointer] += sz;
}

bool CmSketchHP::GetHeavyHitters (Ptr<Packet> item) {
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
    uint64_t offset = 0;
    uint64_t duration = (granularity - 1) * m_windowSize.GetNanoSeconds() + (Simulator::Now().GetNanoSeconds() - m_lastWindowStartTime.GetNanoSeconds());
    // uint64_t B = (double)m_rate / 8 * (granularity - 1)* (double(m_windowSize.GetNanoSeconds()) / 1000000000);
    uint64_t B = (double)m_rate / 8 * ((double)duration / 1000000000);
    if ((double)B / alpha < ((m_u - m_eta) * B) / ((alpha - 1) * (1 - m_eta * B / total_bytes)))
        offset = 0;
    else
        offset = (double)B / alpha - ((m_u - m_eta) * B) / ((alpha - 1) * (1 - m_eta * B / total_bytes));
    // offset = m_offset;

    // std::cout << " result = " << result 
    //           << " total_bytes = " << total_bytes 
    //           << " alpha = " << alpha 
    //           << " B = " << B 
    //           << " offset = " << offset << std::endl;

    if (result > B / alpha - offset || B / alpha < offset) {
        NS_LOG_DEBUG("true "+std::to_string(result)+" "+std::to_string(total_bytes));
        return true;
    }
    else {
        NS_LOG_DEBUG("false "+std::to_string(result)+" "+std::to_string(total_bytes));
        return false;        
    }
}

void CmSketchHP::ClearWindow()
{
    // if (Simulator::Now().GetSeconds() > 2.0100 && Simulator::Now().GetSeconds() < 2.0102) {
    //     std::cout << Simulator::Now().GetSeconds() << " " << GetCard() << " " << total_bytes << std::endl;
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
            if(!i) {
                UpdateDistribution(old_v, new_v);
                // 如果 counter[i][j]==0 且 j 这个键值之前在 hash_to_bytes 里面
                // 那么就把 j 从 hash_to_bytes 里面删掉
                if (new_v == 0 && old_v != 0) {
                    hash_to_bytes.erase(j);
                } 
                // 否则的话，更新 hash_to_bytes[j] = new_v
                else if (new_v != 0) {
                    hash_to_bytes[j] = new_v;
                }
            }
            counter[i][j] = new_v;
        }
    }
    
    // 把 hash_to_bytes 按照 bytes 值排序，计算
    std::vector<std::pair<uint32_t, uint64_t> > vec_hb(hash_to_bytes.begin(), hash_to_bytes.end());
    
    // 再新建一个表 isHitter, isHitter[hash] 代表这个 hash 是否是 H 大流
    // 之后 getHeavyHitters 只要直接返回 isHitter[hash] 就可以了
    m_lastWindowStartTime = Simulator::Now();
    Simulator::Schedule(m_windowSize, &CmSketchHP::ClearWindow, this);
}

uint32_t CmSketchHP::GetCard () {
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

}