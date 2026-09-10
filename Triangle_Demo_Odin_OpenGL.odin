// for odin to work on your pc with vulkan and opengl, downoad vulkan first , then git and then clone the odin repo , after that add it to env paths and use it in vs code 



package main

import "core:fmt"
import "vendor:glfw"
import gl "vendor:OpenGL"

GL_MAJOR_VERSION :: 3
GL_MINOR_VERSION :: 3


Vertex :: struct {
	pos:   [2]f32,
	color: [3]f32,
}

vertex_shader_source := `#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec3 fragColor;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    fragColor = aColor;
}
`

fragment_shader_source := `#version 330 core
in vec3 fragColor;
out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
`

framebuffer_size_callback :: proc "c" (window: glfw.WindowHandle, width, height: i32) {
	gl.Viewport(0, 0, width, height)
}

main :: proc() {

	if !glfw.Init() {
		fmt.eprintln("Failed to initialize GLFW")
		return
	}
	defer glfw.Terminate()

	glfw.WindowHint(glfw.CONTEXT_VERSION_MAJOR, GL_MAJOR_VERSION)
	glfw.WindowHint(glfw.CONTEXT_VERSION_MINOR, GL_MINOR_VERSION)
	glfw.WindowHint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)

	window := glfw.CreateWindow(800, 600, "OpenGL Triangle - Odin", nil, nil)
	if window == nil {
		fmt.eprintln("Failed to create GLFW window")
		return
	}
	defer glfw.DestroyWindow(window)

	glfw.MakeContextCurrent(window)
	glfw.SwapInterval(1) // vsync
	glfw.SetFramebufferSizeCallback(window, framebuffer_size_callback)

	// Load OpenGL function pointers up to 3.3 through GLFW's loader.
	gl.load_up_to(GL_MAJOR_VERSION, GL_MINOR_VERSION, glfw.gl_set_proc_address)


	vertices := [3]Vertex{
		{{0.0, 0.5}, {1, 0, 0}},   // top -- red
		{{0.5, -0.5}, {0, 1, 0}},  // bottom right -- green
		{{-0.5, -0.5}, {0, 0, 1}}, // bottom left -- blue
	}

	vao, vbo: u32
	gl.GenVertexArrays(1, &vao)
	gl.GenBuffers(1, &vbo)

	gl.BindVertexArray(vao)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	gl.BufferData(gl.ARRAY_BUFFER, auto_cast size_of(vertices), &vertices, gl.STATIC_DRAW)

	// Position attribute (location 0).
	gl.VertexAttribPointer(0, 2, gl.FLOAT, false, auto_cast size_of(Vertex), auto_cast offset_of(Vertex, pos))
	gl.EnableVertexAttribArray(0)

	// Color attribute (location 1).
	gl.VertexAttribPointer(1, 3, gl.FLOAT, false, auto_cast size_of(Vertex), auto_cast offset_of(Vertex, color))
	gl.EnableVertexAttribArray(1)

	gl.BindBuffer(gl.ARRAY_BUFFER, 0)
	gl.BindVertexArray(0)


	program, program_ok := gl.load_shaders_source(vertex_shader_source, fragment_shader_source)
	if !program_ok {
		fmt.eprintln("Failed to compile/link shaders")
		return
	}
	defer gl.DeleteProgram(program)


	for !glfw.WindowShouldClose(window) {
		if glfw.GetKey(window, glfw.KEY_ESCAPE) == glfw.PRESS {
			glfw.SetWindowShouldClose(window, true)
		}

		gl.ClearColor(0.01, 0.01, 0.02, 1.0) // near-black background
		gl.Clear(gl.COLOR_BUFFER_BIT)

		gl.UseProgram(program)
		gl.BindVertexArray(vao)
		gl.DrawArrays(gl.TRIANGLES, 0, 3)

		glfw.SwapBuffers(window)
		glfw.PollEvents()
	}

	gl.DeleteVertexArrays(1, &vao)
	gl.DeleteBuffers(1, &vbo)
}
