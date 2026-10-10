#ifndef HIKARI_CORE_GRAPHICS
#define HIKARI_CORE_GRAPHICS

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hikari::gfx {

template<class T> struct Vector2 {
    T x{}, y{};
    Vector2() = default;
    Vector2(T x, T y) : x(x), y(y) {}
};
using Vector2f = Vector2<float>;
using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned int>;
template<class T> struct Rect {
    Vector2<T> position, size;
    Rect() = default;
    Rect(Vector2<T> position, Vector2<T> size) : position(position), size(size) {}
};
using IntRect = Rect<int>;
using FloatRect = Rect<float>;

struct Color {
    std::uint8_t r{}, g{}, b{}, a{255};
    constexpr Color() = default;
    constexpr Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}
    bool operator==(const Color & other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    static const Color White, Black, Blue, Magenta, Transparent, Red, Green, Yellow;
};

struct ImageData {
    Vector2u size;
    std::vector<std::uint8_t> pixels;
    bool smooth = false;
    std::uint64_t revision = 0;
};

class Image {
    ImageData data;
public:
    bool loadFromMemory(const void * bytes, std::size_t length);
    void resize(Vector2u size, Color color = Color::Black);
    void createMaskFromColor(Color color);
    Color getPixel(Vector2u position) const;
    void setPixel(Vector2u position, Color color);
    Vector2u getSize() const { return data.size; }
    const ImageData & getData() const { return data; }
};

struct RecordedPass;
struct TextureSnapshot {
    std::shared_ptr<ImageData> image;
    std::shared_ptr<const RecordedPass> pass;
};

class Texture {
    TextureSnapshot snapshot;
    friend class RenderTexture;
    friend class Renderer;
public:
    Texture();
    bool resize(Vector2u size);
    void update(const Image & image);
    void setSmooth(bool enabled);
    void setRepeated(bool enabled);
    Vector2u getSize() const { return snapshot.image->size; }
    Image copyToImage() const;
    void setPixel(Vector2u position, Color color);
    Color getPixel(Vector2u position) const;
    TextureSnapshot capture() const { return snapshot; }
};

struct Angle { float value; };
inline Angle degrees(float value) { return {value}; }

class Transformable {
    Vector2f position{}, origin{}, scale{1, 1};
    float rotation = 0;
public:
    void setPosition(Vector2f value) { position = value; }
    Vector2f getPosition() const { return position; }
    void move(Vector2f delta) { position.x += delta.x; position.y += delta.y; }
    void setOrigin(Vector2f value) { origin = value; }
    Vector2f getOrigin() const { return origin; }
    void setScale(Vector2f value) { scale = value; }
    Vector2f getScale() const { return scale; }
    void setRotation(Angle value) { rotation = value.value; }
    Vector2f transform(Vector2f point) const;
};

class Sprite : public Transformable {
    const Texture * texture = nullptr;
    IntRect rectangle;
    Color color = Color::White;
public:
    Sprite() = default;
    explicit Sprite(const Texture & texture) { setTexture(texture, true); }
    void setTexture(const Texture & value, bool resetRectangle = false);
    const Texture * getTexture() const { return texture; }
    void setTextureRect(IntRect value) { rectangle = value; }
    IntRect getTextureRect() const { return rectangle; }
    FloatRect getLocalBounds() const;
    void setColor(Color value) { color = value; }
    Color getColor() const { return color; }
};

class RectangleShape : public Transformable {
    Vector2f size;
    Color fill = Color::White, outline = Color::White;
    float thickness = 0;
public:
    explicit RectangleShape(Vector2f size = {}) : size(size) {}
    void setSize(Vector2f value) { size = value; }
    Vector2f getSize() const { return size; }
    void setFillColor(Color value) { fill = value; }
    Color getFillColor() const { return fill; }
    void setOutlineColor(Color value) { outline = value; }
    Color getOutlineColor() const { return outline; }
    void setOutlineThickness(float value) { thickness = value; }
    float getOutlineThickness() const { return thickness; }
};

class View {
    Vector2f center{}, size{};
    FloatRect viewport{{0, 0}, {1, 1}};
public:
    View() = default;
    explicit View(FloatRect rect) : center{rect.position.x + rect.size.x / 2, rect.position.y + rect.size.y / 2}, size(rect.size) {}
    View(Vector2f center, Vector2f size) : center(center), size(size) {}
    void setCenter(Vector2f value) { center = value; }
    Vector2f getCenter() const { return center; }
    void setSize(Vector2f value) { size = value; }
    Vector2f getSize() const { return size; }
    void setViewport(FloatRect value) { viewport = value; }
    FloatRect getViewport() const { return viewport; }
};

enum class Material { Textured, Palette, Fade };
class Shader {
    Material material;
    TextureSnapshot palette;
    std::array<float, 4> parameters{};
    friend class RenderTarget;
public:
    explicit Shader(Material material) : material(material) {}
    void setUniform(const std::string & name, float value);
    void setUniform(const std::string & name, const Texture & value);
};

struct Vertex { Vector2f position; Color color = Color::White; };
enum class PrimitiveType { TriangleFan };
struct GPUVertex { float x, y, u, v, r, g, b, a; };
struct Draw {
    std::array<GPUVertex, 4> vertices;
    TextureSnapshot texture, palette;
    Material material = Material::Textured;
    std::array<float, 4> parameters{};
    IntRect clip;
};
struct RecordedPass {
    std::shared_ptr<ImageData> output;
    Color clear;
    std::vector<Draw> draws;
};

class RenderTarget {
protected:
    Vector2u size;
    View view, defaultView;
    IntRect clip;
    Color clearColor = Color::Black;
    std::vector<Draw> draws;
    void quad(const Transformable & transform, Vector2f position, Vector2f extent,
              Color color, TextureSnapshot texture, IntRect source, const Shader * shader);
    Vector2f project(Vector2f point) const;
public:
    explicit RenderTarget(Vector2u size = {256, 240});
    virtual ~RenderTarget() = default;
    Vector2u getSize() const { return size; }
    const View & getView() const { return view; }
    const View & getDefaultView() const { return defaultView; }
    void setView(const View & value) { view = value; }
    void setClip(IntRect value) { clip = value; }
    IntRect getClip() const { return clip; }
    void clear(Color color);
    void draw(const Sprite & sprite, const Shader * shader = nullptr);
    void draw(const RectangleShape & shape);
    void draw(const Vertex * vertices, std::size_t count, PrimitiveType type);
    Vector2f mapPixelToCoords(Vector2i point) const;
    const std::vector<Draw> & getDraws() const { return draws; }
};

class RenderTexture : public RenderTarget {
    Texture texture;
public:
    explicit RenderTexture(Vector2u size = {256, 240});
    bool resize(Vector2u size);
    void display();
    const Texture & getTexture() const { return texture; }
};

class Renderer {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    struct Statistics { std::size_t texturesCreated, pipelinesCreated, skippedPresentations; };
    explicit Renderer(void * window, bool vsync);
    ~Renderer();
    Renderer(const Renderer &) = delete;
    Renderer & operator=(const Renderer &) = delete;
    void present(const Texture & image);
    std::vector<std::uint8_t> readback(const Texture & image);
    Statistics getStatistics() const;
};

}
#endif
