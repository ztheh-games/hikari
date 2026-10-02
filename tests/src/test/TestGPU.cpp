#include <catch.hpp>
#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/platform/Events.hpp"
#include "hikari/core/util/FileSystemSession.hpp"
#include "hikari/core/util/PhysFS.hpp"
#include <cmath>
#include <SDL3/SDL.h>
#include "hikari/client/game/ScreenEffectsService.hpp"

namespace {
struct Fixture {
    hikari::FileSystemSession filesystem{"gpu-tests"};
    hikari::platform::Session session;
    hikari::gfx::Renderer renderer{nullptr,false};
    Fixture() { PhysFS::addToSearchPath(HIKARI_TEST_CONTENT_DIR); }
};
}

TEST_CASE("SDL GPU renders opaque and alpha geometry with correct orientation", "[gpu]") {
    Fixture fixture;
    hikari::gfx::RenderTexture target({16,16});
    target.clear(hikari::gfx::Color::Blue);
    hikari::gfx::RectangleShape rectangle({4,4});
    rectangle.setPosition({2,3});
    rectangle.setFillColor({255,0,0,128});
    target.draw(rectangle);
    target.display();
    const auto pixels = fixture.renderer.readback(target.getTexture());
    const auto offset = (3*16+2)*4;
    REQUIRE(std::abs(static_cast<int>(pixels[offset])-128) <= 1);
    REQUIRE(pixels[offset+1] == 0);
    REQUIRE(std::abs(static_cast<int>(pixels[offset+2])-127) <= 1);
    REQUIRE(pixels[offset+3] == 255);
    REQUIRE(pixels[0] == 0);
    REQUIRE(pixels[2] == 255);
}

TEST_CASE("SDL GPU palette rows and transparent pixels match the source contract", "[gpu][palette]") {
    Fixture fixture;
    hikari::gfx::Image indices;
    indices.resize({8,1});
    for (unsigned x = 0; x < 8; ++x) indices.setPixel({x,0},{static_cast<std::uint8_t>(x),0,0,255});
    hikari::gfx::Texture source;
    source.update(indices);
    hikari::gfx::Image colors;
    colors.resize({8,32});
    for (unsigned y = 0; y < 32; ++y)
        for (unsigned x = 0; x < 8; ++x) colors.setPixel({x,y},{static_cast<std::uint8_t>(x*30),static_cast<std::uint8_t>(y*7),90});
    hikari::gfx::Texture table; table.update(colors);
    hikari::gfx::Shader shader(hikari::gfx::Material::Palette);
    shader.setUniform("colorTableTexture",table);
    shader.setUniform("colorTableWidth",8.0f);
    shader.setUniform("colorTableHeight",32.0f);
    hikari::gfx::RenderTexture target({8,32});
    hikari::gfx::Sprite sprite(source);
    for (unsigned row = 0; row < 32; ++row) {
        shader.setUniform("paletteIndex",static_cast<float>(row));
        sprite.setPosition({0,static_cast<float>(row)});
        target.draw(sprite,&shader);
    }
    target.display();
    const auto pixels = fixture.renderer.readback(target.getTexture());
    for (unsigned y = 0; y < 32; ++y) for (unsigned x = 0; x < 8; ++x) {
        const auto offset = (y*8+x)*4;
        REQUIRE(pixels[offset] == x*30);
        REQUIRE(pixels[offset+1] == y*7);
        REQUIRE(pixels[offset+2] == 90);
    }
    indices.setPixel({0,0},{0,0,0,0}); source.update(indices);
    target.clear(hikari::gfx::Color::Blue);
    sprite.setPosition({0,0}); target.draw(sprite,&shader); target.display();
    const auto transparent = fixture.renderer.readback(target.getTexture());
    REQUIRE(transparent[0] == 0);
    REQUIRE(transparent[2] == 255);
}

