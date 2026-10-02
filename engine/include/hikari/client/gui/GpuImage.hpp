#ifndef HIKARI_GPUIMAGE_HPP
#define HIKARI_GPUIMAGE_HPP

#include "hikari/core/graphics/Graphics.hpp"

#include "guichan/color.hpp"
#include "guichan/platform.hpp"
#include "guichan/image.hpp"

namespace gcn
{
    /**
     * Cached engine texture implementation of Image.
     */
    class GCN_EXTENSION_DECLSPEC GpuImage : public Image
    {
    public:
        /**
         * Retains shared ownership of a cached texture.
         *
         * @param texture the texture to use.
         */
        explicit GpuImage(std::shared_ptr<hikari::gfx::Texture> texture);

        /**
         * Destructor.
         */
        virtual ~GpuImage();

        /**
         * Gets the engine texture for the image.
         *
         * @return the engine texture for the image.
         */
        virtual hikari::gfx::Texture* getTexture() const;


        // Inherited from Image

        virtual void free();

        virtual int getWidth() const;

        virtual int getHeight() const;

        virtual Color getPixel(int x, int y);

        virtual void putPixel(int x, int y, const Color& color);

        virtual void convertToDisplayFormat();

    protected:
        std::shared_ptr<hikari::gfx::Texture> mTexture;
    };
}

#endif // end HIKARI_GPUIMAGE_HPP