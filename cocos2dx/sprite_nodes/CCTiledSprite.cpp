/****************************************************************************
 Author: Luma (stubma@gmail.com)
 
 https://github.com/stubma/cocos2dx-classical
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/
#include "CCTiledSprite.h"
#include "textures/CCTextureAtlas.h"
#include "CCSpriteBatchNode.h"
#include "shaders/CCShaderCache.h"
#include "CCSpriteFrameCache.h"

NS_CC_BEGIN

CCTiledSprite::CCTiledSprite(CCSprite* sprite) :
m_sprite(NULL),
m_dirty(false),
m_atlas(NULL) {
	CCAssert(sprite != NULL, "CCTiledSprite doesn't accept NULL sprite");
			
	// save sprite
	m_sprite = sprite;
	CC_SAFE_RETAIN(m_sprite);
			
	// atlas
	m_atlas = CCTextureAtlas::createWithTexture(m_sprite->getTexture(), kDefaultSpriteBatchCapacity);
	CC_SAFE_RETAIN(m_atlas);
			
	// shader program
	setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor));
			
	// set content size
	setContentSize(m_sprite->getContentSize());
}

CCTiledSprite::~CCTiledSprite() {
	CC_SAFE_RELEASE(m_sprite);
	CC_SAFE_RELEASE(m_atlas);
}

CCTiledSprite* CCTiledSprite::create(const char* fileImage) {
	return CCTiledSprite::createWithSprite(CCSprite::create(fileImage));
}

CCTiledSprite* CCTiledSprite::createWithTexture(CCTexture2D* tex) {
	return CCTiledSprite::createWithSprite(CCSprite::createWithTexture(tex));
}

CCTiledSprite* CCTiledSprite::createWithSpriteFrameName(const char* name) {
	return createWithSpriteFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(name));
}

CCTiledSprite* CCTiledSprite::createWithSpriteFrame(CCSpriteFrame* frame) {
	return CCTiledSprite::createWithSprite(CCSprite::createWithSpriteFrame(frame));
}

CCTiledSprite* CCTiledSprite::createWithSprite(CCSprite* sprite) {
	CCTiledSprite* s = new CCTiledSprite(sprite);
    if(s->initWithTexture(sprite->getTexture(), sprite->getTextureRect(), sprite->isTextureRectRotated())) {
        CC_SAFE_AUTORELEASE_RETURN(s, CCTiledSprite*);
    }
	CC_SAFE_RELEASE(s);
    return NULL;
}

bool CCTiledSprite::initWithTexture(CCTexture2D *pTexture, const CCRect& rect, bool rotated) {
    if(!CCSprite::initWithTexture(pTexture, rect, rotated)) {
        return false;
    }
    
    return true;
}

void CCTiledSprite::setContentSize(const CCSize& contentSize) {
	CCSprite::setContentSize(contentSize);
	m_dirty = true;
}

void CCTiledSprite::setOpacity(GLubyte opacity) {
    CCSprite::setOpacity(opacity);
	m_dirty = true;
}

void CCTiledSprite::setHeight(float h) {
	CCSize size = getContentSize();
	size.height = h;
	setContentSize(size);
}

void CCTiledSprite::setWidth(float w) {
	CCSize size = getContentSize();
	size.width = w;
	setContentSize(size);
}

void CCTiledSprite::setDisplayFrame(CCSpriteFrame *pNewFrame) {
    CCSize oldSize = getContentSize();
    CCSprite::setDisplayFrame(pNewFrame);
    m_sprite->setDisplayFrame(pNewFrame);
    if(m_sprite->getTexture() != m_atlas->getTexture()) {
        m_atlas->setTexture(m_sprite->getTexture());
    }
    setContentSize(oldSize);
}

void CCTiledSprite::draw() {
	// build atlas
	if(m_dirty) {
		updateAtlas();
		m_dirty = false;
	}
	
	// profile start
	CC_PROFILER_START_CATEGORY(kCCProfilerCategorySprite, "CCTiledSprite - draw");
	
	// setup
    CC_NODE_DRAW_SETUP(this);
	
	// blend func
    ccGLBlendFunc(m_sBlendFunc.src, m_sBlendFunc.dst);
	
    // draw
	if(m_atlas)
		m_atlas->drawQuads();
	
	// profile end
    CC_PROFILER_STOP_CATEGORY(kCCProfilerCategorySprite, "CCTiledSprite - draw");
}

void CCTiledSprite::updateAtlas() {
	// ensure capacity
	CCSize size = getContentSize();
	CCSize tileSize = m_sprite->getContentSize();
	int hTiles = (int)ceilf(size.width / tileSize.width);
	int vTiles = (int)ceilf(size.height / tileSize.height);
	if(m_atlas->getCapacity() < hTiles * vTiles) {
		m_atlas->resizeCapacity(hTiles * vTiles);
	}
	
	// clear quads
	m_atlas->removeAllQuads();
	
	// original sprite
	ccV3F_C4B_T2F_Quad originalQuad = m_sprite->getQuad();
	
	// fill atlas
	ccV3F_C4B_T2F_Quad quad = originalQuad;
	float x = 0, y = 0;
	while(y < size.height) {
		if(y + tileSize.height > size.height) {
			float p = (size.height - y) / tileSize.height;
			quad.tl.vertices.y = quad.bl.vertices.y * (1 - p) + quad.tl.vertices.y * p;
			quad.tr.vertices.y = quad.br.vertices.y * (1 - p) + quad.tr.vertices.y * p;
			quad.tl.texCoords.v = quad.bl.texCoords.v * (1 - p) + quad.tl.texCoords.v * p;
			quad.tr.texCoords.v = quad.br.texCoords.v * (1 - p) + quad.tr.texCoords.v * p;
		}
		
		while(x < size.width) {
			// special process for edge
			if(x + tileSize.width > size.width) {
				float p = (size.width - x) / tileSize.width;
				quad.br.vertices.x = quad.bl.vertices.x * (1 - p) + quad.br.vertices.x * p;
				quad.tr.vertices.x = quad.tl.vertices.x * (1 - p) + quad.tr.vertices.x * p;
				quad.br.texCoords.u = quad.bl.texCoords.u * (1 - p) + quad.br.texCoords.u * p;
				quad.tr.texCoords.u = quad.tl.texCoords.u * (1 - p) + quad.tr.texCoords.u * p;
			}
			
			// add quad
			m_atlas->insertQuad(&quad, m_atlas->getTotalQuads());
			
			// increase
			x += tileSize.width;
			quad.bl.vertices.x += tileSize.width;
			quad.br.vertices.x += tileSize.width;
			quad.tl.vertices.x += tileSize.width;
			quad.tr.vertices.x += tileSize.width;
		}
		
		// move
		x = 0;
		y += tileSize.height;
		
		// reset quad and increase y
		quad = originalQuad;
		quad.bl.vertices.y += y;
		quad.br.vertices.y += y;
		quad.tl.vertices.y += y;
		quad.tr.vertices.y += y;
	}
}

NS_CC_END