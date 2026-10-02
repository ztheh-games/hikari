#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/game/objects/PalettedAnimatedSprite.hpp"

#include "hikari/core/util/FileSystem.hpp"




#include <iostream>
#include <stdexcept>

namespace hikari {

    std::unique_ptr<hikari::gfx::Shader> PalettedAnimatedSprite::pixelShader(nullptr);
    std::unique_ptr<hikari::gfx::Image> PalettedAnimatedSprite::colorTableImage(nullptr);
    std::unique_ptr<hikari::gfx::Texture> PalettedAnimatedSprite::colorTableTexture(nullptr);

    const unsigned int PalettedAnimatedSprite::colorTableWidth = 8;
    const unsigned int PalettedAnimatedSprite::colorTableHeight = 32;

    int PalettedAnimatedSprite::sharedPaletteIndex = 0;
    std::vector<std::vector<hikari::gfx::Color>> PalettedAnimatedSprite::colorTable = std::vector<std::vector<hikari::gfx::Color>>();

    void PalettedAnimatedSprite::initializePaletteShader() {
        pixelShader = std::make_unique<gfx::Shader>(gfx::Material::Palette);
    }

    void PalettedAnimatedSprite::createColorTable(const std::vector<std::vector<hikari::gfx::Color>> & colors) {
        PalettedAnimatedSprite::colorTable = colors;

        if(!colorTableImage) {
            colorTableImage.reset(new hikari::gfx::Image());
        }

        colorTableImage->resize({colorTableWidth, colorTableHeight}, hikari::gfx::Color(0, 0, 255, 255));

        for(unsigned int row = 0; row < colors.size(); ++row) {
            const auto & paletteRow = colors[row];
            for(unsigned int column = 0; column < paletteRow.size(); ++column) {
                const auto color = paletteRow[column];

                colorTableImage->setPixel({column, row}, color);
            }
        }

        if(!colorTableTexture) {
            colorTableTexture.reset(new hikari::gfx::Texture());
        }

        colorTableTexture->resize({colorTableWidth, colorTableHeight});
        colorTableTexture->update(*colorTableImage);
        pixelShader->setUniform("colorTableTexture", *colorTableTexture);
        pixelShader->setUniform("colorTableWidth", static_cast<float>(colorTableWidth));
        pixelShader->setUniform("colorTableHeight", static_cast<float>(colorTableHeight));
    }

    void PalettedAnimatedSprite::destroySharedResources() {
        colorTableTexture.reset();
        pixelShader.reset();
        colorTableImage.reset();
    }

    void PalettedAnimatedSprite::setSharedPaletteIndex(int index) {
        sharedPaletteIndex = index;
    }

    int PalettedAnimatedSprite::getSharedPaletteIndex() {
        return sharedPaletteIndex;
    }

    const std::vector<std::vector<hikari::gfx::Color>> & PalettedAnimatedSprite::getColorTable() {
        return PalettedAnimatedSprite::colorTable;
    }

    PalettedAnimatedSprite::PalettedAnimatedSprite()
        : AnimatedSprite()
        , paletteIndex(0)
        , usePalette(false)
        , useSharedPalette(false)
    {

    }

    PalettedAnimatedSprite::PalettedAnimatedSprite(const PalettedAnimatedSprite & proto)
        : AnimatedSprite(proto)
        , paletteIndex(proto.paletteIndex)
        , usePalette(proto.usePalette)
        , useSharedPalette(proto.useSharedPalette)
    {

    }

    PalettedAnimatedSprite::~PalettedAnimatedSprite() {

    }

    void PalettedAnimatedSprite::update(float dt) {
        AnimatedSprite::update(dt);
    }

    void PalettedAnimatedSprite::render(hikari::gfx::RenderTarget &target) const {
        if(isUsingPalette()) {
            if(!pixelShader) throw std::logic_error("Palette material was not initialized");
            pixelShader->setUniform("paletteIndex",
                static_cast<float>(isUsingSharedPalette() ? sharedPaletteIndex : paletteIndex));
            target.draw(sprite, pixelShader.get());
        } else {
            AnimatedSprite::render(target);
        }
    }

    int PalettedAnimatedSprite::getPaletteIndex() const {
        return paletteIndex;
    }

    void PalettedAnimatedSprite::setPaletteIndex(int index) {
        paletteIndex = index;
    }

    bool PalettedAnimatedSprite::isUsingSharedPalette() const {
        return useSharedPalette;
    }

    void PalettedAnimatedSprite::setUseSharedPalette(bool flag) {
        useSharedPalette = flag;
    }

    bool PalettedAnimatedSprite::isUsingPalette() const {
        return usePalette;
    }

    void PalettedAnimatedSprite::setUsePalette(bool flag) {
        usePalette = flag;
    }

} // hikari
