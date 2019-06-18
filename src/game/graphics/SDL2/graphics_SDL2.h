#pragma once

#include "SDL_video.h"
#include "SDL_render.h"
#include "SDL_image.h"
#include "SDL_filesystem.h"
#include "SDL_ttf.h"
#include <vector>
#include <memory>
#include <string>

class Color : public SDL_Color
{
public:
    Color(unsigned rgba = 0xffffffff);
    Color(unsigned r, unsigned g, unsigned b, unsigned a);
    uint32_t hex() const;
    Color operator+ (const Color& rhs) const;
    Color operator* (const double& rhs) const;
    bool operator== (const Color& rhs) const;
    bool operator!= (const Color& rhs) const;
};

////////////////////////////////////////////////////////////////////////////////
// Enums

enum class TTFStyle
{
    Normal,
    Bold,
    Italic,
    _BI,
};

enum class TTFHinting
{
    Normal,
    Light,
    Mono,
    None,
};

// Used by SDL_RenderCopy().
// Other blend modes should use SDL_ComposeCustomBlendMode(6).
enum class BlendMode
{
    NONE        = SDL_BLENDMODE_NONE,
    ALPHA       = SDL_BLENDMODE_BLEND,
    ADD         = SDL_BLENDMODE_ADD,
    MULTIPLY    = SDL_BLENDMODE_MOD,
    // LR2 specific is decribed below. Refer to old element.cpp
    // SUBTRACT,
    // ANTICOLOR,
    // MULTIPLY_ANTI_BACKGROUND,
    // MULTIPLY_WITH_ALPHA,
    // XOR,
};

enum class BlendFactor
{
    ZERO                = SDL_BLENDFACTOR_ZERO,
    ONE                 = SDL_BLENDFACTOR_ONE,
    COLOR               = SDL_BLENDFACTOR_SRC_COLOR,
    ONE_MINUS_SRC       = SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
    SRC_ALPHA           = SDL_BLENDFACTOR_SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
    DST_COLOR           = SDL_BLENDFACTOR_DST_COLOR,
    ONE_MINUS_DST_COLOR = SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR,
    DST_ALPHA           = SDL_BLENDFACTOR_DST_ALPHA,
    ONE_MINUS_DST_ALPHA = SDL_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
};

////////////////////////////////////////////////////////////////////////////////

// Point: x, y
class Point
{
public:
	double x = 0;
	double y = 0;
public:
	constexpr Point(int zero = 0) {}
	constexpr Point(double x, double y) : x(x), y(y) {}
	constexpr Point operator+ (const Point& rhs) const { return Point(x + rhs.x, y + rhs.y); }
	constexpr Point operator- (const Point& rhs) const { return Point(x - rhs.x, y - rhs.y); }
	constexpr Point operator* (const double& rhs) const { return Point(x * rhs, y * rhs); }
	constexpr bool  operator== (const Point& rhs) const { return x == rhs.x && y == rhs.y; }
};

class Image;

// Rect: x, y, w, h
class Rect: public SDL_Rect
{
public:
    Rect(int zero = 0);
    Rect(int w, int h);
    Rect(int x, int y, int w, int h);
    Rect(const SDL_Rect& rect);
    ~Rect() = default;
public:
    Rect operator+ (const Rect& rhs) const;
    Rect operator* (const double& rhs) const;
    bool operator== (const Rect& rhs) const;
    bool operator!= (const Rect& rhs) const;
    Rect standardize(const Image& image) const;
    Rect standardize(const Rect& rect) const;
};


////////////////////////////////////////////////////////////////////////////////
// SDL_Image loads pictures into SDL_Surface instances
// Run IMG_Init outside.
class Image
{
    friend class Texture;

private:
    std::string _path;
    std::shared_ptr<SDL_RWops> _pRWop;
    std::shared_ptr<SDL_Surface> _pSurface;
    bool _loaded = false;
    bool _haveAlphaLayer = false;
public:
    Image(const char* filePath);
    ~Image();
public:
    Rect getRect() const;
};


////////////////////////////////////////////////////////////////////////////////
// Convert SDL_Surface into SDL_Texture with subarea specified.
class Texture
{
	friend class vSprite;
	friend class SpriteStatic;
	friend class SpriteSelection;
	friend class SpriteAnimated;
	friend class SpriteText;
	friend class SpriteNumber;

	friend class SpriteLaneVertical;

protected:
	SDL_Texture* _pTexture = nullptr;
	bool _loaded = false;
	Rect _texRect;

public:
	// Inner draw function.
	virtual void draw(const Rect& srcRect, Rect dstRect,
		const Color c, const BlendMode blend, const bool filter, const double angleInDegrees) const;
	// TODO params of draw: center, flip

public:
	Texture(const Image& srcImage);
	Texture(const SDL_Surface* pSurface);
	Texture(const SDL_Texture* pTexture, int w, int h);
	virtual ~Texture();
public:
	Rect getRect() const { return _texRect; }
	bool isLoaded() const { return _loaded; }
};


// Special texture class that always uses full texture size as output rect.
// That is, srcRect is ignored and replaced with _texRect.
// Useful when rendering BGs and Error-texture.
class TextureFull: public Texture
{
private:
    virtual void draw(const Rect& srcRect, const Rect& dstRect, 
        const Color c, const double angleInDegrees) const;
public:
    TextureFull(const Color& srcColor);
    TextureFull(const Image& srcImage);
    TextureFull(const SDL_Surface* pSurface);
    TextureFull(const SDL_Texture* pTexture, int w, int h);
    virtual ~TextureFull();
};

////////////////////////////////////////////////////////////////////////////////
// SDL_ttf encapsulation. Mostly as same as Image
// Run TTF_Init outside.
class TTFFont
{
    friend class SpriteText;

protected:
    TTF_Font* _pFont = nullptr;
    bool _loaded = false;

public:
    TTFFont(const char* filePath, int ptsize);
    ~TTFFont();

public:
    // Attributes Settings
    void setStyle(TTFStyle style);
    void setOutline(bool enabled);
    void setHinting(TTFHinting mode);
    void setKerning(bool enabled);
    
    // Rendering Interfaces
    std::shared_ptr<Texture> TextUTF8(const char* text, const Color& c);
    std::shared_ptr<Texture> TextUTF8Solid(const char* text, const Color& c);
    std::shared_ptr<Texture> TextUTF8Shaded(const char* text, const Color& c, const Color& bg);
    Rect getRectUTF8(const char* text);
    //Rect getRectUTF16(const char* text);
};

////////////////////////////////////////////////////////////////////////////////
// Thick line wrapper
class LineMeta
{
public:
	static void draw(Point p1, Point p2, int width = 1, Color c = 0xffffffff);
};