TEST_CASE("SDL GPU screen fade preserves exact stepped boundaries", "[gpu][fade]") {
    Fixture fixture;
    hikari::gfx::Image image; image.resize({1,1},hikari::gfx::Color::White);
    hikari::gfx::Texture texture; texture.update(image);
    hikari::gfx::Sprite sprite(texture);
    hikari::gfx::Shader shader(hikari::gfx::Material::Fade);
    const float percentages[]{0,1,25,50,75};
    const int expected[]{255,95,47,31,15};
    for (int i = 0; i < 5; ++i) {
        shader.setUniform("fadePercent",percentages[i]);
        hikari::gfx::RenderTexture target({1,1});
        target.draw(sprite,&shader); target.display();
        const auto pixels = fixture.renderer.readback(target.getTexture());
        REQUIRE(std::abs(static_cast<int>(pixels[0])-expected[i]) <= 1);
        REQUIRE(pixels[3] == 255);
    }

}

TEST_CASE("SDL GPU retains distinct offscreen generations in adjacent draws", "[gpu][offscreen]") {
    Fixture fixture;
    hikari::gfx::RenderTexture source({1,1}), destination({2,1});
    source.clear(hikari::gfx::Color::Red); source.display();
    destination.draw(hikari::gfx::Sprite(source.getTexture()));
    source.clear(hikari::gfx::Color::Blue); source.display();
    hikari::gfx::Sprite second(source.getTexture()); second.setPosition({1,0});
    destination.draw(second); destination.display();
    const auto pixels = fixture.renderer.readback(destination.getTexture());
    REQUIRE(pixels[0] == 255);
    REQUIRE(pixels[2] == 0);
    REQUIRE(pixels[4] == 0);
    REQUIRE(pixels[6] == 255);
    hikari::gfx::Image data; data.resize({1,1},hikari::gfx::Color::Red);
    hikari::gfx::Texture texture; texture.update(data);
    destination.clear(hikari::gfx::Color::Black);
    destination.draw(hikari::gfx::Sprite(texture));
    texture.setPixel({0,0},hikari::gfx::Color::Green);
    hikari::gfx::Sprite updated(texture); updated.setPosition({1,0});
    destination.draw(updated); destination.display();
    const auto revisions = fixture.renderer.readback(destination.getTexture());
    REQUIRE(revisions[0] == 255);
    REQUIRE(revisions[1] == 0);
    REQUIRE(revisions[4] == 0);
    REQUIRE(revisions[5] == 255);
}

TEST_CASE("SDL GPU scissors intersect negative bounds and sprites flip", "[gpu][clip][transform]") {
    Fixture fixture;
    hikari::gfx::RenderTexture target({2,2});
    target.clear(hikari::gfx::Color::Blue);
    target.setClip({{-2,-1},{3,2}});
    hikari::gfx::RectangleShape shape({2,2}); shape.setFillColor(hikari::gfx::Color::Red);
    target.draw(shape); target.display();
    auto pixels = fixture.renderer.readback(target.getTexture());
    REQUIRE(pixels[0] == 255);
    REQUIRE(pixels[4] == 0);
    REQUIRE(pixels[8] == 0);
    hikari::gfx::Image image; image.resize({2,1});
    image.setPixel({0,0},hikari::gfx::Color::Red); image.setPixel({1,0},hikari::gfx::Color::Green);
    hikari::gfx::Texture texture; texture.update(image);
    hikari::gfx::Sprite sprite(texture); sprite.setScale({-1,1}); sprite.setPosition({2,0});
    target.clear(hikari::gfx::Color::Black); target.setClip({{0,0},{2,2}});
    target.draw(sprite); target.display();
    pixels = fixture.renderer.readback(target.getTexture());
    REQUIRE(pixels[0] == 0);
    REQUIRE(pixels[1] == 255);
    REQUIRE(pixels[4] == 255);
    REQUIRE(pixels[5] == 0);
}

