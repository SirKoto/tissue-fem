#pragma once

#include <cereal/types/polymorphic.hpp>

// only to use with binary or json encoding
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

#include <cereal/types/list.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/unordered_set.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>

#include <cereal/access.hpp>

// To use the serialization, look at https://uscilab.github.io/cereal/polymorphism.html
#define TF_SERIALIZE_TYPE(T) CEREAL_REGISTER_TYPE(T)
#define TF_SERIALIZE_TYPE_WITH_NAME(T, name) CEREAL_REGISTER_TYPE_WITH_NAME(T, name)
#define TF_SERIALIZE_POLYMORPHIC_RELATION(Base, Derived) CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, Derived)
// Create Name Value Pair of Member, with a tag that removes the first 2 chars of the name
#define TF_SERIALIZE_NVP_MEMBER(Member) cereal::make_nvp((#Member) + 2, Member)

#define TF_SERIALIZE_PRIVATE_MEMBERS friend class cereal::access;

#define TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(Type) \
    template void Type::serialize<cereal::BinaryOutputArchive>(cereal::BinaryOutputArchive&); \
    template void Type::serialize<cereal::BinaryInputArchive>(cereal::BinaryInputArchive&); \
    template void Type::serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&); \
    template void Type::serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

// GLM serialization

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace cereal
{
// vec
template<typename Archive, typename T, glm::qualifier Q>
void serialize(Archive& archive,
    glm::vec<3, T, Q>& m)
{
    archive(
        cereal::make_nvp("x", m.x),
        cereal::make_nvp("y", m.y),
        cereal::make_nvp("z", m.z));
}

template<typename Archive, typename T, glm::qualifier Q>
void serialize(Archive& archive,
    glm::vec<2, T, Q>& m)
{
    archive(
        cereal::make_nvp("x", m.x),
        cereal::make_nvp("y", m.y));
}

template<typename Archive, typename T, glm::qualifier Q>
typename std::enable_if<traits::is_input_serializable<BinaryData<T>, Archive>::value, void>::type
serialize(Archive& archive,
    glm::mat<4, 4, T, Q>& m)
{
    archive(cereal::binary_data(glm::value_ptr(m), sizeof(m)));
}

template<typename Archive, typename T, glm::qualifier Q>
typename std::enable_if<!traits::is_input_serializable<BinaryData<T>, Archive>::value, void>::type
serialize(Archive& archive,
    glm::mat<4, 4, T, Q>& m)
{
    for (uint32_t i = 0; i < 4; ++i) {
        for (uint32_t j = 0; j < 4; ++j) {
            archive(m[i][j]);
        }
    }
}

} // namespace cereal