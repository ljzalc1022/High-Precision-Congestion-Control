#include "bigflow-tag.h"

namespace ns3
{

BigflowTag::BigflowTag()
{
}

TypeId 
BigflowTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BigflowTag")
                            .SetParent<Tag>()
                            .AddConstructor<BigflowTag>();
    return tid;
}

TypeId
BigflowTag::GetInstanceTypeId() const 
{
    return GetTypeId();
}

uint32_t
BigflowTag::GetSerializedSize() const 
{
    return 0;
}

void
BigflowTag::Serialize(TagBuffer buf) const 
{
}

void 
BigflowTag::Deserialize(TagBuffer buf) 
{
}

void 
BigflowTag::Print(std::ostream& os) const
{
    os << "bigflow tag" << std::endl;
}

}