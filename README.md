
GLSL-PathTracer
==========
A physically based path tracer that runs in a GLSL fragment shader.

![Jinx](./screenshots/Jinx.png)

![WW2](./screenshots/ww2.png)

Features
--------
- Unidirectional path tracer
- BVH builder from RadeonRays 2.0
- Disney BSDF
- Texture Mapping (Albedo, Metallic-Roughness, Normal, Emission)
- Analytic Lights (Sphere, Rect, Directional)
- OpenImageDenoise
- Image based lighting with importance sampling
- Tile Rendering
- GLTF/GLB Support (In progress)

Build Instructions
--------
Please see INSTALL-WIN.txt for the build instructions for Windows and INSTALL-LINUX.txt for Linux

Sample Scenes
--------
A couple of sample scenes are provided in the repository. Additional scenes can be downloaded from here:
https://drive.google.com/file/d/1UFMMoVb5uB7WIvCeHOfQ2dCQSxNMXluB/view

Gallery
--------
![Slum](./screenshots/Slum.png)
--------
![RenderMan Swatch](./screenshots/Renderman_swatch.png)
--------
![Stormtrooper](./screenshots/stormtrooper.jpg)
--------
![Panther](./screenshots/panther.jpg)
--------
![Crown](./screenshots/crown.png)
--------
![Mustang_red](./screenshots/Mustang_Red.jpg)
--------
![Hyperion](./screenshots/hyperion.jpg)
--------
![Ajax](./screenshots/ajax_materials.png)
--------
![Dining Room](./screenshots/DiningRoom.jpg)
--------
![Rank3PoliceUnit](./screenshots/rank3police_color_corrected.png)
--------
![Staircase](./screenshots/staircase.png)
--------
![Substance Boy](./screenshots/MeetMat_Maps.png)
--------
![Dragon](./screenshots/dragon.jpg)

References/Credits
--------
- A huge shout-out to Cedric Guillemet (https://github.com/CedricGuillemet) for cleaning up the code, adding the UI, integrating ImGuizmo, cmake and quite a lot of fixes.
- Tinsel Renderer (https://github.com/mmacklin/tinsel) A modified version of the scene description & loader from that project is used here.
- Ray Tracing in One Weekend (https://github.com/petershirley/raytracinginoneweekend) Peter Shirley's excellent book introductory book on raytracing which helped me get started on this project.
- Erich Loftis's THREE.js PathTracer (https://github.com/erichlof/THREE.js-PathTracing-Renderer).
- OptiX Introduction Samples (https://github.com/nvpro-samples/optix_advanced_samples/tree/master/src/optixIntroduction)
- Mitsuba Renderer (https://github.com/mitsuba-renderer/mitsuba) Reference for validation.
- PBRT-v3 (https://github.com/mmp/pbrt-v3) which provides an endless source of knowledge. The online book is a gift that keeps giving.
- Model links/credits are in the assets folder (See: Model Credits.txt)
