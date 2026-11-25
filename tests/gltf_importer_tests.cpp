#include "arut/animation/gltf_importer.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {
void u32(std::vector<std::uint8_t>& out,std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v));
    out.push_back(static_cast<std::uint8_t>(v>>8U));
    out.push_back(static_cast<std::uint8_t>(v>>16U));
    out.push_back(static_cast<std::uint8_t>(v>>24U));
}
void u16(std::vector<std::uint8_t>& out,std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v));
    out.push_back(static_cast<std::uint8_t>(v>>8U));
}
void f32(std::vector<std::uint8_t>& out,float v) {
    std::uint32_t bits=0;
    std::memcpy(&bits,&v,sizeof(bits));
    u32(out,bits);
}
void pad4(std::vector<std::uint8_t>& out) {
    while(out.size()%4U) out.push_back(0U);
}
std::vector<std::uint8_t> make_animation_only_glb() {
    std::vector<std::uint8_t> bin;
    const std::size_t times=bin.size();
    f32(bin,0.F); f32(bin,1.F);
    const std::size_t times_len=bin.size()-times;
    const std::size_t translations=bin.size();
    for(float value:{0.F,0.F,0.F, 0.F,0.5F,0.F}) f32(bin,value);
    const std::size_t translations_len=bin.size()-translations;
    pad4(bin);
    const auto view=[&](std::size_t offset,std::size_t length) {
        return std::string("{\"buffer\":0,\"byteOffset\":")+std::to_string(offset)+
            ",\"byteLength\":"+std::to_string(length)+"}";
    };
    std::string json="{";
    json+="\"asset\":{\"version\":\"2.0\",\"generator\":\"ARUT animation-only test\"},";
    json+="\"buffers\":[{\"byteLength\":"+std::to_string(bin.size())+"}],";
    json+="\"bufferViews\":["+view(times,times_len)+","+view(translations,translations_len)+"],";
    json+="\"accessors\":[";
    json+="{\"bufferView\":0,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},";
    json+="{\"bufferView\":1,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],";
    json+="\"nodes\":[{\"name\":\"Root\"}],";
    json+="\"animations\":[{\"name\":\"AnimationOnlyWalk\",\"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"}],\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}}]}],";
    json+="\"scenes\":[{\"nodes\":[0]}],\"scene\":0}";
    while(json.size()%4U) json.push_back(' ');
    std::vector<std::uint8_t> glb;
    const std::uint32_t total=12U+8U+static_cast<std::uint32_t>(json.size())+
        8U+static_cast<std::uint32_t>(bin.size());
    u32(glb,0x46546C67U); u32(glb,2U); u32(glb,total);
    u32(glb,static_cast<std::uint32_t>(json.size())); u32(glb,0x4E4F534AU);
    glb.insert(glb.end(),json.begin(),json.end());
    u32(glb,static_cast<std::uint32_t>(bin.size())); u32(glb,0x004E4942U);
    glb.insert(glb.end(),bin.begin(),bin.end());
    return glb;
}
std::vector<std::uint8_t> make_glb() {
    std::vector<std::uint8_t> bin;
    const auto view=[&](std::size_t offset,std::size_t length) {
        return std::string("{\"buffer\":0,\"byteOffset\":")+std::to_string(offset)+
            ",\"byteLength\":"+std::to_string(length)+"}";
    };
    const std::size_t p=bin.size();
    for(float v:{0.F,0.F,0.F, 1.F,0.F,0.F, 0.F,1.F,0.F}) f32(bin,v);
    const std::size_t p_len=bin.size()-p;
    const std::size_t j=bin.size();
    for(int i=0;i<3;++i) for(int k=0;k<4;++k) bin.push_back(k==0?0U:0U);
    const std::size_t j_len=bin.size()-j;
    pad4(bin);
    const std::size_t w=bin.size();
    for(int i=0;i<3;++i) { f32(bin,1.F); f32(bin,0.F); f32(bin,0.F); f32(bin,0.F); }
    const std::size_t w_len=bin.size()-w;
    const std::size_t ix=bin.size();
    u16(bin,0); u16(bin,1); u16(bin,2);
    const std::size_t ix_len=bin.size()-ix;
    pad4(bin);
    const std::size_t ib=bin.size();
    for(int i=0;i<16;++i) f32(bin,(i%5)==0?1.F:0.F);
    const std::size_t ib_len=bin.size()-ib;
    const std::size_t tm=bin.size();
    f32(bin,0.F); f32(bin,1.F);
    const std::size_t tm_len=bin.size()-tm;
    const std::size_t tr=bin.size();
    for(float v:{0.F,0.F,0.F, 0.F,0.25F,0.F}) f32(bin,v);
    const std::size_t tr_len=bin.size()-tr;
    const std::size_t sc=bin.size();
    for(float v:{0.F,0.F,0.F, 1.F,1.F,1.F, 0.F,0.F,0.F,
                 0.F,0.F,0.F, 2.F,2.F,2.F, 0.F,0.F,0.F}) f32(bin,v);
    const std::size_t sc_len=bin.size()-sc;
    pad4(bin);

    std::string json="{";
    json+="\"asset\":{\"version\":\"2.0\",\"generator\":\"ARUT test\"},";
    json+="\"buffers\":[{\"byteLength\":"+std::to_string(bin.size())+"}],";
    json+="\"bufferViews\":["+view(p,p_len)+","+view(j,j_len)+","+view(w,w_len)+","+
        view(ix,ix_len)+","+view(ib,ib_len)+","+view(tm,tm_len)+","+view(tr,tr_len)+","+view(sc,sc_len)+"],";
    json+="\"accessors\":[";
    json+="{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},";
    json+="{\"bufferView\":1,\"componentType\":5121,\"count\":3,\"type\":\"VEC4\"},";
    json+="{\"bufferView\":2,\"componentType\":5126,\"count\":3,\"type\":\"VEC4\"},";
    json+="{\"bufferView\":3,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"},";
    json+="{\"bufferView\":4,\"componentType\":5126,\"count\":1,\"type\":\"MAT4\"},";
    json+="{\"bufferView\":5,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},";
    json+="{\"bufferView\":6,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"},";
    json+="{\"bufferView\":7,\"componentType\":5126,\"count\":6,\"type\":\"VEC3\"}],";
    json+="\"meshes\":[{\"name\":\"TestMesh\",\"primitives\":[{\"attributes\":{\"POSITION\":0,\"JOINTS_0\":1,\"WEIGHTS_0\":2},\"indices\":3}]}],";
    json+="\"nodes\":[{\"name\":\"MeshNode\",\"mesh\":0,\"skin\":0},{\"name\":\"Root\"},{\"name\":\"MeshNodeSecondary\",\"mesh\":0,\"skin\":1}],";
    json+="\"skins\":[{\"joints\":[1],\"inverseBindMatrices\":4},{\"joints\":[1],\"inverseBindMatrices\":4}],";
    json+="\"animations\":[{\"name\":\"Idle\",\"samplers\":[{\"input\":5,\"output\":6,\"interpolation\":\"LINEAR\"}],\"channels\":[{\"sampler\":0,\"target\":{\"node\":1,\"path\":\"translation\"}}]},";
    json+="{\"name\":\"ScalePulse\",\"samplers\":[{\"input\":5,\"output\":7,\"interpolation\":\"CUBICSPLINE\"}],\"channels\":[{\"sampler\":0,\"target\":{\"node\":1,\"path\":\"scale\"}}]}],";
    json+="\"scenes\":[{\"nodes\":[0,1,2]}],\"scene\":0}";
    while(json.size()%4U) json.push_back(' ');
    std::vector<std::uint8_t> glb;
    const std::uint32_t total=12U+8U+static_cast<std::uint32_t>(json.size())+
        8U+static_cast<std::uint32_t>(bin.size());
    u32(glb,0x46546C67U); u32(glb,2U); u32(glb,total);
    u32(glb,static_cast<std::uint32_t>(json.size())); u32(glb,0x4E4F534AU);
    glb.insert(glb.end(),json.begin(),json.end());
    u32(glb,static_cast<std::uint32_t>(bin.size())); u32(glb,0x004E4942U);
    glb.insert(glb.end(),bin.begin(),bin.end());
    return glb;
}
} // namespace

