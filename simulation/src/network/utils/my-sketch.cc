#include "my-sketch.h"

#include "ns3/core-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MySketch");

NS_OBJECT_ENSURE_REGISTERED(MySketch);

TypeId 
MySketch::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::MySketch")
        .SetParent<Object>()
        .AddConstructor<MySketch>()
        .AddAttribute("Rows",
                      "The number of rows",
                      IntegerValue(2),
                      MakeIntegerAccessor(&MySketch::m_nRows),
                      MakeIntegerChecker<int>())
        .AddAttribute("Columns",
                      "The number of columns",
                      IntegerValue(65537),
                      MakeIntegerAccessor(&MySketch::m_nColumns),
                      MakeIntegerChecker<int>())
        .AddAttribute("Windows",
                      "The number of subwindows",
                      IntegerValue(5),
                      MakeIntegerAccessor(&MySketch::m_nWindows),
                      MakeIntegerChecker<int>())
        .AddAttribute("WindowSize",
                      "the size of a subwindow",
                      TimeValue(MicroSeconds(6)),
                      MakeTimeAccessor(&MySketch::m_windowSize),
                      MakeTimeChecker())
        ;
    return tid;
}

MySketch::MySketch()
    : Object()
{
    NS_LOG_FUNCTION(this);
}

MySketch::~MySketch()
{
    NS_LOG_FUNCTION(this);
}

void
MySketch::Init()
{
    buckets row(m_nColumns);
    sketch window(m_nRows, row);
    m_counters.resize(m_nWindows, window);
    m_curWindow = 0;
    m_cntWindows = 0;
    m_startTime = Simulator::Now();
    Simulator::Schedule(m_windowSize, &MySketch::NextWindow, this);
}

void
MySketch::Update(Ptr<Packet> packet, IntTag &tag)
{
    FiveTuple flowkey(packet);
    uint64_t w = packet->GetSize();
    for (int i = 0; i < m_nRows; i++) 
    {
        int key = Hash(flowkey, i) % m_nColumns;
        m_counters[m_curWindow][i][key].update(w, m_curWindow, m_nWindows);
        for (int j = 1; j < m_nWindows; j++)
        {
            int j_ = (m_curWindow + j) % m_nWindows;
            m_counters[j_][i][key].update(0, m_curWindow, m_nWindows);
        }
    }

    uint64_t Rb = 0;
    for (auto f: m_heavyKeys)
    {
        uint64_t R = Query(f);
        if (R < m_Th) m_heavyKeys.erase(f);
        else Rb += R;
    }
    tag.m_isBigFlow = true;
    if (m_heavyKeys.find(flowkey) == m_heavyKeys.end())
    {
        uint64_t R = Query(flowkey);
        if (R >= m_Th) 
        {
            m_heavyKeys.insert(flowkey);
        }
        else tag.m_isBigFlow = false;
    }
    tag.m_Rb = Rb;
}

uint64_t 
MySketch::Query(const FiveTuple &flowkey)
{
    std::vector<uint64_t> re(m_nWindows);
    for (int i = 0; i < m_nRows; i++)
    {
        int key = Hash(flowkey, i);
        for (int j = 0; j < m_nWindows; j++)
        {
            m_counters[j][i][key].update(0, m_cntWindows, m_nWindows);
            if (!i || re[j] > m_counters[j][i][key].n)
            {
                re[j] = m_counters[j][i][key].n;
            }
        }
    }
    uint64_t sz = 0;
    for (auto s: re) sz += s;
    return sz / (Simulator::Now() - m_startTime).GetSeconds();
}

void
MySketch::NextWindow()
{
    m_curWindow = (m_curWindow + 1) % m_nWindows;
    m_cntWindows++;
    m_startTime += m_windowSize;
}

// hash function by snow
// modified by ljz
uint32_t
MySketch::Hash(const FiveTuple &key, uint32_t permutation) const
{
    uint32_t hash = HashFunction(key.sip, key.dip, key.sport, key.dport, key.pg, permutation);

    return hash;
}
uint32_t
MySketch::HashFunction(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, uint32_t permutation) const
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

}