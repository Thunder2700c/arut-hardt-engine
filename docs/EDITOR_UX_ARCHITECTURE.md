# ARUT-HARDT Editor UX Architecture

## Design goal
Make the editor feel like a distinct, focused engine product—not an untouched Godot editor with only a new accent colour. The early target is a private group of five, so prioritize clarity, easy authoring and consistency over marketing/public-release work.

## Identity layer
- ARUT-HARDT wordmark and a simple geometric symbol.
- Original app icon, splash, project-manager identity, about/version surface and editor icon set.
- A defined colour, typography, spacing and corner-radius system.
- A dark theme as the primary mode, with readable contrast and accessible focus/hover states.
- Consistent ARUT Mobile and HARDT Studio identities with the same engine branding.
- Keep Godot credit and license notices in the about panel/docs and redistributed builds.

## Editor workspace
- **Scene / Nodes:** clear hierarchy, search and ARUT-specific categories.
- **3D Viewport:** main canvas with a compact toolbar for select/move/rotate/scale, snapping, camera and run controls.
- **Inspector:** properties grouped into Basic, Movement/Physics, Appearance, Input and Advanced sections; common settings first.
- **Assets:** a clear project file panel with asset-type filters and previews.
- **Diagnostics:** output/errors, CPU/GPU frame time, memory and physics metrics.
- **Mobile Preview:** device resolution/aspect presets and input-safe-area preview, with runtime emulation limitations clearly labelled.

The first implementation can retain Godot's reliable viewport/resource systems while customising the actual editor source, default layout, themes, icons, project manager and native panels. A theme alone is not the whole redesign.

## ARUT-native node browser
Top-level groups should include:
- **Vehicles:** Car3D, wheel/suspension tools, drivetrain presets.
- **Characters:** FPS/third-person controller, camera rig, ragdoll profile.
- **Interaction:** interactable, pickup/throw, surface/material response.
- **Destruction:** breakable, damage threshold, debris profile.
- **Rendering:** anime surface, outline/rim, quality profile.
- **Diagnostics:** vehicle telemetry, physics view and mobile performance panel.

Only show a feature when it is implemented. Do not add decorative menu entries that do nothing.

## Inspector conventions
- Sensible defaults allow a system to function after adding the node and assigning essential assets.
- Expose common controls first; group detailed tuning under expandable advanced sections.
- Provide presets that set coherent property bundles without locking the user out of manual editing.
- Add tooltips that explain units and effects in everyday terms.
- Show dependencies clearly (for example, Car3D needs a chassis collision shape and a visual mesh; the wheel probes are generated/managed by the node).
- Prefer typed C++ properties and engine-native inspector hints over custom GDScript inspectors where feasible.

## Implementation path
1. Add a small branded identity/theme resource set for source builds.
2. Patch Godot's editor defaults and application identity in reviewed, version-pinned source patches.
3. Add ARUT's built-in C++ classes and node categories to the engine source.
4. Add native editor panels only when the underlying feature provides real data.
5. Refresh patches and verify them against the pinned Godot source revision as one integration batch.

## Guardrails
- The engine remains based on Godot; branding must not falsely imply the underlying editor/runtime was created from scratch.
- Preserve scene/project compatibility between ARUT Mobile and HARDT Studio at matching engine versions.
- Do not copy proprietary UI code/assets from Unity or Unreal. Build original styles and icons.
- No user phone-test is required for every design iteration. Batch source-level UI work and include it in the private-alpha package.


## Initial source changes (not yet build-verified)

The first identity batch now has source assets at `engine/branding/` and patch `engine/patches/0001-arut-editor-identity.patch`. The patch adds an ARUT-HARDT theme preset with a dark navy base and turquoise accent, switches the default preset for fresh editor settings, and changes editor/project-manager title text. The Android and desktop workflows overlay the ARUT mark and custom Car3D/Character3D node icons. This is the start of the UI redesign, not a claim that the complete editor layout has already been redesigned.


## Native component icons and profiling node

The source build also overlays `ArutCar3D.svg`, `ArutCharacter3D.svg`, `ArutRagdollDummy3D.svg`, and `ArutRenderProfile3D.svg` into the editor icon directory. The corresponding classes are registered by the custom native module. Whether all icons appear in the expected node-browser contexts still needs confirmation from a compiled editor.
