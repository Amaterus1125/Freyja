// the basic imports we have to do everytime

// PLS PUT THE BIN FILE WITH THE GLTF MODEL FILE FOR IT TO LOAD , BC THE BIN FILE CONTAINS THE JSON DATA FOR THE BUFFER
//FIXED THE WIREFRAME ISSUE

package main

import "core:fmt"
import "core:math/linalg"
import "vendor:glfw"
import gl "vendor:OpenGL"
import cgltf "vendor:cgltf"

/* What this this file do - In cpp you use Assimp to load a 3d model file , assimp is a big c++ library and has no odin versions 
but since the example we are taking is gltf model , we can use cgltf instead which is an already build odin library 
and precompiled for windows OS */

//The big idea is to open a 3d model file , pull out just the vertex position and upload them to the gpu and draw them.

PerFrameData :: struct {
  mvp: matrix[4,4]f32,
}

main :: proc() {
      glfw.Init() 
      defer glfw.Terminate() 
      glfw.WindowHint(glfw.CONTEXT_VERSION_MAJOR, 4)
	   glfw.WindowHint(glfw.CONTEXT_VERSION_MINOR, 6)
	   glfw.WindowHint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)

	   window := glfw.CreateWindow(800, 600, "glTF Model", nil, nil)
	   glfw.MakeContextCurrent(window)
	   gl.load_up_to(4, 6, glfw.gl_set_proc_address)
	   gl.Enable(gl.DEPTH_TEST)

// loading the model file 
options : cgltf.options // leave this zeroed/default , we don't need any specific settings 
data , parse_result := cgltf.parse_file(options, "Avocado.gltf")
if parse_result != .success {
       fmt.println("FAILED TO FIND THE PARSED GLTF FILE:" , parse_result)
       return
}
defer cgltf.free(data)  // free the loaded model data once main() ends 

/* Parsing only reads the JSON structure of the file , the actual raw vertex numbers and fragment shader lives 
in a seperate binary buffers that will still nees to load in as a second step */

load_result := cgltf.load_buffers(options , data , "Avocado.gltf")
if load_result != .success {
   fmt.println("FAILED TO LOAD THE GLTF BUFFERS:", load_result)
   return
}

// Pull out just the vertex positions
positions : [dynamic][3]f32
defer delete(positions)

mesh := data.meshes[0] //just grab the first mesh , like the only first mesh it can find 
prim := mesh.primitives[0] // and its first primitive attributes 

//find the position attribute among this primitive's attributes 
pos_accessor : ^cgltf.accessor
for attr in prim.attributes {
     if attr.type == .position { 
           pos_accessor = attr.data
} }

if prim.indices != nil {
 /* the model must have an index list ( most of them do) walk through it and for each index look up that vertex actual xyz position 
this flattens everything into one long triangle list and uses a manual face flattening loop  */
index_count := prim.indices.count 
for i in 0 ..< index_count {
    idx := cgltf.accessor_read_index(prim.indices , i) 
    v : [3]f32 
    ok := cgltf.accessor_read_float(pos_accessor , idx , &v[0] , 3)
    if !ok {
       continue    //skip this vertex if reading it somehow failed 
 }
  append(&positions , [3]f32{v.x,v.y,v.z})
}
}
// UPLOAD THE POSITIONS TO THE GPU 
num_vertices := i32(len(positions)) 
fmt.println("LOADED" , num_vertices , "VERTICES")

vao: u32
	gl.GenVertexArrays(1, &vao)
	gl.BindVertexArray(vao)

	vbo: u32
	gl.GenBuffers(1, &vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(positions) * size_of([3]f32), raw_data(positions), gl.STATIC_DRAW)

	gl.EnableVertexAttribArray(0)
	gl.VertexAttribPointer(0, 3, gl.FLOAT, false, size_of([3]f32), 0)

	// vertex and fragment SHADERS - plain position-only, driven by an MVP matrix, same idea as our cube
	vertex_src := `#version 460 core
layout (location = 0) in vec3 a_pos;
layout (std140, binding = 0) uniform PerFrameData { mat4 MVP; };
void main() {
	gl_Position = MVP * vec4(a_pos, 1.0);
}`

	fragment_src := `#version 460 core
layout (location = 1) uniform vec4 u_color;
out vec4 out_color;
void main() {
	out_color = u_color; 
}`

	vs := gl.CreateShader(gl.VERTEX_SHADER)
	src1 := cstring(raw_data(vertex_src))
	gl.ShaderSource(vs, 1, &src1, nil)
	gl.CompileShader(vs)

	fs := gl.CreateShader(gl.FRAGMENT_SHADER)
	src2 := cstring(raw_data(fragment_src))
	gl.ShaderSource(fs, 1, &src2, nil)
	gl.CompileShader(fs)

	program := gl.CreateProgram()
	gl.AttachShader(program, vs)
	gl.AttachShader(program, fs)
	gl.LinkProgram(program)
	gl.DeleteShader(vs)
	gl.DeleteShader(fs)

	// a small buffer that holds just our MVP matrix, shared with the shader every frame
	per_frame_buf: u32
	gl.CreateBuffers(1, &per_frame_buf)
	gl.NamedBufferStorage(per_frame_buf, size_of(PerFrameData), nil, gl.DYNAMIC_STORAGE_BIT)
	gl.BindBufferRange(gl.UNIFORM_BUFFER, 0, per_frame_buf, 0, size_of(PerFrameData))

	for !glfw.WindowShouldClose(window) {
		width, height := glfw.GetFramebufferSize(window)
		gl.Viewport(0, 0, width, height)
		gl.ClearColor(0.1, 0.1, 0.1, 1.0)
		gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

		aspect := f32(width) / f32(height)
		p := linalg.matrix4_perspective_f32(linalg.to_radians(f32(60.0)), aspect, 0.1, 100.0)
		view := linalg.matrix4_translate_f32({0.0, -3.0, -12.0}) // pull the camera back so the model is in view
		
        // to make it spin on its own hehe
        time := f32(glfw.GetTime())
		model := linalg.matrix4_rotate_f32(time, {0.0, 1.0, 0.0})
		mvp := p * view * model

		frame_data := PerFrameData{mvp = mvp}
		gl.NamedBufferSubData(per_frame_buf, 0, size_of(PerFrameData), &frame_data)

		gl.UseProgram(program)
		gl.BindVertexArray(vao)

		// same two-pass trick as the book: solid fill, then wireframe on top
		gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
		gl.Uniform4f(1, 1.0, 0.7, 0.2, 1.0) // Set solid color to yellow
		gl.DrawArrays(gl.TRIANGLES, 0, num_vertices)

		gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)
		gl.Uniform4f(1, 0.0, 0.0, 0.0, 1.0) // Set wireframe color to black
		gl.DrawArrays(gl.TRIANGLES, 0, num_vertices)

		glfw.SwapBuffers(window)
		glfw.PollEvents()
	}

	gl.DeleteBuffers(1, &per_frame_buf)
	gl.DeleteBuffers(1, &vbo)
	gl.DeleteVertexArrays(1, &vao)
	gl.DeleteProgram(program)
}



