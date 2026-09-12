GLuint VAO;
glCreateVertexArrays(1,&VAO);
Gluint handleVBO;
glCreateBuffers(1,&handleVBO);
glNamedBufferStorage(handleVBO,256*1024,nullptr,GL_DYNAMIC_STORAGE_BIT);
Glunit handleElements;
glCreateBuffers(1,&handleElements);
glNamedBufferStorage(handleElements,256*1024,nullptr,GL_DYNAMIC_STORAGE_BIT);
glVertexArraysElementBuffer(VAO,handleElements);
glVertexArraysVertexBuffer(VAO,0,handleVBO,0,sizeof(ImDrawVert));
glEnableVertexArrayAttrib(VAO,0);
glEnableVertexArrayAttrib(VAO,1);
glEnableVertexArrayAttrib(VAO,2);
struct ImDrawVert{
    ImVec2 pos;ImVec2 uv;ImU32 col;};
glVertexArrayAttribFormat(VAO,0,2,GL_FLOAT,GL_FALSE,IM_OFFSETOF(ImDrawVert,pos));
glVertexArrayAttribFormat(VAO,1,2,GL_FLOAT,GL_FALSE,IM_OFFSETOF(ImDrawVert,pos));
glVertexArrayAttribFormat(VAO,2,4,GL_FLOAT,GL_FALSE,IM_OFFSETOF(ImDrawVert,pos));
glEnableVertexArrayAttribBinding(VAO,0,0);
glEnableVertexArrayAttribBinding(VAO,1,0);
glEnableVertexArrayAttribBinding(VAO,2,0);
glBindVertexArray(VAO);
const GLchar* shaderCodeVertex = R"(
#version 460 core
layout(location=0) in vec2 Position;
layout(location=1) in vec UV;
layout(location=2) in vec4 Color;
layout(std140,binding=0) uniform PerFrameData{
    uniform mat4 MVP;};
out vec2 Frag_UV;
out vec4 Frag_Color;
void main(){
    Frag_UV = UV;
    Frag_Color = Color;
    gl_postion = MVP*vec4(Position.xy,0,1);})";
const GLchar* shaderCodeFragment = R"(
# version 460 core
in vec2 Frag_UV;
in in vec4 Frag_Color;
layout(binding=0) uniform sampler2D Texture;
layout(location=0) out vec4 out_Color;
void main(){
    out_color = Frag_Color *
texture(Texture,Frag_UV.st);};)";
// IMgui part
IMGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();
io.BackendFlags
| = ImGuiBackendFlags_RendererHasVtxOffset;
ImFontConfig cfg = ImFontConfig();
cfg.FOntDataOwnedByAtlas = false;
cfg.RasterizerMultiply = 1.5f;
cfg.SizePixels = 768.0f/32.0f;
cfg.PixelSnapH = true;
cfg.OversampleH =4;
cfg.OversampleV=4;
IMFont* Font =io.Fonts->AddFontFromFileTTF("data/OpenSans-Light.ttf",cfg.SizePixels,&cfg);
unsigned char* pixels = nullptr;
int width,height;
io.Font->GetTexDataAsRGBA32(&pixels,&width,&height);
GLuint texture;
glCreateTextures(GL_TEXTURE_2D,1,&texture);
glTextureParameteri(texture,GL_TEXTURE_MAX_LEVEL,0);
gl
