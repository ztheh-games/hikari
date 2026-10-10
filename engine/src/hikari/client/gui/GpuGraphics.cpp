#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/gui/GpuGraphics.hpp"

#include "guichan/exception.hpp"
#include "guichan/font.hpp"
#include "guichan/image.hpp"
#include "hikari/client/gui/GpuImage.hpp"

#include <cmath>

namespace gcn
{
    Color GpuGraphics::convertColorToGuichanColor(const hikari::gfx::Color& color)
    {
        return Color(color.r, color.g, color.b, color.a);
    }

    hikari::gfx::Color GpuGraphics::convertGuichanColorToColor(const Color& color)
    {
        return hikari::gfx::Color(color.r, color.g, color.b, color.a);
    }

    GpuGraphics::GpuGraphics(hikari::gfx::RenderTarget& target)
        : mTarget(&target)
    {
        mContextView = mTarget->getView();
        mSize = mContextView.getSize();
        mSavedClip = mTarget->getClip();
        setColor(Color());
    }

    void GpuGraphics::_beginDraw()
    {
        // Save the view before drawing.
        mContextView = mTarget->getView();
        mSize = mContextView.getSize();
        mSavedClip = mTarget->getClip();
        mTarget->setView(mTarget->getDefaultView());

        pushClipArea(Rectangle(0, 0, static_cast<int>(mSize.x), static_cast<int>(mSize.y)));
    }

    void GpuGraphics::_endDraw()
    {
        popClipArea();

        // Restore the view after drawing.
        mTarget->setView(mContextView);
        mTarget->setClip(mSavedClip);
    }

    void GpuGraphics::setRenderTarget(hikari::gfx::RenderTarget& target)
    {
        mTarget = &target;
        mContextView = mTarget->getView();
        mSize = mContextView.getSize();
        mSavedClip = mTarget->getClip();
    }

    bool GpuGraphics::pushClipArea(Rectangle area)
    {
        bool result = Graphics::pushClipArea(area);

        const auto & clip = mClipStack.top();
        mTarget->setClip({{clip.x,clip.y},{clip.width,clip.height}});

        return result;
    }

    void GpuGraphics::popClipArea()
    {
        Graphics::popClipArea();

        if (mClipStack.empty())
        {
            mTarget->setClip(mSavedClip);
            return;
        }

        const auto & clip = mClipStack.top();
        mTarget->setClip({{clip.x,clip.y},{clip.width,clip.height}});
    }

    hikari::gfx::RenderTarget& GpuGraphics::getRenderTarget() const {
        return *mTarget;
    }

    void GpuGraphics::drawImage(const Image* image,
                                int srcX,
                                int srcY,
                                int dstX,
                                int dstY,
                                int width,
                                int height)
    {
        const GpuImage* srcImage = dynamic_cast<const GpuImage*>(image);

        if (srcImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to draw an image of unknown format, must be an GpuImage.");
        }

        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw function outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        dstX += top.xOffset;
        dstY += top.yOffset;

        const hikari::gfx::IntRect srcRect({srcX, srcY}, {width, height});

        mSprite.setTexture(*srcImage->getTexture(), false);
        mSprite.setTextureRect(srcRect);
        mSprite.setPosition({static_cast<float>(dstX), static_cast<float>(dstY)});
        
        mTarget->draw(mSprite);
    }

    void GpuGraphics::drawPoint(int x, int y)
    {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw function outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        x += top.xOffset;
        y += top.yOffset;

        _drawFauxPixel(x, y);
    }

    void GpuGraphics::drawLine(int x1, int y1, int x2, int y2)
    {
        if (y1 == y2)
        {
            drawHorizontalLine(x1, y1, x2);
            return;
        }
        else if (x1 == x2)
        {
            drawVerticalLine(x1, y1, y2);
            return;
        }
        else
        {
            drawBresenham(x1, y1, x2, y2);
            return;
        }
    }

    void GpuGraphics::drawRectangle(const Rectangle& rectangle)
    {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw function outside of _beginDraw() and _endDraw()?");
        }

        int x1 = rectangle.x;
        int x2 = rectangle.x + rectangle.width - 1;
        int y1 = rectangle.y;
        int y2 = rectangle.y + rectangle.height - 1;

        drawHorizontalLine(x1, y1, x2);
        drawHorizontalLine(x1, y2, x2);