TEST_CASE("SDL window events retain logical coordinates, modifiers, and repeat policy", "[gpu][platform]") {
    Fixture fixture;
    hikari::platform::Window window;
    window.create({512,480},"Hikari platform test",false,false);
    while (window.pollEvent()) {}
    const auto logical = window.toLogical({256,240});
    REQUIRE(logical.x == 128);
    REQUIRE(logical.y == 120);
    SDL_Event raw{};
    raw.type = SDL_EVENT_KEY_DOWN; raw.key.scancode = SDL_SCANCODE_A;
    raw.key.key = static_cast<SDL_Keycode>('A'); raw.key.mod = SDL_KMOD_SHIFT;
    REQUIRE(SDL_PushEvent(&raw));
    const auto event = window.pollEvent();
    REQUIRE(event.has_value());
    REQUIRE(event->is<hikari::platform::Event::KeyPressed>());
    const auto * key = event->getIf<hikari::platform::Event::KeyPressed>();
    REQUIRE(key->code == hikari::platform::Keyboard::Key::A);
    REQUIRE(key->shift);
    raw.key.repeat = true; REQUIRE(SDL_PushEvent(&raw));
    raw.type = SDL_EVENT_KEY_UP; raw.key.repeat = false; REQUIRE(SDL_PushEvent(&raw));
    std::optional<hikari::platform::Event> release;
    for (int step = 0; step < 10 && !release; ++step) release = window.pollEvent();
    REQUIRE(release.has_value());
    REQUIRE(release->is<hikari::platform::Event::KeyReleased>());
    hikari::gfx::RenderTexture target({256,240}); target.clear(hikari::gfx::Color::Blue); target.display();
    window.present(target.getTexture());
    REQUIRE_THROWS_AS(window.create({256,240},"duplicate",false,false),std::logic_error);
}

TEST_CASE("Production screen effects use a separate target with an unfaded HUD", "[gpu][effects]") {
    Fixture fixture;
    hikari::gfx::RenderTexture scene({2,1}); scene.clear(hikari::gfx::Color::White); scene.display();
    hikari::ScreenEffectsService effects(2,1);
    effects.fadeOut(1); effects.update(0.5f);
    auto & final = effects.apply(scene);
    REQUIRE(&final != &scene);
    hikari::gfx::RectangleShape hud({1,1}); hud.setFillColor(hikari::gfx::Color::Red);
    final.draw(hud); final.display();
    const auto pixels = fixture.renderer.readback(final.getTexture());
    REQUIRE(pixels[0] == 255);
    REQUIRE(pixels[1] == 0);
    REQUIRE(pixels[4] == 31);
    REQUIRE(pixels[5] == 31);
    effects.clearEffects();
    REQUIRE(&effects.apply(scene) == &scene);
}

TEST_CASE("SDL GPU reuses target allocations and pipelines after warmup", "[gpu][lifetime]") {
    Fixture fixture;
    hikari::gfx::RenderTexture target({2,2});
    hikari::gfx::RectangleShape rectangle({2,2});
    auto frame = [&] {
        target.clear(hikari::gfx::Color::Blue); target.draw(rectangle); target.display();
        return fixture.renderer.readback(target.getTexture());
    };
    frame();
    const auto warm = fixture.renderer.getStatistics();
    for (int i = 0; i < 30; ++i) REQUIRE(frame()[0] == 255);
    const auto final = fixture.renderer.getStatistics();
    REQUIRE(final.texturesCreated == warm.texturesCreated);
    REQUIRE(final.pipelinesCreated == warm.pipelinesCreated);
}

