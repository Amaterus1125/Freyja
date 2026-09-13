package main //package name must be main 
import "vendor:glfw"
import gl "vendor:OpenGL"
main :: proc(){
   glfw.Init()     //for start upsfor windows and input lib 
   defer glfw.Terminate()
//telling glfw we want an OPENGL 3.3 core profile context 
  glfw.WindowHint(glfw.CONTEXT_VERSION_MAJOR ,3)
  glfw.WindowHint(glfw.CONTEXT_VERSION_MINOR ,3)
  glfw.WindowHint(glfw.OPENGL_PROFILE , glfw.OPENGL_CORE_PROFILE)

win := glfw.CreateWindow(800 , 800 , "THE SCAPEGOAT LORD" ,nil,nil) //creating the window
glfw.MakeContextCurrent(win)  // make this window context active 
gl.load_up_to(3,3,glfw.gl_set_proc_address) //load opengl 3.3 functions 

// making vertices -- 
//each vertex has position (x,y,z) and color (r,g,b) packed together 
//layout per vertex : x,y,z andd r,g,b
vertices :=[18]f32{
    0.0 , 0.5, 0.0 ,    0.58 , 0.0 , 0.83 , //top vertex with some color 
  -0.5 , -0.5 , 0.0 ,   0.55 , 0.0 , 0.0 , //bottom left - any color , basically yr screen is between 1 , 0 , -1 // u can change the colors from here , no need for it to be all red blue and green 
   0.5 , -0.5 , 0.0 ,  0.22 , 1.0 , 0.08 ,  //bottom right with any color 
}

vao , vbo: u32 
gl.GenVertexArrays(1,&vao)   // vao (vertex array object) is for how to read vbo data , its a kind of recipe 
gl.GenBuffers(1, &vbo)   //vbo (vertex buffer object) is raw buffer of vertex numbers on the gpu 
gl.BindVertexArray(vao)  //start recording into this vao 
gl.BindBuffer(gl.ARRAY_BUFFER ,vbo) // this vbo is now the active array buffer 
gl.BufferData(gl.ARRAY_BUFFER , size_of(vertices) , &vertices , gl.STATIC_DRAW) // upload vertex data to the gpu 


stride := i32(6 * size_of(f32)) //each vertex takes 6floats (3 for position and 3 for color)

gl.VertexAttribPointer(0,3,gl.FLOAT, false , stride ,0)  //attribute 0 = position , starting at offset 0 
gl.EnableVertexAttribArray(0)

gl.VertexAttribPointer(1 ,3, gl.FLOAT,false , stride , 3 * size_of(f32)) // attribute 1 for color , 3 floats , starting after the 3 position floats 
gl.EnableVertexAttribArray(1)

//vertex shader runs per vertex
//takes in position + coloe , outputs position which is required and passess color onwards 
vertex_srcc := `#version 330 core 
layout(location =0) in vec3 aPos;
layout(location =1) in vec3 aColor;
out vec3 vColor;

void main(){
   gl_Position = vec4(aPos ,1.0);
   vColor = aColor ;
}`

//fragment shader runs once per pixel inside the triangle 
//GPU interpolates vcolor between the 3 vertices and gives out a smooth gradient 
frag_src := `#version 330 core 
in vec3 vColor;
out vec4 FragColor;

void main() {
   FragColor = vec4(vColor , 1.0);
}`

vs := gl.CreateShader(gl.VERTEX_SHADER)
src1 := cstring(raw_data(vertex_srcc))
gl.ShaderSource(vs , 1, &src1 , nil)
gl.CompileShader(vs)

fs := gl.CreateShader(gl.FRAGMENT_SHADER)
src2 := cstring(raw_data(frag_src))
gl.ShaderSource(fs , 1, &src2, nil)
gl.CompileShader(fs)

program := gl.CreateProgram() // programs are the linked pair of shaders we will draw with 
gl.AttachShader(program , vs)
gl.AttachShader(program , fs)
gl.LinkProgram(program)

gl.DeleteShader(vs) //no longer linked into the program to later save gpu memory 
gl.DeleteShader(fs)

//main loop runs once epr frame until window is closed 
for !glfw.WindowShouldClose(win) { 
    gl.ClearColor(0.0,0.0,0.0,0.0)  // set clear color to black , like the background color 
    gl.Clear(gl.COLOR_BUFFER_BIT) //clear the screen with that color

    gl.UseProgram(program)
    gl.BindVertexArray(vao)
    gl.DrawArrays(gl.TRIANGLES , 0, 3) //draw 3 vertices as a 1 triangle 
   glfw.SwapBuffers(win) // show the frame we just drew 
   glfw.PollEvents() //check for input events like the close button or resize ig
  }
}
