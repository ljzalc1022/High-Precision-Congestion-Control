#ifndef _CM_SKETCH_H
#define _CM_SKETCH_H

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include <map>

namespace ns3 {

class CmSketch {
public:
    uint64_t *** counter_split;
    uint64_t ** counter;
    uint32_t width, depth;
    uint64_t total_bytes;
    uint64_t * split_bytes;
    uint32_t m_card; // cardinality
    uint32_t m_offset{100};
    // std::map<uint32_t, uint32_t> distribution;

public:
    CmSketch (uint32_t d, uint32_t w);
    CmSketch ();
    ~CmSketch ();

    /**
     * \brief update the counter for each flow
     */
    void UpdateCounter (Ptr<Packet> item);
    /**
     * \brief update the txBytes
    */
    void UpdateTxBytes (Ptr<Packet> item);

    bool GetHeavyHitters (Ptr<Packet> item);
    void ClearWindow ();
    uint32_t GetCard ();

private:
    const uint32_t granularity = 5;
    Time m_windowSize{NanoSeconds(120960 / 2)}; // window size = 0.5 base RTT
    
    uint32_t pointer;

    /**
     * \brief update the flow distribution
     * \param old_v the old size of the flow
     * \param new_v the new size of the flow
    */
    void UpdateDistribution(uint32_t old_v, uint32_t new_v);
    
    uint32_t Hash(Ptr<Packet> item, uint32_t permutation) const;
    uint32_t HashFunction(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, uint32_t permutation) const;
};

}

#endif