TEST_CASE("SDL GPU handles resize, minimized presentation, and restore", "[gpu][window]") {
    Fixture fixture;
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow("Hikari swapchain test",512,480,SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY),
        SDL_DestroyWindow);
    REQUIRE(window != nullptr);
    hikari::gfx::Renderer renderer(window.get(),false);
    hikari::gfx::RenderTexture scene({256,240}); scene.clear(hikari::gfx::Color::Blue); scene.display();
    renderer.present(scene.getTexture());
    REQUIRE(SDL_SetWindowSize(window.get(),768,720));
    REQUIRE(SDL_SyncWindow(window.get()));
    SDL_PumpEvents();
    renderer.present(scene.getTexture());
    REQUIRE(SDL_MinimizeWindow(window.get()));
    REQUIRE(SDL_SyncWindow(window.get()));
    SDL_PumpEvents();
    REQUIRE((SDL_GetWindowFlags(window.get()) & SDL_WINDOW_MINIMIZED) != 0);
    const auto skipped = renderer.getStatistics().skippedPresentations;
    renderer.present(scene.getTexture());
    REQUIRE(renderer.getStatistics().skippedPresentations == skipped + 1);
    REQUIRE(SDL_RestoreWindow(window.get()));
    REQUIRE(SDL_SyncWindow(window.get()));
    SDL_PumpEvents();
    REQUIRE((SDL_GetWindowFlags(window.get()) & SDL_WINDOW_MINIMIZED) == 0);
    renderer.present(scene.getTexture());
    REQUIRE(renderer.getStatistics().skippedPresentations == skipped + 1);
    REQUIRE(SDL_HideWindow(window.get()));
    REQUIRE(SDL_SyncWindow(window.get()));
    SDL_PumpEvents();
    REQUIRE((SDL_GetWindowFlags(window.get()) & SDL_WINDOW_HIDDEN) != 0);
    renderer.present(scene.getTexture());
    REQUIRE(renderer.getStatistics().skippedPresentations == skipped + 2);
    REQUIRE(SDL_ShowWindow(window.get()));
    REQUIRE(SDL_SyncWindow(window.get()));
    SDL_PumpEvents();
    renderer.present(scene.getTexture());
    REQUIRE(renderer.getStatistics().skippedPresentations == skipped + 2);
}

TEST_CASE("SDL fullscreen preserves desktop fallback and logical coordinates with vsync", "[gpu][window]") {
    Fixture fixture;
    hikari::platform::Window window;
    window.create({256,240},"Hikari fullscreen test",true,true);
    int count = 0;
    std::unique_ptr<SDL_Window*,decltype(&SDL_free)> windows(SDL_GetWindows(&count),SDL_free);
    REQUIRE(windows != nullptr);
    REQUIRE(count == 1);
    auto * native = windows.get()[0];
    REQUIRE(SDL_SyncWindow(native));
    SDL_PumpEvents();
    REQUIRE((SDL_GetWindowFlags(native) & SDL_WINDOW_FULLSCREEN) != 0);
    REQUIRE((SDL_GetWindowFlags(native) & SDL_WINDOW_HIGH_PIXEL_DENSITY) != 0);
    int width = 0,height = 0;
    REQUIRE(SDL_GetWindowSize(native,&width,&height));
    REQUIRE(width > 0);
    REQUIRE(height > 0);
    const auto middle = window.toLogical({width/2,height/2});
    REQUIRE(std::abs(middle.x-128) <= 1);
    REQUIRE(std::abs(middle.y-120) <= 1);
    hikari::gfx::RenderTexture scene({256,240}); scene.clear(hikari::gfx::Color::Blue); scene.display();
    window.present(scene.getTexture());
}

TEST_CASE("SDL GPU device creation failures are explicit and safely unwind", "[gpu][failure]") {
    Fixture fixture;
    struct DriverHint {
        std::string previous;
        bool existed;
        DriverHint() : previous(SDL_GetHint(SDL_HINT_GPU_DRIVER) ? SDL_GetHint(SDL_HINT_GPU_DRIVER) : ""),
                       existed(SDL_GetHint(SDL_HINT_GPU_DRIVER) != nullptr) {}
        ~DriverHint() {
            SDL_ResetHint(SDL_HINT_GPU_DRIVER);
            if (existed) SDL_SetHint(SDL_HINT_GPU_DRIVER,previous.c_str());
        }
    } restore;
    REQUIRE(SDL_SetHintWithPriority(SDL_HINT_GPU_DRIVER,"hikari-unavailable-backend",SDL_HINT_OVERRIDE));
    REQUIRE_THROWS_AS(hikari::gfx::Renderer(nullptr,false),std::runtime_error);
}
