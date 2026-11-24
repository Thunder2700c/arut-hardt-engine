#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "arut/animation/gltf_importer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <utility>

namespace arut::animation {
namespace {

struct DataDeleter final {
    void operator()(cgltf_data* data) const noexcept { if (data) cgltf_free(data); }
};
using DataPtr = std::unique_ptr<cgltf_data, DataDeleter>;

bool finite(float v) noexcept { return std::isfinite(v); }

const cgltf_attribute* attribute(const cgltf_primitive& primitive,
                                cgltf_attribute_type type) noexcept {
    for (cgltf_size i = 0; i < primitive.attributes_count; ++i) {
        const auto& a = primitive.attributes[i];
        if (a.type == type && a.index == 0) return &a;
    }
    return nullptr;
}

bool unpack(const cgltf_accessor* accessor, cgltf_size components,
            std::vector<float>& values) {
    if (!accessor || cgltf_num_components(accessor->type) != components ||
        accessor->count == 0U ||
        accessor->count > std::numeric_limits<std::size_t>::max() / components)
        return false;
    values.resize(static_cast<std::size_t>(accessor->count * components));
    return cgltf_accessor_unpack_floats(accessor, values.data(), values.size()) ==
           values.size();
}

std::int16_t joint_index(const cgltf_skin& skin, const cgltf_node* node) noexcept {
    for (cgltf_size i = 0; i < skin.joints_count; ++i)
        if (skin.joints[i] == node) return static_cast<std::int16_t>(i);
    return -1;
}

bool same_skin_layout(const cgltf_skin& reference, const cgltf_skin& candidate) noexcept {
    if (reference.joints_count == 0U ||
        reference.joints_count != candidate.joints_count) return false;
    for (cgltf_size i = 0; i < reference.joints_count; ++i)
        if (reference.joints[i] != candidate.joints[i]) return false;

    const auto read_bind = [](const cgltf_accessor* accessor, cgltf_size index,
                              std::array<float, 16>& matrix) noexcept {
        matrix.fill(0.0F);
        if (accessor)
            return cgltf_accessor_read_float(accessor, index, matrix.data(), 16U) != 0;
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0F;
        return true;
    };
    std::array<float, 16> reference_bind{};
    std::array<float, 16> candidate_bind{};
    for (cgltf_size i = 0; i < reference.joints_count; ++i) {
        if (!read_bind(reference.inverse_bind_matrices, i, reference_bind) ||
            !read_bind(candidate.inverse_bind_matrices, i, candidate_bind)) return false;
        for (std::size_t component = 0; component < reference_bind.size(); ++component)
            if (!finite(reference_bind[component]) || !finite(candidate_bind[component]) ||
                std::abs(reference_bind[component] - candidate_bind[component]) > 1.0e-5F)
                return false;
    }
    return true;
}

Quaternion normalized(Quaternion q) noexcept {
    const float l2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (!finite(l2) || l2 < 1.0e-12F) return {};
    const float inv = 1.0F / std::sqrt(l2);
    return {q.x*inv, q.y*inv, q.z*inv, q.w*inv};
}

Quaternion slerp(Quaternion a, Quaternion b, float t) noexcept {
    a = normalized(a); b = normalized(b);
    float d = a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
    if (d < 0.0F) { d=-d; b={-b.x,-b.y,-b.z,-b.w}; }
    if (d > 0.9995F)
        return normalized({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,
                           a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t});
    d=std::clamp(d,-1.0F,1.0F);
    const float angle=std::acos(d), sine=std::sin(angle);
    if (std::abs(sine)<1.0e-7F) return a;
    const float x=std::sin((1.0F-t)*angle)/sine;
    const float y=std::sin(t*angle)/sine;
    return normalized({x*a.x+y*b.x,x*a.y+y*b.y,
                       x*a.z+y*b.z,x*a.w+y*b.w});
}

struct Track final {
    const cgltf_animation_channel* channel{nullptr};
    std::vector<float> times;
    std::vector<float> values;
    std::size_t components{0U};
    bool cubic{false};
};

bool sample_track(const Track& track, float time, float* result) noexcept {
    if (track.times.empty() || track.components == 0U) return false;
    const std::size_t count=track.times.size();
    auto upper=std::upper_bound(track.times.begin(),track.times.end(),time);
    std::size_t lo=upper==track.times.begin()?0U:
        static_cast<std::size_t>((upper-track.times.begin())-1);
    std::size_t hi=std::min(lo+1U,count-1U);
    const float dt=track.times[hi]-track.times[lo];
    const float t=hi==lo||dt<=1.0e-7F?0.0F:
        std::clamp((time-track.times[lo])/dt,0.0F,1.0F);
    const auto value_index=[&](std::size_t key) {
        return (key*(track.cubic?3U:1U)+(track.cubic?1U:0U))*track.components;
    };
    const float* a=&track.values[value_index(lo)];
    const float* b=&track.values[value_index(hi)];
    if (track.channel->sampler->interpolation==cgltf_interpolation_type_step || hi==lo) {
        std::copy(a,a+track.components,result);
        return true;
    }
    if (!track.cubic) {
        if (track.channel->target_path==cgltf_animation_path_type_rotation &&
            track.components==4U) {
            const auto q=slerp({a[0],a[1],a[2],a[3]},
                               {b[0],b[1],b[2],b[3]},t);
            result[0]=q.x; result[1]=q.y; result[2]=q.z; result[3]=q.w;
        } else {
            for(std::size_t c=0;c<track.components;++c)
                result[c]=a[c]+(b[c]-a[c])*t;
        }
        return true;
    }
    // glTF CUBICSPLINE stores [in tangent, value, out tangent] per key.
    const float* out_tangent=&track.values[(lo*3U+2U)*track.components];
    const float* in_tangent=&track.values[(hi*3U)*track.components];
    const float t2=t*t,t3=t2*t;
    const float h00=2.0F*t3-3.0F*t2+1.0F;
    const float h10=t3-2.0F*t2+t;
    const float h01=-2.0F*t3+3.0F*t2;
    const float h11=t3-t2;
    for(std::size_t c=0;c<track.components;++c)
        result[c]=h00*a[c]+h10*dt*out_tangent[c]+h01*b[c]+h11*dt*in_tangent[c];
    if(track.channel->target_path==cgltf_animation_path_type_rotation && track.components==4U) {
        const auto q=normalized({result[0],result[1],result[2],result[3]});
        result[0]=q.x; result[1]=q.y; result[2]=q.z; result[3]=q.w;
    }
    return true;
}

bool import_mesh_primitive(const cgltf_primitive& primitive,
                           std::size_t joint_count,
                           ImportedSkinnedMesh& mesh) {
    if (primitive.type != cgltf_primitive_type_triangles) return false;
    const auto* position=attribute(primitive,cgltf_attribute_type_position);
    const auto* normal=attribute(primitive,cgltf_attribute_type_normal);
    const auto* uv=attribute(primitive,cgltf_attribute_type_texcoord);
    const auto* joints=attribute(primitive,cgltf_attribute_type_joints);
    const auto* weights=attribute(primitive,cgltf_attribute_type_weights);
    if(!position || !joints || !weights) return false;
    std::vector<float> p,n,t,j,w;
    if(!unpack(position->data,3U,p) || !unpack(joints->data,4U,j) ||
       !unpack(weights->data,4U,w)) return false;
    const std::size_t vertex_count=position->data->count;
    if(joints->data->count!=vertex_count || weights->data->count!=vertex_count)
        return false;
    if(normal && (!unpack(normal->data,3U,n)||normal->data->count!=vertex_count)) return false;
    if(uv && (!unpack(uv->data,2U,t)||uv->data->count!=vertex_count)) return false;
    mesh.vertices.reserve(mesh.vertices.size()+vertex_count);
    for(std::size_t i=0;i<vertex_count;++i) {
        ImportedSkinnedVertex v{};
        v.position={p[i*3U],p[i*3U+1U],p[i*3U+2U]};
        if(normal) v.normal={n[i*3U],n[i*3U+1U],n[i*3U+2U]};
        if(uv) { v.uv[0]=t[i*2U]; v.uv[1]=t[i*2U+1U]; }
        float sum=0.0F;
        for(std::size_t k=0;k<4U;++k) {
            const float ji=j[i*4U+k], wi=w[i*4U+k];
            if(!finite(ji)||!finite(wi)||ji<0.0F||wi<0.0F||
               ji>=static_cast<float>(joint_count)||std::floor(ji)!=ji) return false;
            v.joints[k]=static_cast<std::uint16_t>(ji);
            v.weights[k]=wi;
            sum+=wi;
        }
        if(!finite(v.position.x)||!finite(v.position.y)||!finite(v.position.z)||
           !finite(sum)||sum<=1.0e-8F) return false;
        for(float& weight:v.weights) weight/=sum;
        mesh.vertices.push_back(v);
    }
    const std::uint32_t base=static_cast<std::uint32_t>(mesh.vertices.size()-vertex_count);
    if(primitive.indices) {
        if(primitive.indices->count%3U!=0U) return false;
        mesh.indices.reserve(mesh.indices.size()+primitive.indices->count);
        for(cgltf_size i=0;i<primitive.indices->count;++i) {
            const auto index=cgltf_accessor_read_index(primitive.indices,i);
            if(index>=vertex_count) return false;
            mesh.indices.push_back(base+static_cast<std::uint32_t>(index));
        }
    } else {
        if(vertex_count%3U!=0U) return false;
        for(std::size_t i=0;i<vertex_count;++i)
            mesh.indices.push_back(base+static_cast<std::uint32_t>(i));
    }
    return true;
}

} // namespace

bool SkeletalAsset::valid() const noexcept {
    if(joint_count==0U||joint_count>kMaxImportedJoints||meshes.empty()||animations.empty())
        return false;
    for(std::size_t i=0;i<joint_count;++i) {
        if(joints[i].name.empty()||joints[i].parent < -1||
           (joints[i].parent>=0 &&
            (static_cast<std::size_t>(joints[i].parent)>=joint_count ||
             static_cast<std::size_t>(joints[i].parent)==i))) return false;
        std::array<bool,kMaxImportedJoints> visited{};
        std::size_t ancestor=i;
        for(std::size_t depth=0;depth<joint_count;++depth) {
            if(visited[ancestor]) return false;
            visited[ancestor]=true;
            const auto parent=joints[ancestor].parent;
            if(parent<0) break;
            ancestor=static_cast<std::size_t>(parent);
        }
        const auto& t=joints[i].rest_local;
        const auto& q=t.rotation;
        const float q2=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
        if(!finite(t.translation.x)||!finite(t.translation.y)||!finite(t.translation.z)||
           !finite(t.scale.x)||!finite(t.scale.y)||!finite(t.scale.z)||
           !finite(q.x)||!finite(q.y)||!finite(q.z)||!finite(q.w)||
           !finite(q2)||q2<=1.0e-12F) return false;
        for(float value:joints[i].inverse_bind_matrix) if(!finite(value)) return false;
    }
    for(const auto& mesh:meshes) {
        if(mesh.vertices.empty()||mesh.indices.empty()||mesh.indices.size()%3U!=0U) return false;
        for(const auto& vertex:mesh.vertices) {
            if(!finite(vertex.position.x)||!finite(vertex.position.y)||!finite(vertex.position.z)||
               !finite(vertex.normal.x)||!finite(vertex.normal.y)||!finite(vertex.normal.z)||
               !finite(vertex.uv[0])||!finite(vertex.uv[1])) return false;
            float sum=0.0F;
            for(std::size_t k=0;k<4U;++k) {
                if(vertex.joints[k]>=joint_count||!finite(vertex.weights[k])||vertex.weights[k]<0.0F)
                    return false;
                sum+=vertex.weights[k];
            }
            if(!finite(sum)||std::abs(sum-1.0F)>1.0e-3F) return false;
        }
        for(const auto index:mesh.indices) if(index>=mesh.vertices.size()) return false;
    }
    for(const auto& clip:animations)
        if(!clip.valid(joint_count)) return false;
    return true;
}

bool import_glb(const void* bytes, std::size_t byte_count,
                SkeletalAsset& output, GltfImportReport& report) {
    output={}; report={};
    if(!bytes||byte_count<20U) { report.message="GLB input is empty or truncated"; return false; }
    cgltf_options options{};
    cgltf_data* raw=nullptr;
    if(cgltf_parse(&options,bytes,byte_count,&raw)!=cgltf_result_success||!raw) {
        report.message="cgltf could not parse the input"; return false;
    }
    DataPtr data(raw);
    if(data->file_type!=cgltf_file_type_glb) {
        report.message="This importer accepts GLB 2.0 only; external .gltf resources are not resolved";
        return false;
    }
    if(cgltf_load_buffers(&options,data.get(),nullptr)!=cgltf_result_success ||
       cgltf_validate(data.get())!=cgltf_result_success) {
        report.message="GLB buffers or structural validation failed"; return false;
    }
    if(data->skins_count==0U||data->animations_count==0U) {
        report.message="GLB must contain at least one skin and one animation"; return false;
    }
    const cgltf_skin& skin=data->skins[0];
    if(skin.joints_count==0U||skin.joints_count>kMaxImportedJoints) {
        report.message="Skin joint count exceeds the current 128-joint runtime limit"; return false;
    }
    output.joint_count=skin.joints_count;
    std::unordered_map<const cgltf_node*,std::size_t> joint_map;
    for(cgltf_size i=0;i<skin.joints_count;++i) joint_map.emplace(skin.joints[i],i);
    for(cgltf_size i=0;i<skin.joints_count;++i) {
        const cgltf_node* node=skin.joints[i];
        auto& joint=output.joints[i];
        joint.name=node->name&&node->name[0]?node->name:("joint_"+std::to_string(i));
        const auto parent=joint_map.find(node->parent);
        joint.parent=parent==joint_map.end()?-1:static_cast<std::int16_t>(parent->second);
        if(node->has_matrix) {
            report.message="Joint nodes using matrix transforms are not supported yet; export TRS transforms";
            output={}; return false;
        }
        if(node->has_translation)
            joint.rest_local.translation={node->translation[0],node->translation[1],node->translation[2]};
        if(node->has_rotation)
            joint.rest_local.rotation=normalized({node->rotation[0],node->rotation[1],node->rotation[2],node->rotation[3]});
        if(node->has_scale)
            joint.rest_local.scale={node->scale[0],node->scale[1],node->scale[2]};
        joint.inverse_bind_matrix.fill(0.0F);
        if(skin.inverse_bind_matrices) {
            float matrix[16]{};
            if(!cgltf_accessor_read_float(skin.inverse_bind_matrices,i,matrix,16U)) {
                report.message="Unable to read inverse bind matrix"; output={}; return false;
            }
            std::copy(matrix,matrix+16,joint.inverse_bind_matrix.begin());
        } else {
            joint.inverse_bind_matrix[0]=joint.inverse_bind_matrix[5]=
                joint.inverse_bind_matrix[10]=joint.inverse_bind_matrix[15]=1.0F;
        }
    }

    for(cgltf_size ni=0;ni<data->nodes_count;++ni) {
        const auto& node=data->nodes[ni];
        if(!node.mesh || !node.skin) continue;

        bool skin_belongs_to_asset = false;
        for (cgltf_size si = 0; si < data->skins_count; ++si) {
            if (node.skin == &data->skins[si]) {
                skin_belongs_to_asset = true;
                break;
            }
        }
        if (!skin_belongs_to_asset) {
            report.message = "A skinned mesh references a skin outside the parsed GLB";
            output = {};
            return false;
        }
        if (!same_skin_layout(skin, *node.skin)) {
            report.message = "GLB contains multiple incompatible skeletons or inverse-bind layouts; one SkeletalAsset can represent only one compatible rig";
            output = {};
            return false;
        }

        ImportedSkinnedMesh mesh{};
        mesh.name=node.mesh->name&&node.mesh->name[0]?node.mesh->name:
                  (node.name&&node.name[0]?node.name:"skinned_mesh");
        for(cgltf_size pi=0;pi<node.mesh->primitives_count;++pi) {
            if(!import_mesh_primitive(node.mesh->primitives[pi],output.joint_count,mesh)) {
                report.message="A skinned mesh primitive is unsupported or has invalid vertex data";
                output={}; return false;
            }
        }
        if(!mesh.vertices.empty()) output.meshes.push_back(std::move(mesh));
    }
    if(output.meshes.empty()) {
        report.message="No mesh node references the selected compatible skeleton";
        output={}; return false;
    }

    for(cgltf_size ai=0;ai<data->animations_count;++ai) {
        const auto& source=data->animations[ai];
        std::vector<Track> tracks;
        std::vector<float> key_times{0.0F};
        float duration=0.0F;
        for(cgltf_size ci=0;ci<source.channels_count;++ci) {
            const auto& channel=source.channels[ci];
            if(!channel.target_node||channel.target_path==cgltf_animation_path_type_weights) continue;
            const auto mapped=joint_map.find(channel.target_node);
            if(mapped==joint_map.end()) continue;
            if(channel.target_path!=cgltf_animation_path_type_translation&&
               channel.target_path!=cgltf_animation_path_type_rotation&&
               channel.target_path!=cgltf_animation_path_type_scale) continue;
            if(channel.sampler->interpolation==cgltf_interpolation_type_cubic_spline) {
                // Cubic splines are supported by the sampler below.
            }
            Track track{};
            track.channel=&channel;
            track.components=channel.target_path==cgltf_animation_path_type_rotation?4U:3U;
            track.cubic=channel.sampler->interpolation==cgltf_interpolation_type_cubic_spline;
            if(!unpack(channel.sampler->input,1U,track.times)||
               !unpack(channel.sampler->output,track.components,track.values)||
               track.times.size()<2U||
               track.values.size()!=track.times.size()*track.components*(track.cubic?3U:1U)) {
                report.message="Animation sampler accessors are malformed or unsupported";
                output={}; return false;
            }
            for(std::size_t k=0;k<track.times.size();++k) {
                if(!finite(track.times[k])||track.times[k]<0.0F||
                   (k&&track.times[k]<=track.times[k-1U])) {
                    report.message="Animation key times must be finite, nonnegative and increasing";
                    output={}; return false;
                }
                key_times.push_back(track.times[k]);
                duration=std::max(duration,track.times[k]);
            }
            tracks.push_back(std::move(track));
        }
        if(tracks.empty()||duration<=0.0F) continue;
        std::sort(key_times.begin(),key_times.end());
        key_times.erase(std::unique(key_times.begin(),key_times.end(),
            [](float a,float b){return std::abs(a-b)<1.0e-6F;}),key_times.end());
        AnimationClip clip{};
        clip.name=source.name&&source.name[0]?source.name:
                  "animation_"+std::to_string(ai);
        clip.duration_seconds=duration;
        clip.looping=true;
        clip.keys.reserve(key_times.size());
        for(float time:key_times) {
            PoseKey key{};
            key.time_seconds=time;
            key.pose.joint_count=output.joint_count;
            for(std::size_t j=0;j<output.joint_count;++j)
                key.pose.joints[j]=output.joints[j].rest_local;
            for(const auto& track:tracks) {
                const auto mapped=joint_map.find(track.channel->target_node);
                if(mapped==joint_map.end()) continue;
                float value[4]{};
                if(!sample_track(track,time,value)) {
                    report.message="Failed to sample animation channel"; output={}; return false;
                }
                auto& transform=key.pose.joints[mapped->second];
                switch(track.channel->target_path) {
                case cgltf_animation_path_type_translation:
                    transform.translation={value[0],value[1],value[2]}; break;
                case cgltf_animation_path_type_rotation:
                    transform.rotation=normalized({value[0],value[1],value[2],value[3]}); break;
                case cgltf_animation_path_type_scale:
                    transform.scale={value[0],value[1],value[2]}; break;
                default: break;
                }
            }
            clip.keys.push_back(std::move(key));
        }
        if(clip.keys.size()>=2U&&clip.valid(output.joint_count))
            output.animations.push_back(std::move(clip));
    }
    if(output.animations.empty()) {
        report.message="No supported skeletal animation channels were found"; output={}; return false;
    }
    if(!output.valid()) {
        report.message="Imported skeletal asset failed runtime validation"; output={}; return false;
    }
    report.imported_joints=output.joint_count;
    report.imported_meshes=output.meshes.size();
    report.imported_animations=output.animations.size();
    for(const auto& mesh:output.meshes) {
        report.imported_vertices+=mesh.vertices.size();
        report.imported_indices+=mesh.indices.size();
    }
    report.message="GLB skeletal asset imported successfully";
    return true;
}

} // namespace arut::animation
