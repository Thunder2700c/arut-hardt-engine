#include "arut/rendering/imported_skinning.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace arut;

static void identity(std::array<float,16>& m) {
    m.fill(0.0F);
    m[0]=m[5]=m[10]=m[15]=1.0F;
}

int main() {
    animation::SkeletalAsset asset{};
    asset.joint_count=2U;
    asset.joints[0].name="Child";
    asset.joints[0].parent=1;
    asset.joints[1].name="Root";
    asset.joints[1].parent=-1;
    identity(asset.joints[0].inverse_bind_matrix);
    identity(asset.joints[1].inverse_bind_matrix);

    animation::Pose pose{};
    pose.joint_count=2U;
    pose.joints[0].translation={0.0F,1.0F,0.0F};
    pose.joints[1].translation={2.0F,0.0F,0.0F};

    rendering::ImportedSkinningPalette palette{};
    assert(rendering::build_imported_skinning_palette(asset,pose,palette));
    assert(palette.joint_count==2U);
    assert(std::abs(palette.matrices[0].value[12]-2.0F)<1.0e-5F);
    assert(std::abs(palette.matrices[0].value[13]-1.0F)<1.0e-5F);
    assert(std::abs(palette.matrices[1].value[12]-2.0F)<1.0e-5F);

    asset.joints[1].parent=0;
    assert(!rendering::build_imported_skinning_palette(asset,pose,palette));
    assert(palette.joint_count==0U);

    asset.joints[1].parent=-1;
    pose.joints[0].rotation={0.0F,0.0F,0.0F,0.0F};
    assert(!rendering::build_imported_skinning_palette(asset,pose,palette));
    std::cout<<"IMPORTED_SKINNING_TEST pass reverse_parent_order=1 cycle_rejection=1\n";
}
