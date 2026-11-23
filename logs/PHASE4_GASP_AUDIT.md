# Phase 4 GASP-style animation audit log

Date: 2026-11-23
Branch: main

## Repository audit

- Existing reference skeleton: 20-bone humanoid.
- Existing skinning: GPU skinning with four vertex influences and a 20-matrix palette.
- Existing animation: procedural state/pose generation for Idle, Walk, Run, StrafeLeft, StrafeRight, Backward, Jump, Fall and Land.
- Existing IK: terrain-aware analytic two-bone foot IK.
- Missing from current implementation: imported skeletal animation clips, glTF skeletal-animation importer, animation graph, pose history, predicted trajectory search, pose-search database, motion matching, retargeting pipeline, full-body IK, powered ragdoll.
- Existing Android renderer currently owns the mannequin animation update path; this must be decoupled into a reusable runtime system.
- Physics status: kinematic character controller/static AABB test arena only; no integrated general rigid-body backend or ragdoll.

## Reference study

Epic's public GASP documentation describes Motion Matching using pose search databases and trajectory/pose history, plus contextual database selection, orientation warping, leg IK, root offsets and traversal examples. The project is an animation sample rather than a single portable subsystem. ARUT-HARDT will implement original C++ equivalents and use appropriately licensed assets.

## Scope reminder

The forgotten adjacent feature is the reusable vehicle/car system already listed as Phase 5 in docs/ROADMAP.md. It remains after the Phase 4 character/animation gate.

## Current result

- Added docs/GASP_STYLE_ANIMATION_PLAN.md as the architecture and implementation baseline.
- No runtime feature is claimed complete by this planning commit.
