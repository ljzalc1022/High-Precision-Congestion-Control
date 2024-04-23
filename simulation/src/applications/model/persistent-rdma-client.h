#ifndef PERSISTENT_RDMA_CLIENT_H
#define PERSISTENT_RDMA_CLIENT_H

#include "ns3/random-variable-stream.h"
#include "ns3/rdma-client.h"
#include "ns3/rdma-driver.h"

namespace ns3 {

class PersistentRdmaClient : public RdmaClient
{
public:
    static TypeId GetTypeId();

    PersistentRdmaClient();
    virtual ~PersistentRdmaClient();

protected:
    Ptr<RdmaDriver> m_rdma;

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void NewMessage();

    uint32_t m_messageSize;
    Ptr<RandomVariableStream> m_gap;
    EventId m_nxtMessage;

    Time m_msgGenEndTime;
    void EndGenMessage();
};

}

#endif