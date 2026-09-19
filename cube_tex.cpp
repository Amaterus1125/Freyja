#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace glm;

enum eBitmapType   { eBitmapType_2D, eBitmapType_Cube };
enum eBitmapFormat { eBitmapFormat_UnsignedByte, eBitmapFormat_Float };

class Bitmap {
public:
    Bitmap() = default;
    Bitmap(int w, int h, int comp, eBitmapFormat fmt) : Bitmap(w, h, 1, comp, fmt) {}
    Bitmap(int w, int h, int d, int comp, eBitmapFormat fmt)
        : w_(w), h_(h), d_(d), comp_(comp), fmt_(fmt),
          data_(w * h * d * comp * getBytesPerComponent(fmt)) {}
    Bitmap(int w, int h, int comp, eBitmapFormat fmt, const void* ptr) : Bitmap(w, h, comp, fmt) {
        memcpy(data_.data(), ptr, data_.size());}
    int w_ = 0, h_ = 0, d_ = 1, comp_ = 3;
    eBitmapFormat fmt_ = eBitmapFormat_UnsignedByte;
    eBitmapType type_ = eBitmapType_2D;
    std::vector<uint8_t> data_;
    static int getBytesPerComponent(eBitmapFormat fmt) {
        return fmt == eBitmapFormat_Float ? 4 : 1;}
    void setPixel(int x, int y, const vec4& c) {
        const int ofs = comp_ * (y * w_ + x);
        for (int k = 0; k < comp_; k++) {
            if (fmt_ == eBitmapFormat_Float) reinterpret_cast<float*>(data_.data())[ofs + k] = c[k];
            else data_[ofs + k] = uint8_t(c[k] * 255.0f);}}
    vec4 getPixel(int x, int y) const {
        const int ofs = comp_ * (y * w_ + x);
        vec4 c(0.0f);
        for (int k = 0; k < comp_ && k < 4; k++)
            c[k] = fmt_ == eBitmapFormat_Float ? reinterpret_cast<const float*>(data_.data())[ofs + k] : data_[ofs + k] / 255.0f;
        return c;}};

vec3 faceCoordsToXYZ(int i, int j, int faceID, int faceSize) {
    const float A = 2.0f * float(i) / faceSize;
    const float B = 2.0f * float(j) / faceSize;
    if (faceID == 0) return vec3(-1.0f, A - 1.0f, B - 1.0f);
    if (faceID == 1) return vec3(A - 1.0f, -1.0f, 1.0f - B);
    if (faceID == 2) return vec3(1.0f, A - 1.0f, 1.0f - B);
    if (faceID == 3) return vec3(1.0f - A, 1.0f, 1.0f - B);
    if (faceID == 4) return vec3(B - 1.0f, A - 1.0f, 1.0f);
    if (faceID == 5) return vec3(1.0f - B, A - 1.0f, -1.0f);
    return vec3();}

Bitmap convertEquirectangularMapToVerticalCross(const Bitmap& b) {
    if (b.type_ != eBitmapType_2D) return Bitmap();
    const int faceSize = b.w_ / 4;
    const int w = faceSize * 3;
    const int h = faceSize * 4;
    Bitmap result(w, h, 3, b.fmt_);
    const ivec2 kFaceOffsets[] = {
        ivec2(faceSize, faceSize * 3), ivec2(0, faceSize), ivec2(faceSize, faceSize),
        ivec2(faceSize * 2, faceSize), ivec2(faceSize, 0), ivec2(faceSize, faceSize * 2)};
    const int clampW = b.w_ - 1;
    const int clampH = b.h_ - 1;
    for (int face = 0; face != 6; face++) {
        for (int i = 0; i != faceSize; i++) {
            for (int j = 0; j != faceSize; j++) {
                const vec3 P = faceCoordsToXYZ(i, j, face, faceSize);
                const float R = hypot(P.x, P.y);
                const float theta = atan2(P.y, P.x);
                const float phi = atan2(P.z, R);
                const float Uf = float(2.0f * faceSize * (theta + M_PI) / M_PI);
                const float Vf = float(2.0f * faceSize * (M_PI / 2.0f - phi) / M_PI);
                const int U1 = clamp(int(floor(Uf)), 0, clampW);
                const int V1 = clamp(int(floor(Vf)), 0, clampH);
                const int U2 = clamp(U1 + 1, 0, clampW);
                const int V2 = clamp(V1 + 1, 0, clampH);
                const float s = Uf - U1;
                const float t = Vf - V1;
                const vec4 A = b.getPixel(U1, V1);
                const vec4 B = b.getPixel(U2, V1);
                const vec4 C = b.getPixel(U1, V2);
                const vec4 D = b.getPixel(U2, V2);
                const vec4 color = A * (1 - s) * (1 - t) + B * s * (1 - t) + C * (1 - s) * t + D * s * t;
                result.setPixel(i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);}}}
    return result;}

