//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer9.h: Defines a back-end specific class for the D3D9 renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER9_H_
#define LIBGLESV2_RENDERER_RENDERER9_H_

#include <set>
#include <map>
#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define EGLAPI
#include <EGL/egl.h>

#include <d3d9.h>
#include <D3Dcompiler.h>

#include "common/angleutils.h"
#include "libGLESv2/renderer/ShaderCache.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Context.h"

namespace gl
{
class VertexDataManager;
}

namespace rx
{

// Helper class to construct and cache vertex declarations
class VertexDeclarationCache
{
  public:
    VertexDeclarationCache();
    ~VertexDeclarationCache();

    GLenum applyDeclaration(IDirect3DDevice9 *device, gl::TranslatedAttribute attributes[], gl::ProgramBinary *programBinary, GLsizei instances, GLsizei *repeatDraw);

    void markStateDirty();

  private:
    UINT mMaxLru;

    enum { NUM_VERTEX_DECL_CACHE_ENTRIES = 32 };

    struct VBData
    {
        unsigned int serial;
        unsigned int stride;
        unsigned int offset;
    };

    VBData mAppliedVBs[gl::MAX_VERTEX_ATTRIBS];
    IDirect3DVertexDeclaration9 *mLastSetVDecl;
    bool mInstancingEnabled;

    struct VertexDeclCacheEntry
    {
        D3DVERTEXELEMENT9 cachedElements[gl::MAX_VERTEX_ATTRIBS + 1];
        UINT lruCount;
        IDirect3DVertexDeclaration9 *vertexDeclaration;
    } mVertexDeclCache[NUM_VERTEX_DECL_CACHE_ENTRIES];
};

class Renderer9 : public Renderer
{
  public:
    Renderer9(egl::Display *display, HDC hDc, bool softwareDevice);
    virtual ~Renderer9();

    static Renderer9 *makeRenderer9(Renderer *renderer);

    virtual EGLint initialize();
    virtual bool resetDevice();

    virtual int generateConfigs(ConfigDesc **configDescList);
    virtual void deleteConfigs(ConfigDesc *configDescList);

    virtual void startScene();
    virtual void endScene();

    virtual void sync(bool block);

    virtual SwapChain *createSwapChain(HWND window, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat);

    IDirect3DQuery9* allocateEventQuery();
    void freeEventQuery(IDirect3DQuery9* query);

    // resource creation
    IDirect3DVertexShader9 *createVertexShader(const DWORD *function, size_t length);
    IDirect3DPixelShader9 *createPixelShader(const DWORD *function, size_t length);
    HRESULT compileShaderSource(const char* hlsl, const char* sourceName, const char* profile, DWORD flags, ID3DBlob** binary, ID3DBlob** errorMessage);
    HRESULT createVertexBuffer(UINT Length, DWORD Usage, IDirect3DVertexBuffer9 **ppVertexBuffer);
    HRESULT createIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, IDirect3DIndexBuffer9 **ppIndexBuffer);
#if 0
    void *createTexture2D();
    void *createTextureCube();
    void *createQuery();
    void *createIndexBuffer();
    void *createVertexbuffer();

