#include "ns3/cm-sketch.h"
#include "ns3/core-module.h"
#include "ns3/custom-header.h"
#include "ns3/assert.h"
#include "ns3/hash-fnv.h"

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
        .AddAttribute("FP",
            "The possibility of false positive",
            DoubleValue(0),
            MakeDoubleAccessor(&CmSketch::m_FP),
            MakeDoubleChecker<double>(0, 1))
        .AddAttribute("FN",
            "The possibility of false negative",
            DoubleValue(0),
            MakeDoubleAccessor(&CmSketch::m_FN),
            MakeDoubleChecker<double>(0, 1)) 
        .AddAttribute("MicroburstMode",
            "Different ways to detect and handle microburst",
            EnumValue(MB_NONE),
            MakeEnumAccessor(&CmSketch::m_microburstMode),
            MakeEnumChecker(MB_NONE, "do nothing about microburst",
                            MB_NAIVE_HALF, "simple detection and handle strategy with granularity 0.5 rtt",
                            MB_NAIVE_ONE,  "simple detection and handle strategy with granularity 1.0 rtt"))
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

    m_uv = CreateObject<UniformRandomVariable> ();

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

    if (m_microburstMode != MB_NONE) {
        if (!m_ableToGet) {
            return false;
        }
    }

    uint64_t result = 0xffffffff;
    for (uint32_t i = 0; i < m_depth; ++i) {
        uint32_t pos = Hash(item, i) % m_width;
        if (result > counter[i][pos]) {
            result = counter[i][pos];
        }
    }

    // uint32_t hash_0 = Hash(item, 0) % m_width;

    uint32_t alpha = GetCard();
    if(alpha == 0)
        return RandomFault(true, hash_0);

    uint64_t hh_thresh;
    switch (m_heavyhitterMode) {

        case HHM_NAIVE:
            // corner case: threshold < 0
            if (total_txbytes / alpha < m_offset) return RandomFault(true, hash_0);

            hh_thresh = total_txbytes / alpha - m_offset;
            return RandomFault(result >= hh_thresh, hash_0);
        
        case HHM_DYNAMIC:
            // Threshold = u * C / N
            hh_thresh = m_u * total_txbytes / alpha;

            // if (Simulator::Now().GetSeconds() > 2.01 && Simulator::Now().GetSeconds() < 2.02) {
            //     std::cout << Simulator::Now().GetSeconds() << " " << GetCard() << " " << total_txbytes
            //               << " thresh:" << hh_thresh << " result:" << result << std::endl;
            // }
            return RandomFault(result >= hh_thresh, hash_0);
        
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
            if (B / alpha < dy_offset) return RandomFault(true, hash_0);

            hh_thresh = B / alpha - dy_offset;
            return RandomFault(result >= hh_thresh, hash_0);
    }

}

void CmSketch::ResetGet()
{
    m_ableToGet = true;
}

void CmSketch::ClearWindow()
{
    // if (Simulator::Now().GetSeconds() > 2.0100 && Simulator::Now().GetSeconds() < 2.0102) {
    //     std::cout << Simulator::Now().GetSeconds() << " " << GetCard() << " " << total_txbytes << std::endl;
    // }
    bool flowEnded = false;
    
    switch (m_microburstMode)
    {
    case MB_NONE:
        break;
    
    case MB_NAIVE_HALF:
        for (uint32_t i = 0; i < m_width; ++i) {
            uint64_t currentRTT = counter_split[pointer][0][i];
            uint64_t lastRTT = counter_split[(pointer + m_granularity - 1) % m_granularity][0][i]; 

            // if (lastRTT != 0 || currentRTT != 0) {
            //     if (Simulator::Now().GetSeconds() > 2.0045 && Simulator::Now().GetSeconds() < 2.0060) {
            //         std::cout << Simulator::Now().GetSeconds() << " " << i << " " << "lastRTT: " << lastRTT 
            //                   << " currentRTT" << currentRTT << std::endl;
            //     }                
            // }

            if (lastRTT >= 500 && currentRTT <= 200) {
                flowEnded = true;
            }
        }

        if (flowEnded) {
            m_ableToGet = false;
            Simulator::Schedule(m_windowSize + m_windowSize , &CmSketch::ResetGet, this);
        }

        break;

    case MB_NAIVE_ONE:
        // m_updateMB is used to make sure that microburst detection is activated every 2 window (= 1 rtt)
        if (!m_updateMB) {
            m_updateMB = true;
        }
        else {

            for (uint32_t i = 0; i < m_width; ++i) {
                uint64_t currentRTT = counter_split[pointer][0][i] 
                                    + counter_split[(pointer + m_granularity - 1) % m_granularity][0][i];
                uint64_t lastRTT = counter_split[(pointer + m_granularity - 2) % m_granularity][0][i] 
                                + counter_split[(pointer + m_granularity - 3) % m_granularity][0][i];

                // if (lastRTT != 0 || currentRTT != 0) {
                //     if (Simulator::Now().GetSeconds() > 2.0045 && Simulator::Now().GetSeconds() < 2.0060) {
                //         std::cout << Simulator::Now().GetSeconds() << " " << i << " " << "lastRTT: " << lastRTT 
                //                   << " currentRTT" << currentRTT << std::endl;
                //     }                
                // }

                if (lastRTT >= 500 && currentRTT <= 200) {
                    flowEnded = true;
                }
            }

            m_updateMB = false;
        }

        if (flowEnded) {
            m_ableToGet = false;
            Simulator::Schedule(m_windowSize + m_windowSize , &CmSketch::ResetGet, this);
        }

        break;

    default:
        break;
    }

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
            // // erase the hashValue->randomVariable mapping entry if a flow ends
            // if(!i && (old_v != 0 && new_v == 0)) {
            //     m_hash2rv.erase(j);
            // }
            counter[i][j] = new_v;
        }
    }
    m_lastWindowStartTime = Simulator::Now();
    Simulator::Schedule(m_windowSize, &CmSketch::ClearWindow, this);
}

