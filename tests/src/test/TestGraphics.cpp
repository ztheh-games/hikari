#include <catch.hpp>
#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/game/KeyboardInput.hpp"
#include "hikari/client/gui/GpuGraphics.hpp"
#include "hikari/client/gui/PlatformInput.hpp"
#include "hikari/client/gui/GpuImage.hpp"

TEST_CASE("Recorded sprites snapshot transforms and preserve draw order", "[graphics][recording]") {
    hikari::gfx::Texture texture;
    texture.resize({16,16});
    hikari::gfx::Sprite sprite(texture);
    sprite.setOrigin({2,3});
    sprite.setPosition({10,20});
    sprite.setScale({-1,1});
    hikari::gfx::RenderTexture target({256,240});
    target.draw(sprite);
    sprite.setPosition({30,40});
    target.draw(sprite);
    REQUIRE(target.getDraws().size() == 2);
    REQUIRE(target.getDraws()[0].vertices[0].x == Approx(12));
    REQUIRE(target.getDraws()[0].vertices[0].y == Approx(17));
    REQUIRE(target.getDraws()[1].vertices[0].x == Approx(32));
    REQUIRE(target.getDraws()[0].vertices[1].x == Approx(-4));
}

TEST_CASE("Recorded offscreen snapshots retain their command generation", "[graphics][recording]") {
    hikari::gfx::RenderTexture source({16,16}), destination({16,16});
    source.clear(hikari::gfx::Color::Red);
    source.display();
    destination.draw(hikari::gfx::Sprite(source.getTexture()));
    source.clear(hikari::gfx::Color::Blue);
    source.display();
    REQUIRE(destination.getDraws()[0].texture.pass->clear == hikari::gfx::Color::Red);
}

TEST_CASE("Image masking and pixel edits retain RGBA semantics", "[graphics][image]") {
    hikari::gfx::Image image;
    image.resize({2,1},hikari::gfx::Color::Magenta);
    image.setPixel({1,0},{10,20,30,40});
    image.createMaskFromColor(hikari::gfx::Color::Magenta);
    REQUIRE(image.getPixel({0,0}).a == 0);
    REQUIRE(image.getPixel({1,0}).a == 40);
    hikari::gfx::Texture texture;
    texture.update(image);
    texture.setPixel({1,0},{1,2,3,4});
    REQUIRE(texture.getPixel({1,0}) == hikari::gfx::Color(1,2,3,4));
    REQUIRE_THROWS_AS(texture.getPixel({2,0}),std::out_of_range);
}

TEST_CASE("Guichan clip intersections use offsets without changing the camera", "[graphics][gui]") {
    hikari::gfx::RenderTexture target({256,240});
    auto camera = target.getView(); camera.setCenter({100,100});
    target.setView(camera);
    gcn::GpuGraphics graphics(target);
    graphics._beginDraw();
    REQUIRE(graphics.pushClipArea({10,20,40,30}));
    REQUIRE(graphics.pushClipArea({30,20,40,30}));
    graphics.fillRectangle({0,0,40,30});
    REQUIRE(target.getDraws().size() == 1);
    const auto & draw = target.getDraws()[0];
    REQUIRE(draw.vertices[0].x == Approx(40));
    REQUIRE(draw.vertices[0].y == Approx(40));
    REQUIRE(draw.clip.position.x == 40);
    REQUIRE(draw.clip.position.y == 40);
    REQUIRE(draw.clip.size.x == 10);
    REQUIRE(draw.clip.size.y == 10);
    graphics.popClipArea();
    graphics.popClipArea();
    graphics._endDraw();
    REQUIRE(target.getView().getCenter().x == Approx(100));
    REQUIRE(target.getClip().size.x == 256);
}

TEST_CASE("Global keyboard preserves event-driven button edges and A alias", "[input][compatibility]") {
    hikari::KeyboardInput input;
    using Event = hikari::platform::Event;
    using Key = hikari::platform::Keyboard::Key;
    input.processEvent(Event(Event::KeyPressed{Key::A}));
    REQUIRE(input.isDown(hikari::Input::BUTTON_SHOOT));
    REQUIRE(input.wasPressed(hikari::Input::BUTTON_START));
    input.update(1.0f/60.0f);
    REQUIRE(input.isHeld(hikari::Input::BUTTON_SHOOT));
    REQUIRE_FALSE(input.wasPressed(hikari::Input::BUTTON_START));
    input.processEvent(Event(Event::KeyReleased{Key::A}));
    REQUIRE(input.wasReleased(hikari::Input::BUTTON_SHOOT));
    input.update(1.0f/60.0f);
    input.processEvent(Event(Event::KeyPressed{Key::S}));
    input.processEvent(Event(Event::KeyReleased{Key::S}));
    REQUIRE_FALSE(input.wasPressed(hikari::Input::BUTTON_JUMP));
}

TEST_CASE("Guichan keeps logical mouse coordinates and keyboard modifiers", "[input][gui]") {
    gcn::PlatformInput input;
    using Event = hikari::platform::Event;
    using Key = hikari::platform::Keyboard::Key;
    input.pushInput(Event(Event::KeyPressed{Key::Numpad3,true,false,true,false}));
    const auto key = input.dequeueKeyInput();
    REQUIRE(key.getKey().getValue() == static_cast<int>(Key::Numpad3));
    REQUIRE(key.isNumericPad());
    REQUIRE(key.isAltPressed());
    REQUIRE(key.isShiftPressed());
    input.pushInput(Event(Event::KeyPressed{Key::A}));
    REQUIRE(input.dequeueKeyInput().getKey().getValue() == 'A');
    input.pushInput(Event(Event::MouseMoved{{80,90}}));
    const auto mouse = input.dequeueMouseInput();
    REQUIRE(mouse.getX() == 80);
    REQUIRE(mouse.getY() == 90);
}

TEST_CASE("Guichan image retains cached pixels beyond the loader reference", "[graphics][gui]") {
    auto texture = std::make_shared<hikari::gfx::Texture>();
    texture->resize({2,2});
    gcn::GpuImage image(texture); texture.reset();
    image.putPixel(1,1,gcn::Color(10,20,30,40));
    REQUIRE(image.getWidth() == 2);
    REQUIRE(image.getPixel(1,1).a == 40);
    REQUIRE(image.getTexture()->getPixel({1,1}).r == 10);
    image.free();
    REQUIRE(image.getTexture() == nullptr);
}
