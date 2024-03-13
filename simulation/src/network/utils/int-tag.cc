#include "int-tag.h"

namespace ns3 {

IntTag::IntTag()
    : Tag()
{
}

TypeId
IntTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::IntTag")
                            .SetParent<Tag>()
                            .AddConstructor<IntTag>();
    return tid;
}

TypeId
IntTag::GetInstanceTypeId() const 
{
    return GetTypeId();
}

uint32_t
IntTag::GetSerializedSize() const 
{
    return 17;
}

void
IntTag::Serialize(TagBuffer buf) const 
{
    buf.WriteU8(m_isBigFlow);
    buf.WriteU64(m_Rb);
    buf.WriteU64(m_R);
}

void 
IntTag::Deserialize(TagBuffer buf)
{
    m_isBigFlow = buf.ReadU8();
    m_Rb = buf.ReadU64();
    m_R = buf.ReadU64();
}

void 
IntTag::Print(std::ostream& os) const 
{
    os << "INT tag: " << m_isBigFlow << ' ' << m_Rb << ' ' << m_R << std::endl;
}


}