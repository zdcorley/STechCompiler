# STechCompiler
STechCompiler is a compiler for the meta shader language "ShaderTech" (made up by me) that compiles down to spirv for use in Vulkan based rendering applications. It is being developed along side a Vulkan based renderer I am working on, and is in a pretty early, though workable stage. Features will be added on as that Vulkan renderer gets support for those features.

You can file sample files of the language as well as a brief language spec in the samples directory.

## Motivation
The stech language fixes 4 main annoyances I had with glsl shader dev in the default Vulkan SDK:
  - Allows multiple shaders/shader stages to be written within a single file. (This can be done with preprocessor defines, but reads poorly)
  - Automatically assigns layout locations of "in/out" vars transferred between the vert and frag shaders.
  - Automatically assigns layout locations of "in" vars in vert shaders and "out" vars in frag shaders based on configurable hlsl style semantics.
  - Automatically assigns layout bindings of uniforms.
BUT, I didn't want to fully parse all the syntax of glsl to make it happen, so stechc takes a bit of a easier approach by considering the internals of main function definitions to just be dumb blocks of raw text, and doing the same for BEGINRAW/ENDRAW blocks.

## Features
  - Supports automatic binding of uniforms, with optional explicit binding using the normal syntax (layout(binding = x))
  - Supports raw glsl blocks for structs, helper functions, and when the language really doesnt do what you want.
  - Supports #include, #ifdef and #define directives
  - Supports input and output semantics whose meanings are determined by a config file.
  - Supports many shaders defined within a single stech file. Any number of shadertech "<TechniqueName>" blocks may be defined in one file.

## Limitations and Oddities
  - Only supports vertex and fragment shaders at the moment.
  - No support for structured buffers
  - No support for layout parameters other than "binding = x"
  - ALL uniforms are compiled with the std140 data storage layout.
  - The output shader stages' main functions are always named "main"
  - glsl built in variables are still referenced the normal way, such as gl_Position or gl_VertexID. Dont define them in your transfer blocks.

## Requirements
 STechCompiler relies on C++17 support for <filesystem> usage, as well as an installed version of the vulkan sdk on your PC for glslc. (Using the %VULKAN_SDK% env variable)
  
## Building
 - Compile stechc.l and stechc.y with flex and bison. (flex/bison not included in this repo)
 - Include the flex/bison output files in the visual studio solution
 - Build and run
