#include "ns3/log.h"
#include "ns3/persistent-rdma-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("PersistentRdmaClient");
NS_OBJECT_ENSURE_REGISTERED(PersistentRdmaClient);

TypeId
PersistentRdmaClient::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PersistentRdmaClient")
        .SetParent<RdmaClient>()
        .AddConstructor<PersistentRdmaClient>()
        ;
    return tid;
}

PersistentRdmaClient::PersistentRdmaClient()
{
    NS_LOG_FUNCTION_NOARGS();
}

PersistentRdmaClient::~PersistentRdmaClient()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
PersistentRdmaClient::StartApplication()
{
    NS_LOG_FUNCTION_NOARGS();
    Ptr<Node> node = GetNode();
    m_rdma = node->GetObject<RdmaDriver>();
    m_rdma->AddQueuePair(m_messageSize, m_pg, 
                       m_sip, m_dip, m_sport, m_dport, 
                       m_win, m_baseRtt, m_endTime,
                       MakeCallback(&RdmaClient::Finish, this));
    m_rdma->SetAppContrained(m_pg, m_dip, m_sport, true);
    m_nxtMessage = Simulator::Schedule(Seconds(m_gap->GetValue()), 
                                       &PersistentRdmaClient::NewMessage, this);
    Simulator::Schedule(m_msgGenEndTime, &PersistentRdmaClient::EndGenMessage, this);
}

void 
PersistentRdmaClient::StopApplication()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
PersistentRdmaClient::NewMessage()
{
    NS_LOG_FUNCTION_NOARGS();
    m_rdma->NewMessage(m_dip, m_sport, m_pg, m_messageSize);
    m_nxtMessage = Simulator::Schedule(Seconds(m_gap->GetValue()),
                                       &PersistentRdmaClient::NewMessage, this);
}

void 
PersistentRdmaClient::EndGenMessage()
{
    m_nxtMessage.Cancel();
    m_rdma->SetAppContrained(m_pg, m_dip, m_sport, false);
}

}