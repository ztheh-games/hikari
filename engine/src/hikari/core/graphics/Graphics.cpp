#include "hikari/core/graphics/Graphics.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace hikari::gfx {

const Color Color::White{255,255,255}, Color::Black{0,0,0}, Color::Blue{0,0,255},
    Color::Magenta{255,0,255}, Color::Transparent{0,0,0,0}, Color::Red{255,0,0},
    Color::Green{0,255,0}, Color::Yellow{255,255,0};

namespace {
std::size_t offset(Vector2u size, Vector2u position) {
    if (position.x >= size.x || position.y >= size.y)
        throw std::out_of_range("Image pixel outside bounds");
    return (static_cast<std::size_t>(position.y) * size.x + position.x) * 4;
}
}

bool Image::loadFromMemory(const void * bytes, std::size_t length) {
    SDL_IOStream * stream = SDL_IOFromConstMem(bytes, length);
    if (!stream) throw std::runtime_error(std::string("Image input: ") + SDL_GetError());
    auto * decoded = IMG_Load_IO(stream, true);
    if (!decoded) throw std::runtime_error(std::string("Image decode: ") + SDL_GetError());
    auto * converted = SDL_ConvertSurface(decoded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(decoded);
    if (!converted) throw std::runtime_error(std::string("Image conversion: ") + SDL_GetError());
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(converted, SDL_DestroySurface);
    if (!SDL_LockSurface(surface.get())) throw std::runtime_error(SDL_GetError());
    resize({static_cast<unsigned>(surface->w), static_cast<unsigned>(surface->h)});
    for (unsigned y = 0; y < data.size.y; ++y)
        std::memcpy(data.pixels.data() + static_cast<std::size_t>(y) * data.size.x * 4,
                    static_cast<const std::uint8_t *>(surface->pixels) + static_cast<std::size_t>(y) * surface->pitch,
                    static_cast<std::size_t>(data.size.x) * 4);
    SDL_UnlockSurface(surface.get());
    return true;
}
void Image::resize(Vector2u size, Color color) {
    if (!size.x || !size.y || static_cast<std::size_t>(size.x) > std::numeric_limits<std::size_t>::max() / size.y / 4)
        throw std::invalid_argument("Invalid image dimensions");
    data.size = size;
    data.pixels.resize(static_cast<std::size_t>(size.x) * size.y * 4);
    for (std::size_t i = 0; i < data.pixels.size(); i += 4) {
        data.pixels[i] = color.r; data.pixels[i+1] = color.g;
        data.pixels[i+2] = color.b; data.pixels[i+3] = color.a;
    }
}
void Image::createMaskFromColor(Color color) {
    for (std::size_t i = 0; i < data.pixels.size(); i += 4)
        if (data.pixels[i] == color.r && data.pixels[i+1] == color.g && data.pixels[i+2] == color.b && data.pixels[i+3] == color.a)
            data.pixels[i+3] = 0;
}
Color Image::getPixel(Vector2u position) const {
    const auto i = offset(data.size, position);
    return {data.pixels[i], data.pixels[i+1], data.pixels[i+2], data.pixels[i+3]};
}
void Image::setPixel(Vector2u position, Color color) {
    const auto i = offset(data.size, position);
    data.pixels[i] = color.r; data.pixels[i+1] = color.g;
    data.pixels[i+2] = color.b; data.pixels[i+3] = color.a;
}
Texture::Texture() { snapshot.image = std::make_shared<ImageData>(); }
bool Texture::resize(Vector2u size) {
    Image image; image.resize(size);
    update(image);
    return true;
}
void Texture::update(const Image & image) {
    const auto revision = snapshot.image->revision + 1;
    const auto smooth = snapshot.image->smooth;
    snapshot.image = std::make_shared<ImageData>(image.getData());
    snapshot.pass.reset();
    snapshot.image->revision = revision;
    snapshot.image->smooth = smooth;
}
void Texture::setSmooth(bool enabled) {
    if (!snapshot.image.unique()) snapshot.image = std::make_shared<ImageData>(*snapshot.image);
    snapshot.image->smooth = enabled;
}
void Texture::setRepeated(bool enabled) {
    if (enabled) throw std::invalid_argument("Repeated textures are not supported by Hikari");
}
Image Texture::copyToImage() const {
    if (snapshot.pass) throw std::logic_error("Render targets require explicit fenced GPU readback");
    Image image; image.resize(getSize());
    for (unsigned y = 0; y < getSize().y; ++y)
        for (unsigned x = 0; x < getSize().x; ++x) image.setPixel({x,y}, getPixel({x,y}));
    return image;
}
Color Texture::getPixel(Vector2u position) const {
    const auto i = offset(getSize(), position);
    if (snapshot.pass) throw std::logic_error("Cannot read a render target as a CPU image");
    const auto & p = snapshot.image->pixels;
    return {p[i], p[i+1], p[i+2], p[i+3]};
}
void Texture::setPixel(Vector2u position, Color color) {
    const auto i = offset(getSize(), position);
    if (snapshot.pass) throw std::logic_error("Cannot edit a render target as a CPU image");
    if (!snapshot.image.unique()) snapshot.image = std::make_shared<ImageData>(*snapshot.image);
    auto & p = snapshot.image->pixels;
    p[i] = color.r; p[i+1] = color.g; p[i+2] = color.b; p[i+3] = color.a;
    ++snapshot.image->revision;
}
Vector2f Transformable::transform(Vector2f point) const {
    point.x = (point.x - origin.x) * scale.x; point.y = (point.y - origin.y) * scale.y;
    const float angle = rotation * 0.017453292519943295f;
    const float c = std::cos(angle), s = std::sin(angle);
    return {position.x + c * point.x - s * point.y, position.y + s * point.x + c * point.y};
}
void Sprite::setTexture(const Texture & value, bool resetRectangle) {
    if (!texture || resetRectangle) rectangle = {{0,0}, {static_cast<int>(value.getSize().x), static_cast<int>(value.getSize().y)}};
    texture = &value;
}
FloatRect Sprite::getLocalBounds() const { return {{0,0}, {std::abs(static_cast<float>(rectangle.size.x)), std::abs(static_cast<float>(rectangle.size.y))}}; }
void Shader::setUniform(const std::string & name, float value) {
    if (name == "paletteIndex" || name == "fadePercent") parameters[0] = value;
    else if (name == "colorTableWidth") parameters[1] = value;
    else if (name == "colorTableHeight") parameters[2] = value;
    else throw std::invalid_argument("Unknown shader parameter: " + name);
}
void Shader::setUniform(const std::string & name, const Texture & value) {
    if (name != "colorTableTexture") throw std::invalid_argument("Unknown shader texture: " + name);
    palette = value.capture();
}
RenderTarget::RenderTarget(Vector2u size) : size(size),
    view(FloatRect{{0,0}, {static_cast<float>(size.x), static_cast<float>(size.y)}}), defaultView(view),
    clip({0,0}, {static_cast<int>(size.x), static_cast<int>(size.y)}) {}
void RenderTarget::clear(Color color) { clearColor = color; draws.clear(); }
Vector2f RenderTarget::project(Vector2f p) const {
    const auto s = view.getSize(), c = view.getCenter();
    const auto v = view.getViewport();
    return {(p.x - c.x + s.x/2) / s.x * v.size.x * size.x + v.position.x * size.x,
            (p.y - c.y + s.y/2) / s.y * v.size.y * size.y + v.position.y * size.y};
}
Vector2f RenderTarget::mapPixelToCoords(Vector2i p) const {
    const auto s = view.getSize(), c = view.getCenter();
    const auto v = view.getViewport();
    return {(p.x - v.position.x * size.x) / (v.size.x * size.x) * s.x + c.x - s.x/2,
            (p.y - v.position.y * size.y) / (v.size.y * size.y) * s.y + c.y - s.y/2};
}
void RenderTarget::quad(const Transformable & transform, Vector2f p, Vector2f e, Color color,
                       TextureSnapshot texture, IntRect source, const Shader * shader) {
    if (clip.size.x <= 0 || clip.size.y <= 0) return;
    const Vector2f points[4]{{p.x,p.y}, {p.x+e.x,p.y}, {p.x+e.x,p.y+e.y}, {p.x,p.y+e.y}};
    const float w = texture.image ? static_cast<float>(texture.image->size.x) : 1;
    const float h = texture.image ? static_cast<float>(texture.image->size.y) : 1;
    if (w <= 0 || h <= 0) throw std::logic_error("Drawing an unloaded texture");
    const float u[4]{source.position.x/w, (source.position.x+source.size.x)/w, (source.position.x+source.size.x)/w, source.position.x/w};
    const float v[4]{source.position.y/h, source.position.y/h, (source.position.y+source.size.y)/h, (source.position.y+source.size.y)/h};
    Draw draw;
    draw.texture = std::move(texture); draw.clip = clip;
    if (shader) { draw.material = shader->material; draw.palette = shader->palette; draw.parameters = shader->parameters; }
    for (int i = 0; i < 4; ++i) {
        const auto point = project(transform.transform(points[i]));
        draw.vertices[i] = {point.x,point.y,u[i],v[i],color.r/255.0f,color.g/255.0f,color.b/255.0f,color.a/255.0f};
    }
    draws.push_back(std::move(draw));
}
void RenderTarget::draw(const Sprite & sprite, const Shader * shader) {
    if (!sprite.getTexture()) throw std::logic_error("Drawing a sprite without a texture");
    quad(sprite, {}, sprite.getLocalBounds().size, sprite.getColor(), sprite.getTexture()->capture(), sprite.getTextureRect(), shader);
}
void RenderTarget::draw(const RectangleShape & shape) {
    const auto s = shape.getSize();
    quad(shape, {}, s, shape.getFillColor(), {}, {}, nullptr);
    const auto t = shape.getOutlineThickness();
    if (t != 0) {
        quad(shape, {-t,-t}, {s.x+2*t,t}, shape.getOutlineColor(), {}, {}, nullptr);
        quad(shape, {-t,s.y}, {s.x+2*t,t}, shape.getOutlineColor(), {}, {}, nullptr);
        quad(shape, {-t,0}, {t,s.y}, shape.getOutlineColor(), {}, {}, nullptr);
        quad(shape, {s.x,0}, {t,s.y}, shape.getOutlineColor(), {}, {}, nullptr);
    }
}
void RenderTarget::draw(const Vertex * vertices, std::size_t count, PrimitiveType) {
    if (count != 4) throw std::invalid_argument("Hikari quad drawing requires four vertices");
    if (clip.size.x <= 0 || clip.size.y <= 0) return;
    Draw draw; draw.clip = clip;
    for (int i = 0; i < 4; ++i) {
        const auto point = project(vertices[i].position);
        const auto c = vertices[i].color;
        draw.vertices[i] = {point.x,point.y,0,0,c.r/255.0f,c.g/255.0f,c.b/255.0f,c.a/255.0f};
    }
    draws.push_back(std::move(draw));
}
RenderTexture::RenderTexture(Vector2u size) : RenderTarget(size) { resize(size); }
bool RenderTexture::resize(Vector2u value) {
    size = value;
    defaultView = View(FloatRect{{0,0}, {static_cast<float>(size.x),static_cast<float>(size.y)}});
    view = defaultView; clip = {{0,0},{static_cast<int>(size.x),static_cast<int>(size.y)}};
    texture.resize(value); texture.snapshot.pass.reset(); draws.clear();
    return true;
}
void RenderTexture::display() {
    texture.snapshot.pass = std::make_shared<RecordedPass>(RecordedPass{texture.snapshot.image, clearColor, draws});
}
}