    // state setup
    void applyShaders();
    void applyConstants();
#endif
    virtual void setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &sampler);
    virtual void setTexture(gl::SamplerType type, int index, gl::Texture *texture);

    virtual void setRasterizerState(const gl::RasterizerState &rasterState, unsigned int depthSize);
    virtual void setBlendState(const gl::BlendState &blendState, const gl::Color &blendColor,
                               unsigned int sampleMask);
    virtual void setDepthStencilState(const gl::DepthStencilState &depthStencilState, int stencilRef,
                                      int stencilBackRef, bool frontFaceCCW, unsigned int stencilSize);

    virtual void setScissorRectangle(const gl::Rectangle& scissor, unsigned int renderTargetWidth,
                                     unsigned int renderTargetHeight);
    virtual bool setViewport(const gl::Rectangle& viewport, float zNear, float zFar,
                             unsigned int renderTargetWidth, unsigned int renderTargetHeight,
                             gl::ProgramBinary *currentProgram, bool forceSetUniforms);

    virtual bool applyRenderTarget(gl::Framebuffer *frameBuffer);
    virtual void applyShaders(gl::ProgramBinary *programBinary);

    virtual GLenum applyVertexBuffer(gl::ProgramBinary *programBinary, gl::VertexAttribute vertexAttributes[], GLint first, GLsizei count, GLsizei instances, GLsizei *repeatDraw);

    virtual void clear(const gl::ClearParameters &clearParams, gl::Framebuffer *frameBuffer);

    virtual void markAllStateDirty();

    // lost device
    virtual void markDeviceLost();
    virtual bool isDeviceLost();
    virtual bool testDeviceLost(bool notify);
    virtual bool testDeviceResettable();

    // Renderer capabilities
    IDirect3DDevice9 *getDevice() { return mDevice; }
    virtual DWORD getAdapterVendor() const;
    virtual const char *getAdapterDescription() const;
    virtual GUID getAdapterIdentifier() const;

    virtual bool getDXT1TextureSupport();
    virtual bool getDXT3TextureSupport();
    virtual bool getDXT5TextureSupport();
    virtual bool getEventQuerySupport();
    virtual bool getFloat32TextureSupport(bool *filtering, bool *renderable);
    virtual bool getFloat16TextureSupport(bool *filtering, bool *renderable);
    virtual bool getLuminanceTextureSupport();
    virtual bool getLuminanceAlphaTextureSupport();
    virtual bool getVertexTextureSupport() const;
    virtual bool getNonPower2TextureSupport() const;
    virtual bool getDepthTextureSupport() const;
    virtual bool getOcclusionQuerySupport() const;
    virtual bool getInstancingSupport() const;
    virtual bool getTextureFilterAnisotropySupport() const;
    virtual float getTextureMaxAnisotropy() const;
    virtual bool getShareHandleSupport() const;

    virtual bool getShaderModel3Support() const;
    virtual float getMaxPointSize() const;
    virtual int getMaxTextureWidth() const;
    virtual int getMaxTextureHeight() const;
    virtual bool get32BitIndexSupport() const;
    DWORD getCapsDeclTypes() const; // D3D9_REPLACE
    virtual int getMinSwapInterval() const;
    virtual int getMaxSwapInterval() const;

    virtual GLsizei getMaxSupportedSamples() const;
    int getNearestSupportedSamples(D3DFORMAT format, int requested) const;
    
    D3DFORMAT ConvertTextureInternalFormat(GLint internalformat);

    virtual bool copyToRenderTarget(TextureStorage2D *dest, TextureStorage2D *source);
    virtual bool copyToRenderTarget(TextureStorageCubeMap *dest, TextureStorageCubeMap *source);

    virtual bool copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorage2D *storage, GLint level);
    virtual bool copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageCubeMap *storage, GLenum target, GLint level);

    virtual bool blitRect(gl::Framebuffer *readTarget, gl::Rectangle *readRect, gl::Framebuffer *drawTarget, gl::Rectangle *drawRect,
                          bool blitRenderTarget, bool blitDepthStencil);
    virtual void readPixels(gl::Framebuffer *framebuffer, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                            GLsizei outputPitch, bool packReverseRowOrder, GLint packAlignment, void* pixels);

    virtual RenderTarget *createRenderTarget(SwapChain *swapChain, bool depth);
    virtual RenderTarget *createRenderTarget(int width, int height, GLenum format, GLsizei samples, bool depth);

    bool boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest);

    D3DPOOL getTexturePool(DWORD usage) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer9);

    void getMultiSampleSupport(D3DFORMAT format, bool *multiSampleArray);
    bool copyToRenderTarget(IDirect3DSurface9 *dest, IDirect3DSurface9 *source, bool fromManaged);

    D3DPOOL getBufferPool(DWORD usage) const;

    HMODULE mD3d9Module;
    HDC mDc;

    typedef HRESULT (WINAPI *D3DCompileFunc)(LPCVOID pSrcData,
                                             SIZE_T SrcDataSize,
                                             LPCSTR pSourceName,
                                             CONST D3D_SHADER_MACRO* pDefines,
                                             ID3DInclude* pInclude,
                                             LPCSTR pEntrypoint,
                                             LPCSTR pTarget,
                                             UINT Flags1,
                                             UINT Flags2,
                                             ID3DBlob** ppCode,
                                             ID3DBlob** ppErrorMsgs);

    HMODULE mD3dCompilerModule;
    D3DCompileFunc mD3DCompileFunc;

    void initializeDevice();
    D3DPRESENT_PARAMETERS getDefaultPresentParameters();
    void releaseDeviceResources();

    UINT mAdapter;
    D3DDEVTYPE mDeviceType;
    bool mSoftwareDevice;   // FIXME: Deprecate
    IDirect3D9 *mD3d9;  // Always valid after successful initialization.
    IDirect3D9Ex *mD3d9Ex;  // Might be null if D3D9Ex is not supported.
    IDirect3DDevice9 *mDevice;
    IDirect3DDevice9Ex *mDeviceEx;  // Might be null if D3D9Ex is not supported.

    Blit *mBlit;

    HWND mDeviceWindow;

    bool mDeviceLost;
    D3DCAPS9 mDeviceCaps;
    D3DADAPTER_IDENTIFIER9 mAdapterIdentifier;

    bool mSceneStarted;
    bool mSupportsNonPower2Textures;
    bool mSupportsTextureFilterAnisotropy;
    int mMinSwapInterval;
    int mMaxSwapInterval;

    std::map<D3DFORMAT, bool *> mMultiSampleSupport;
    GLsizei mMaxSupportedSamples;

    // current render target states
    unsigned int mAppliedRenderTargetSerial;
    unsigned int mAppliedDepthbufferSerial;
    unsigned int mAppliedStencilbufferSerial;
    bool mDepthStencilInitialized;
    bool mRenderTargetDescInitialized;
    rx::RenderTarget::Desc mRenderTargetDesc;

    IDirect3DStateBlock9 *mMaskedClearSavedState;

    // previously set render states
    bool mForceSetDepthStencilState;
    gl::DepthStencilState mCurDepthStencilState;
    int mCurStencilRef;
    int mCurStencilBackRef;
    bool mCurFrontFaceCCW;
    unsigned int mCurStencilSize;

    bool mForceSetRasterState;
    gl::RasterizerState mCurRasterState;
    unsigned int mCurDepthSize;

    bool mForceSetScissor;
    gl::Rectangle mCurScissor;
    unsigned int mCurRenderTargetWidth;
    unsigned int mCurRenderTargetHeight;

    bool mForceSetViewport;
    gl::Rectangle mCurViewport;
    float mCurNear;
    float mCurFar;

    bool mForceSetBlendState;
    gl::BlendState mCurBlendState;
    gl::Color mCurBlendColor;
    GLuint mCurSampleMask;

    // A pool of event queries that are currently unused.
    std::vector<IDirect3DQuery9*> mEventQueryPool;
    VertexShaderCache mVertexShaderCache;
    PixelShaderCache mPixelShaderCache;

    gl::VertexDataManager *mVertexDataManager;
    VertexDeclarationCache mVertexDeclarationCache;
};

}
#endif // LIBGLESV2_RENDERER_RENDERER9_H_