int main() {
    const auto bytes=make_glb();
    arut::animation::SkeletalAsset asset{};
    arut::animation::GltfImportReport report{};
    assert(arut::animation::import_glb(bytes.data(),bytes.size(),asset,report));
    assert(asset.valid());
    assert(asset.joint_count==1U);
    assert(asset.meshes.size()==2U);
    assert(asset.meshes[0].vertices.size()==3U);
    assert(asset.meshes[0].indices.size()==3U);
    assert(asset.meshes[1].vertices.size()==3U);
    assert(asset.meshes[1].indices.size()==3U);
    assert(asset.animations.size()==2U);
    assert(asset.animations[0].name=="Idle");
    arut::animation::Pose sampled{};
    assert(asset.animations[0].sample(1.0F,sampled));
    assert(sampled.joint_count==1U);
    assert(sampled.joints[0].translation.y>0.24F);
    assert(asset.animations[1].sample(0.5F,sampled));
    assert(sampled.joints[0].scale.x>1.49F&&sampled.joints[0].scale.x<1.51F);
    assert(report.imported_meshes==2U);
    assert(report.imported_vertices==6U);
    assert(report.imported_indices==6U);
    const auto animation_only_bytes=make_animation_only_glb();
    std::vector<arut::animation::AnimationClip> animation_only_clips;
    assert(arut::animation::import_glb_animation_clips(
        animation_only_bytes.data(),animation_only_bytes.size(),asset,
        animation_only_clips,report));
    assert(animation_only_clips.size()==1U);
    assert(animation_only_clips[0].name=="AnimationOnlyWalk");
    assert(animation_only_clips[0].sample(1.0F,sampled));
    assert(sampled.joint_count==1U);
    assert(sampled.joints[0].translation.y>0.49F);
    std::cout<<"GLTF_ANIMATION_ONLY_TEST pass clips=1 reference_joints=1\n";
    const std::uint8_t invalid[4]{};
    assert(!arut::animation::import_glb(invalid,sizeof(invalid),asset,report));
    std::cout<<"GLTF_IMPORTER_TEST pass joints=1 compatible_multi_skin_meshes=2 vertices=6 clips=2 linear+cubic\n";
}
