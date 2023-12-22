#ifndef _CM_SKETCH_H
#define _CM_SKETCH_H

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include <map>

namespace ns3 {

class CmSketch : public Object{
public:
    // Rolling Window
    uint32_t m_granularity;
    Time     m_windowSize;      // window size (set to 0.5 base RTT by default)
    uint32_t m_depth, m_width;

    uint32_t m_card;    // cardinality
    uint64_t m_rate;    // data rate of the Net Device on which the sketch structure resides
                        // (only used in HHM_DYNAMIC_HPCC)

    enum heavyhitterMode_t {
        HHM_NAIVE,              // hh_thresh = total / card - fixed_offset
        HHM_DYNAMIC,            // hh_thresh = m_u * total / (card - 1)
        HHM_DYNAMIC_HPCC        // hh_thresh = total / card - dynamic_offset
    };
    uint32_t m_heavyhitterMode;

    // Fixed offset
    uint64_t m_offset;

    // Dynamic offset params for universal CC algos (hjt & ljz's idea)
    int32_t  m_qdiff;    // max(0, q-q_t)
    int32_t  m_qt;       // target queue length
    double   m_u;  

    // Dynamic offset for HPCC only (snow's idea)
    double   m_miu;
    double   m_eta;

    // For experiments about sketch percision
    double   m_FP;  // possibility of false positive
    double   m_FN;  // possibility of false negative
    Ptr<UniformRandomVariable> m_uv;  // random variable generator

public:
    static TypeId GetTypeId(void);
    CmSketch ();
    ~CmSketch ();
    void Init();
    void UpdateCounter (Ptr<Packet> item);
    void UpdateTxBytes (Ptr<Packet> item);
    bool GetHeavyHitters (Ptr<Packet> item);
    void ClearWindow ();
    uint32_t GetCard ();

    void setRate(uint64_t rate);
    void setQdiff(int32_t q);

private:
    // Measured when a packet is enqueued
    uint64_t *** counter_split;     
    uint64_t ** counter;

    // Measured when a packet is dequeued(transmitted)
    uint64_t total_txbytes;           // # bytes transmitted
    uint64_t * split_txbytes;         // # bytes transmitted in each window

    uint32_t pointer;   // index of the currently using subwindow

    Time m_lastWindowStartTime;

    void UpdateCardinality(uint32_t old_v, uint32_t new_v);
    
    uint32_t Hash(Ptr<Packet> item, uint32_t permutation) const;
    uint32_t HashFunction(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, uint32_t permutation) const;

    bool RandomFault(bool re);
};

}

#endif