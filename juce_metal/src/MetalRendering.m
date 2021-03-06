#include "MetalRendering.h"




// =============================================================================
static NSString* _Nonnull shaderSource = @""
"#include <metal_stdlib>\n"
"#include <simd/simd.h>\n"
"// ============================================================================\n"
"typedef struct\n"
"{\n"
"    vector_float4 position [[position]];\n"
"    vector_float4 color;\n"
"} RasterizerData;\n"
"// ============================================================================\n"
"constexpr metal::sampler colorMapSampler (metal::mag_filter::linear,\n"
"                                          metal::min_filter::linear);\n"
"// ============================================================================\n"
"vertex RasterizerData\n"
"vertexFunctionLiteralColors(unsigned int                     vertexID        [[ vertex_id  ]],\n"
"                            device const vector_float2      *vertexPositions [[ buffer (0) ]],\n"
"                            device const vector_float4      *vertexColors    [[ buffer (1) ]],\n"
"                            device const vector_float4      &domain          [[ buffer (2) ]])\n"
"{\n"
"    RasterizerData out;\n"
"    out.position.x = -1.f + 2.f * (vertexPositions[vertexID].x - domain[0]) / (domain[1] - domain[0]);\n"
"    out.position.y = -1.f + 2.f * (vertexPositions[vertexID].y - domain[2]) / (domain[3] - domain[2]);\n"
"    out.position.zw = vector_float2(0, 1);\n"
"    out.color = vertexColors[vertexID];\n"
"    return out;\n"
"}\n"
"// ============================================================================\n"
"vertex RasterizerData\n"
"vertexFunctionScalarMapping(unsigned int                    vertexID         [[ vertex_id    ]],\n"
"                            device const vector_float2     *vertexPositions  [[ buffer  (0)  ]],\n"
"                            device const float             *vertexScalars    [[ buffer  (1)  ]],\n"
"                            device const vector_float4     &domain           [[ buffer  (2)  ]],\n"
"                            device const vector_float2     &scalarDomain     [[ buffer  (3)  ]],\n"
"                            metal::texture1d<float>         colormap         [[ texture (4)  ]])\n"
"{\n"
"    float s = (vertexScalars[vertexID] - scalarDomain[0]) / (scalarDomain[1] - scalarDomain[0]);\n"
"    RasterizerData out;\n"
"    out.position.x = -1.f + 2.f * (vertexPositions[vertexID].x - domain[0]) / (domain[1] - domain[0]);\n"
"    out.position.y = -1.f + 2.f * (vertexPositions[vertexID].y - domain[2]) / (domain[3] - domain[2]);\n"
"    out.position.zw = vector_float2(0, 1);\n"
"    out.color = colormap.sample(colorMapSampler, s);\n"
"    return out;\n"
"}\n"
"// ============================================================================\n"
"fragment vector_float4\n"
"fragmentFunction(RasterizerData in [[stage_in]])\n"
"{\n"
"    return in.color;\n"
"};\n";




// =============================================================================
@implementation MetalRenderer
{
    id<MTLDevice> _device;
    id<MTLRenderPipelineState> _pipelineStateLiteralColors;
    id<MTLRenderPipelineState> _pipelineStateScalarMapping;
    id<MTLCommandQueue> _commandQueue;
    vector_float2 _viewportSize;
    MetalScene* _scene;
}




// =============================================================================
- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView*)mtkView
{
    if (self = [super init])
    {
        NSError *error = NULL;
        _device = mtkView.device;
        _commandQueue = [_device newCommandQueue];

        // Compile the shader library
        // =============================================================================
        id<MTLLibrary> library                      = [_device newLibraryWithSource:shaderSource options:nil error:&error];
        // id<MTLLibrary> library = [_device newDefaultLibrary];
        id<MTLFunction> vertexFunctionLiteralColors = [library newFunctionWithName:@"vertexFunctionLiteralColors"];
        id<MTLFunction> vertexFunctionScalarMapping = [library newFunctionWithName:@"vertexFunctionScalarMapping"];
        id<MTLFunction> fragmentFunction            = [library newFunctionWithName:@"fragmentFunction"];

        if (fragmentFunction == nil || vertexFunctionLiteralColors == nil || vertexFunctionScalarMapping == nil)
        {
            NSLog(@"%@ ", error.userInfo);
        }

        // Assemble the literal colors pipeline state
        // =============================================================================
        {
            MTLRenderPipelineDescriptor *psd = [[MTLRenderPipelineDescriptor alloc] init];
            psd.vertexFunction = vertexFunctionLiteralColors;
            psd.fragmentFunction = fragmentFunction;
            psd.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat;
            _pipelineStateLiteralColors = [_device newRenderPipelineStateWithDescriptor:psd error:&error];
        }

        // Assemble the scalar mapping pipeline state
        // =============================================================================
        {
            MTLRenderPipelineDescriptor *psd = [[MTLRenderPipelineDescriptor alloc] init];
            psd.vertexFunction = vertexFunctionScalarMapping;
            psd.fragmentFunction = fragmentFunction;
            psd.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat;
            _pipelineStateScalarMapping = [_device newRenderPipelineStateWithDescriptor:psd error:&error];
        }
    }
    return self;
}

- (void)setScene:(nullable MetalScene*)sceneToDisplay
{
    _scene = sceneToDisplay;
}

- (nullable MetalScene*)scene
{
    return _scene;
}

- (void)mtkView:(nonnull MTKView*)view drawableSizeWillChange:(CGSize)size
{
    _viewportSize.x = size.width;
    _viewportSize.y = size.height;
}

