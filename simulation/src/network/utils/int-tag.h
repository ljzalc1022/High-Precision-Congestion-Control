#ifndef INT_TAG_H
#define INT_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class IntTag : public Tag {
public:
    IntTag();
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer buf) const override;
    void Deserialize(TagBuffer buf) override;
    void Print(std::ostream &os) const override;
public:
    uint8_t m_isBigFlow;
    uint64_t m_Rb;
};

}

#endif // !INT_TAG_H