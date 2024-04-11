#include "my-sketch.h"

#include <cmath>

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
        .AddAttribute("H",
                      "the hysteresis factor",
                      DoubleValue(1),
                      MakeDoubleAccessor(&MySketch::m_h),
                      MakeDoubleChecker<double>(0, 1))
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

    m_card = 0;
    m_B.resize(m_nColumns, -m_nWindows);
    m_cntB.resize(m_nWindows, 0);

    m_totalBytes = 0;
    m_bytes.resize(m_nWindows, 0);
}

void
MySketch::Update(Ptr<Packet> packet, IntTag &tag)
{
    FiveTuple flowkey(packet);
    uint64_t w = packet->GetSize();
    m_totalBytes += w;
    m_bytes[m_curWindow] += w;
    for (int i = 0; i < m_nRows; i++) 
    {
        int key = Hash(flowkey, i) % m_nColumns;
        m_counters[m_curWindow][i][key].update(w, m_cntWindows, m_nWindows);
        for (int j = 1; j < m_nWindows; j++)
        {
            int j_ = (m_curWindow + j) % m_nWindows;
            m_counters[j_][i][key].update(0, m_cntWindows, m_nWindows);
        }

        // update B
        if (i == 0)
        {
            if (m_cntWindows - m_B[key] < m_nWindows)
            {
                m_cntB[m_B[key] % m_nWindows]--;
                m_card--;
            }
            m_B[key] = m_cntWindows;
            m_cntB[m_curWindow]++;
            m_card++;
        }
    }

    uint64_t rate = Query(flowkey);
    uint64_t Th = GetThBinary(flowkey, tag);
    tag.m_isBigFlow = rate >= Th;
    tag.m_R = m_totalBytes * 8 / (Simulator::Now() - m_startTime).GetSeconds();
    tag.m_card = GetCard();
//    if (Simulator::Now().GetSeconds() > 2.4)
//    {
//        printf("At sketch, Time: %.6lf SourceIP: %d\n", Simulator::Now().GetSeconds(), flowkey.sip);
//        printf("\tmeasured rate: %lu; Threshold: %lu\n", rate, Th);
//    }

    NS_LOG_DEBUG("m_R = " << tag.m_R << ", m_totalBytes = " << m_totalBytes << std::endl);
}

uint64_t 
MySketch::Query(const FiveTuple &flowkey)
{
    std::vector<uint64_t> re(m_nWindows);
    std::vector<double> tm(m_nWindows);
    for (int i = 0; i < m_nRows; i++)
    {
        int key = Hash(flowkey, i) % m_nColumns;
        for (int j = 0; j < m_nWindows; j++)
        {
            m_counters[j][i][key].update(0, m_cntWindows, m_nWindows);
            if (!i || re[j] > m_counters[j][i][key].n)
            { 
                re[j] = m_counters[j][i][key].n;
            }
            if (!i || tm[j] > m_counters[j][i][key].t) 
            {
                tm[j] = m_counters[j][i][key].t;
            }
        }
    }
    uint64_t sz = 0;
    double start = 1e10;
    for (int i = 0; i < m_nWindows; i++) if(re[i]) 
    {
        sz += re[i];
        if (start > tm[i]) start = tm[i];
    }
    double delta = Simulator::Now().GetSeconds() - start;
    //double delta = (Simulator::Now() - m_startTime).GetSeconds();
    double rate = fabs(delta) < 1e-12 ? 25e9 : sz * 8 / delta;
    return rate;
}

void
MySketch::NextWindow()
{
    m_curWindow = (m_curWindow + 1) % m_nWindows;
    if (++m_cntWindows >= m_nWindows)
    {
        m_startTime += m_windowSize;
    }

    // printf("%.6lf %d %lu\n", Simulator::Now().GetSeconds(), m_card, GetTh());

    m_card -= m_cntB[m_curWindow];
    m_cntB[m_curWindow] = 0;    

    m_totalBytes -= m_bytes[m_curWindow];
    m_bytes[m_curWindow] = 0;

    Simulator::Schedule(m_windowSize, &MySketch::NextWindow, this);
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