- (void)drawInMTKView:(nonnull MTKView*)view
{
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;

    if (_scene != nil && renderPassDescriptor != nil && ! NSIsEmptyRect (view.frame))
    {
        [self render:view.currentDrawable with:renderPassDescriptor blit:false];
    }
}

- (void)render:(nonnull id<CAMetalDrawable>)drawable
          with:(nonnull MTLRenderPassDescriptor*)renderPassDescriptor
          blit:(bool)blitTexture
{
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];


    simd_float4 domain = simd_make_float4(_scene.domain.xmin, _scene.domain.xmax,
                                          _scene.domain.ymin, _scene.domain.ymax);


    // Render in the direct colors pipeline state
    // =============================================================================
    [renderEncoder setRenderPipelineState:_pipelineStateLiteralColors];
    [renderEncoder setVertexBytes:&domain length:sizeof(domain) atIndex:2];

    for (MetalNode* node in _scene.nodes)
    {
        if (node.vertexColors != nil && node.vertexScalars == nil)
        {
            [renderEncoder setVertexBuffer:node.vertexPositions offset:0 atIndex:0];
            [renderEncoder setVertexBuffer:node.vertexColors offset:0 atIndex:1];
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:node.vertexCount];
        }
    }


    // Render in the scalar mapping pipeline state
    // =============================================================================
    [renderEncoder setRenderPipelineState:_pipelineStateScalarMapping];
    [renderEncoder setVertexBytes:&domain length:sizeof(domain) atIndex:2];

    for (MetalNode* node in _scene.nodes)
    {
        if (node.vertexColors == nil && node.vertexScalars != nil)
        {
            simd_float2 scalarDomain = simd_make_float2(node.scalarDomainLower, node.scalarDomainUpper);

            [renderEncoder setVertexBuffer:node.vertexPositions offset:0 atIndex:0];
            [renderEncoder setVertexBuffer:node.vertexScalars offset:0 atIndex:1];
            [renderEncoder setVertexBytes:&scalarDomain length:sizeof(scalarDomain) atIndex:3];
            [renderEncoder setVertexTexture:node.scalarMapping atIndex:4];
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:node.vertexCount];
        }
    }

    [renderEncoder endEncoding];


    // This Enables texture reads
    // =============================================================================
    if (blitTexture)
    {
        id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
        [blitEncoder synchronizeTexture:drawable.texture slice:0 level:0];
        [blitEncoder endEncoding];
    }

    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];

    if (blitTexture)
    {
        [commandBuffer waitUntilCompleted];
    }
}
@end




// =============================================================================
@implementation MetalViewController
{
    MetalRenderer* _renderer;
    MTKView* _view;
}

- (nonnull instancetype)init
{
    if (self = [super init])
    {
        _view = [[MTKView alloc] init];
        _view.framebufferOnly = false; // <- for reading texture
        _view.layer.opaque = false;
        _view.enableSetNeedsDisplay = true;
        _view.paused = true;
        _view.device = MTLCreateSystemDefaultDevice();
        _view.delegate = _renderer = [[MetalRenderer alloc] initWithMetalKitView:_view];
        self.view = _view;
    }
    return self;
}

- (nullable MetalScene*)scene
{
    return _renderer.scene;
}

- (void)setScene:(nullable MetalScene*)newScene
{
    _renderer.scene = newScene;
    // self.view.needsDisplay = true;
    [_view draw];
}

- (nullable NSImage*)createSnapshot
{
    [_renderer render:_view.currentDrawable with:_view.currentRenderPassDescriptor blit:true];

    id<MTLTexture> texture = _view.currentDrawable.texture;
    NSUInteger w = texture.width;
    NSUInteger h = texture.height;
    NSUInteger bytesPerRow = w * 4;

    NSMutableData* pixelBytes = [[NSMutableData alloc] initWithLength:w * h * 4];
    MTLRegion region = MTLRegionMake2D(0, 0, w, h);

    [texture getBytes:pixelBytes.mutableBytes
          bytesPerRow:bytesPerRow
           fromRegion:region
          mipmapLevel:0];

    NSData* immutableData = [NSData dataWithData:pixelBytes];
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)immutableData);
    CGBitmapInfo info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little;
    CGImageRef cgImage = CGImageCreate(w, h, 8, 32, bytesPerRow, colorSpace, info, provider, nil, true, kCGRenderingIntentDefault);
    CGColorSpaceRelease (colorSpace);
    return [[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize];
}

@end




// =============================================================================
@implementation MetalNode

- (nonnull instancetype)init
{
    if (self = [super init])
    {
        _vertexPositions = nil;
        _vertexColors = nil;
        _vertexScalars = nil;
        _scalarDomainLower = 0.f;
        _scalarDomainUpper = 1.f;
        _vertexCount = 0;
    }
    return self;
}

@end




// =============================================================================
@implementation MetalScene
{
    NSMutableArray<MetalNode*>* _nodes;
    struct MetalDomain _domain;
}

- (nonnull instancetype)init
{
    if (self = [super init])
    {
        _nodes = [[NSMutableArray<MetalNode*> alloc] init];
        _domain.xmin = 0;
        _domain.xmax = 1;
        _domain.ymin = 0;
        _domain.ymax = 1;
    }
    return self;
}

- (void)clear
{
    [_nodes removeAllObjects];
}

- (void)addNode:(nonnull MetalNode *)node
{
    [_nodes addObject:node];
}

- (nonnull NSMutableArray<MetalNode*> *)nodes
{
    return _nodes;
}

- (void)setDomain:(struct MetalDomain)domain
{
    _domain = domain;
}

- (struct MetalDomain)domain
{
    return _domain;
}

@end