        drawVerticalLine(x1, y1, y2);
        drawVerticalLine(x2, y1, y2); // Fill in the "missing" pixel.
    }

    void GpuGraphics::fillRectangle(const Rectangle& rectangle)
    {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw function outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        const float x = static_cast<float>(rectangle.x + top.xOffset);
        const float y = static_cast<float>(rectangle.y + top.yOffset);
        const float w = static_cast<float>(rectangle.width);
        const float h = static_cast<float>(rectangle.height);

        hikari::gfx::Vertex rect[4] =
        {
            hikari::gfx::Vertex{{x, y}, mColorValue},
            hikari::gfx::Vertex{{x + w, y}, mColorValue},
            hikari::gfx::Vertex{{x + w, y + h}, mColorValue},
            hikari::gfx::Vertex{{x, y + h}, mColorValue}
        };

        mTarget->draw(rect, 4, hikari::gfx::PrimitiveType::TriangleFan);
    }

    void GpuGraphics::drawText(const std::string& text,
                                int x,
                                int y,
                                Alignment alignment)
    {
        if (mFont == NULL)
        {
            throw GCN_EXCEPTION("No font set in graphics.");
        }

        if (alignment == Graphics::Center)
        {
            const int textWidth = mFont->getWidth(text);

            x -= textWidth / 2;
        }
        else if (alignment == Graphics::Right)
        {
            const int textWidth = mFont->getWidth(text);

            x -= textWidth;
        }

        mFont->drawString(this, text, x, y);
    }

    void GpuGraphics::setColor(const Color& color)
    {
        mColor = color;
        mColorValue = convertGuichanColorToColor(color);
    }

    const Color& GpuGraphics::getColor() const
    {
        return mColor;
    }

    void GpuGraphics::setFont(Font* font)
    {
        mFont = font;
    }

    void GpuGraphics::_drawFauxPixel(int x, int y) {
        float px = static_cast<float>(x);
        float py = static_cast<float>(y);

        hikari::gfx::Vertex rect[4] =
        {
            hikari::gfx::Vertex{{px, py}, mColorValue},
            hikari::gfx::Vertex{{px + 1, py}, mColorValue},
            hikari::gfx::Vertex{{px + 1, py + 1}, mColorValue},
            hikari::gfx::Vertex{{px, py + 1}, mColorValue}
        };

        mTarget->draw(rect, 4, hikari::gfx::PrimitiveType::TriangleFan);
    }

    void GpuGraphics::drawHorizontalLine(int x1, int y, int x2) {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw function outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        x1 += top.xOffset;
        x2 += top.xOffset;
        y += top.yOffset;

        if (y < top.y || y >= top.y + top.height)
        {
            return;
        }

        if(x1 > x2) {
            std::swap(x1, x2);
        }

        if (top.x > x1)
        {
            if (top.x > x2)
            {
                return;
            }

            x1 = top.x;
        }

        if (top.x + top.width <= x2)
        {
            if (top.x + top.width <= x1)
            {
                return;
            }

            x2 = top.x + top.width - 1;
        }

        // Overdraw by 1 pixel; Widgets expect this behavior
        x2 += 1;

        float lineX1 = static_cast<float>(x1);
        float lineX2 = static_cast<float>(x2);
        float lineY = static_cast<float>(y);

        hikari::gfx::Vertex rect[4] =
        {
            hikari::gfx::Vertex{{lineX1, lineY}, mColorValue},
            hikari::gfx::Vertex{{lineX2, lineY}, mColorValue},
            hikari::gfx::Vertex{{lineX2, lineY + 1}, mColorValue},
            hikari::gfx::Vertex{{lineX1, lineY + 1}, mColorValue}
        };

        mTarget->draw(rect, 4, hikari::gfx::PrimitiveType::TriangleFan);
    }

    void GpuGraphics::drawVerticalLine(int x, int y1, int y2) {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw function outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        x += top.xOffset;
        y1 += top.yOffset;
        y2 += top.yOffset;

        if (x < top.x || x >= top.x + top.width)
        {
            return;
        }

        if(y1 > y2) {
            std::swap(y1, y2);
        }

        if (top.y > y1)
        {
            if (top.y > y2)
            {
                return;
            }

            y1 = top.y;
        }

        if (top.y + top.height <= y2)
        {
            if (top.y + top.height <= y1)
            {
                return;
            }

            y2 = top.y + top.height - 1;
        }

        // Overdraw by 1 pixel; Widgets expect this behavior
        y2 += 1;
        
        float lineX = static_cast<float>(x);
        float lineY1 = static_cast<float>(y1);
        float lineY2 = static_cast<float>(y2);

        hikari::gfx::Vertex rect[4] =
        {
            hikari::gfx::Vertex{{lineX, lineY1}, mColorValue},
            hikari::gfx::Vertex{{lineX, lineY2}, mColorValue},
            hikari::gfx::Vertex{{lineX + 1, lineY2}, mColorValue},
            hikari::gfx::Vertex{{lineX + 1, lineY1}, mColorValue}
        };

        mTarget->draw(rect, 4, hikari::gfx::PrimitiveType::TriangleFan);
    }

    void GpuGraphics::drawBresenham(int x1, int y1, int x2, int y2) {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw function outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        x1 += top.xOffset;
        x2 += top.xOffset;
        y1 += top.yOffset;
        y2 += top.yOffset;

        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);

        if (dx > dy)
        {
            if (x1 > x2)
            {
                std::swap(x1, x2);
                std::swap(y1, y2);
            }

            if (y1 < y2)
            {
                int y = y1;
                int p = 0;

                for (int x = x1; x <= x2; x++)
                {
                    if (top.isContaining(x, y))
                    {
                        _drawFauxPixel(x, y);
                    }

                    p += dy;

                    if (p * 2 >= dx)
                    {
                        y++;
                        p -= dx;
                    }
                }
            }
            else
            {
                int y = y1;
                int p = 0;

                for (int x = x1; x <= x2; x++)
                {
                    if (top.isContaining(x, y))
                    {
                        _drawFauxPixel(x, y);
                    }

                    p += dy;

                    if (p * 2 >= dx)
                    {
                        y--;
                        p -= dx;
                    }
                }
            }
        }
        else
        {
            if (y1 > y2)
            {
                std::swap(y1, y2);
                std::swap(x1, x2);
            }

            if (x1 < x2)
            {
                int x = x1;
                int p = 0;

                for (int y = y1; y <= y2; y++)
                {
                    if (top.isContaining(x, y))
                    {
                        _drawFauxPixel(x, y);
                    }

                    p += dx;

                    if (p * 2 >= dy)
                    {
                        x++;
                        p -= dy;
                    }
                }
            }
            else
            {
                int x = x1;
                int p = 0;

                for (int y = y1; y <= y2; y++)
                {
                    if (top.isContaining(x, y))
                    {
                        _drawFauxPixel(x, y);
                    }

                    p += dx;

                    if (p * 2 >= dy)
                    {
                        x--;
                        p -= dy;
                    }
                }
            }
        }
    }
}