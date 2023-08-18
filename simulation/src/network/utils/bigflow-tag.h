#ifndef BIGFLOW_TAG_H
#define BIGFLOW_TAG_H

#include "ns3/tag.h"

namespace ns3
{

class BigflowTag : public Tag 
{
public:
    BigflowTag();
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer buf) const override;
    void Deserialize(TagBuffer buf) override;
    void Print(std::ostream& os) const override;
};

}

#endif // BIGFLOW_TAG_H