Bitmap convertVerticalCrossToCubeMapFaces(const Bitmap& b) {
    const int faceWidth = b.w_ / 3;
    const int faceHeight = b.h_ / 4;
    Bitmap cubemap(faceWidth, faceHeight, 6, b.comp_, b.fmt_);
    cubemap.type_ = eBitmapType_Cube;
    const uint8_t* src = b.data_.data();
    uint8_t* dst = cubemap.data_.data();
    const int pixelSize = cubemap.comp_ * Bitmap::getBytesPerComponent(cubemap.fmt_);
    for (int face = 0; face != 6; ++face) {
        for (int j = 0; j != faceHeight; ++j) {
            for (int i = 0; i != faceWidth; ++i) {
                int x = 0, y = 0;
                switch (face) {
                    case 0: x = i;y = faceHeight + j;break;
                    case 1: x = 2 * faceWidth + i;y = 1 * faceHeight + j;break;
                    case 2: x = 2 * faceWidth - (i + 1);y = 1 * faceHeight-(j + 1);break;
                    case 3: x = 2 * faceWidth - (i + 1);y = 3 * faceHeight - (j + 1);break;
                    case 4: x = 2 * faceWidth - (i + 1);y = b.h_ - (j + 1);break;
                    case 5: x = faceWidth + i;y = faceHeight + j;break;}
                memcpy(dst, src + (y * b.w_ + x) * pixelSize, pixelSize);
                dst += pixelSize;}}}
    return cubemap;}

GLuint loadCubemap(const char* hdrFile) {
    int w, h, comp;
    const float* img = stbi_loadf("/home/arc/dev/engine/data/texture/street.hdr", &w, &h, &comp, 3);
    if (!img) return 0;
    Bitmap in(w, h, 3, eBitmapFormat_Float, img);
    stbi_image_free((void*)img);
    Bitmap out = convertEquirectangularMapToVerticalCross(in);
    stbi_write_hdr("data/out/screen.hdr", out.w_, out.h_, out.comp_, reinterpret_cast<const float*>(out.data_.data()));
    Bitmap cm = convertVerticalCrossToCubeMapFaces(out);
    GLuint tex;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &tex);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(tex, 1, GL_RGB32F, cm.w_, cm.h_);
    const uint8_t* data = cm.data_.data();
    for (unsigned i = 0; i != 6; ++i) {
        glTextureSubImage3D(tex, 0, 0, 0, i, cm.w_, cm.h_, 1, GL_RGB, GL_FLOAT, data);
        data += cm.w_ * cm.h_ * cm.comp_ * Bitmap::getBytesPerComponent(cm.fmt_);}
    return tex;}
const char* kVertexShader = R"(
#version 460 core
layout(std140, binding = 0) uniform PerFrameData {
    mat4 model;
    mat4 MVP;
    vec4 cameraPos;};
struct PerVertex { vec2 uv; vec3 normal; vec3 worldPos; };
layout(location = 0) out PerVertex vtx;
layout(std430, binding = 1) restrict readonly buffer Vertices { float data[]; } in_Vertices;
vec3 getPosition(int i) { return vec3(in_Vertices.data[8*i+0], in_Vertices.data[8*i+1], in_Vertices.data[8*i+2]); }
vec2 getTexCoord(int i) { return vec2(in_Vertices.data[8*i+3], in_Vertices.data[8*i+4]); }
vec3 getNormal(int i)   { return vec3(in_Vertices.data[8*i+5], in_Vertices.data[8*i+6], in_Vertices.data[8*i+7]); }
void main() {
    vec3 pos = getPosition(gl_VertexID);
    gl_Position = MVP * vec4(pos, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vtx.uv = getTexCoord(gl_VertexID);
    vtx.normal = normalMatrix * getNormal(gl_VertexID);
    vtx.worldPos = (model * vec4(pos, 1.0)).xyz;})";

const char* kFragmentShader = R"(
#version 460 core
layout(std140, binding = 0) uniform PerFrameData {
    mat4 model;
    mat4 MVP;
    vec4 cameraPos;};
struct PerVertex { vec2 uv; vec3 normal; vec3 worldPos; };
layout(location = 0) in PerVertex vtx;
layout(location = 0) out vec4 out_FragColor;
layout(binding = 0) uniform sampler2D texture0;
layout(binding = 1) uniform samplerCube texture1;
void main() {
    vec3 n = normalize(vtx.normal);
    vec3 v = normalize(cameraPos.xyz - vtx.worldPos);
    vec3 reflection = -normalize(reflect(v, n));
    float eta = 1.00 / 1.31;
    vec3 refraction = -normalize(refract(v, n, eta));
    const float R0 = ((1.0 - eta) * (1.0 - eta)) / ((1.0 + eta) * (1.0 + eta));
    const float Rtheta = R0 + (1.0 - R0) * pow((1.0 - dot(-v, n)), 5.0);
    vec4 color = texture(texture0, vtx.uv);
    vec4 colorRefl = texture(texture1, reflection);
    vec4 colorRefr = texture(texture1, refraction);
    color = color * mix(colorRefl, colorRefr, Rtheta);
    out_FragColor = color;})";