uint32_t CmSketch::GetCard () {
    double rate = (m_width - m_card) / double(m_width);
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
    // uint32_t hash = 0;

    // hash += 13 * dip + 9 * sip + (dip >> 3) + (sip >> 2);
    // hash ^= sport;
    // hash ^= dport;
    // hash ^= pg;
    // hash ^= permutation;

    // hash += (hash << 3);
    // hash ^= (hash >> 11);
    // hash += (hash << 15);

    // Use fnv1a hash function (defined in src/core/model/hash-fnv.h)
    Hash::Function::Fnv1a fnvHasher;

    char buf[20];
    size_t size = 18;
    // 18 = sizeof(sip) + sizeof(dip) + sizeof(sport) + sizeof(dport) + sizeof(pg) + sizeof(permutation)
    
    uint32_t *p_sip = (uint32_t *)buf;
    (*p_sip) = sip;
    uint32_t *p_dip = (uint32_t *)(buf + 4);
    (*p_dip) = dip;
    uint16_t *p_sport = (uint16_t *)(buf + 8);
    (*p_sport) = sport;    
    uint16_t *p_dport = (uint16_t *)(buf + 10);
    (*p_dport) = dport;
    uint16_t *p_pg = (uint16_t *)(buf + 12);
    (*p_pg) = pg;
    uint32_t *p_permutation = (uint32_t *)(buf + 14);
    (*p_permutation) = permutation;

    uint32_t hash = fnvHasher.GetHash32(buf, size);
    fnvHasher.clear();

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

bool 
CmSketch::RandomFault(bool re, uint32_t hash) 
{
    // printf("In rf\n");
    double u;
    if (re == true) {
        // printf("In rf: re = true\n");
        if (m_FN > 0) {
            u = m_uv->GetValue();
            if (u <= m_FN) {
                re = false;
            }
        }
    } else {
        // printf("FP: %lf\n", m_FP);
        if (m_FP > 0) {
            // printf("In m_FP > 0\n");
            u = m_uv->GetValue();
            // printf("u: %lf, FP: %lf\n", u, m_FP);
            if (u <= m_FP) {
                re = true;
            }
        }
    }
    return re;
    // double rv;

    // if (re == true && m_FN == 0) {
    //     return true;
    // }
    // if (re == false && m_FP == 0) {
    //     return false;
    // }

    // // record a map: hashValue->randomVariable(in [0, 1]) 
    // // to simulate per-flow FP/FN
    // auto search = m_hash2rv.find(hash);
    // if (search != m_hash2rv.end()) {
    //     rv = search->second;
    //     // printf("hash: %u, rv: %lf\n", hash, rv);
    // } else {
    //     rv = m_uv->GetValue();
    //     m_hash2rv.insert(std::make_pair(hash, rv));
    //     // printf("hash: %u, rv: %lf, new inserted\n", hash, rv);
    // }

    // // Now the code use a same rv for each flow to determine whether FP&FN 
    // // should happen or not. It is feasible because now we only manually set
    // // either FN or FP in current experimentes. But in fact, if in the future 
    // // we need to simulate FN and FP at the same time in a single experiment, 
    // // we should **maintain one rv for FN and another for FP** per flow.
    // if (re == true && rv <= m_FN) {
    //     return false;
    // }
    // else if (re == false && rv <= m_FP) {
    //     return true;
    // }
    // else {
    //     return re;
    // }
}

}