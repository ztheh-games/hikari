#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/core/util/FileSystem.hpp"
#include "hikari/core/util/Log.hpp"
#include <SDL3/SDL.h>
#include <json/reader.h>
#include <algorithm>
#include <cstring>
#include <exception>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace hikari::gfx {
namespace {
void require(bool success, const char * operation) {
    if (!success) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}
std::vector<std::uint8_t> shaderBytes(const std::string & path) {
    auto file = FileSystem::openFileRead(path);
    return {std::istreambuf_iterator<char>(*file), std::istreambuf_iterator<char>()};
}
bool bindingCount(const Json::Value & value, Uint32 expected) {
    return (value.isUInt() || (value.isInt() && value.asInt() >= 0)) && value.asUInt() == expected;
}
bool same(const Draw & a, const Draw & b) {
    return a.material == b.material && a.texture.image == b.texture.image && a.texture.pass == b.texture.pass
        && a.palette.image == b.palette.image && a.palette.pass == b.palette.pass
        && a.parameters == b.parameters && a.clip.position.x == b.clip.position.x && a.clip.position.y == b.clip.position.y
        && a.clip.size.x == b.clip.size.x && a.clip.size.y == b.clip.size.y;
}
struct CopyPassEnd {
    void operator()(SDL_GPUCopyPass * pass) const { SDL_EndGPUCopyPass(pass); }
};
struct RenderPassEnd {
    void operator()(SDL_GPURenderPass * pass) const { SDL_EndGPURenderPass(pass); }
};
struct TransferRelease {
    SDL_GPUDevice * device;
    void operator()(SDL_GPUTransferBuffer * transfer) const { SDL_ReleaseGPUTransferBuffer(device,transfer); }
};
struct CommandOwner {
    SDL_GPUCommandBuffer * command;
    bool swapchainAcquired = false;
    ~CommandOwner() {
        if (!command) return;
        const bool ok = swapchainAcquired ? SDL_SubmitGPUCommandBuffer(command) : SDL_CancelGPUCommandBuffer(command);
        if (!ok) HIKARI_LOG(error) << "GPU command cleanup: " << SDL_GetError();
    }
    SDL_GPUCommandBuffer * release() { auto * result = command; command = nullptr; return result; }
};
}

struct Renderer::Impl {
    SDL_GPUDevice * device = nullptr;
    SDL_Window * window = nullptr;
    bool claimed = false;
    SDL_GPUSampler * nearest = nullptr, * linear = nullptr;
    SDL_GPUBuffer * vertexBuffer = nullptr, * indexBuffer = nullptr;
    SDL_GPUTransferBuffer * vertexUpload = nullptr, * indexUpload = nullptr;
    Uint32 capacity = 0;
    std::size_t texturesCreated = 0, pipelinesCreated = 0, skippedPresentations = 0;
    SDL_GPUShaderFormat format{};
    std::map<std::pair<Material, SDL_GPUTextureFormat>, SDL_GPUGraphicsPipeline *> pipelines;
    std::vector<SDL_GPUShader *> shaders;
    struct Allocation {
        std::weak_ptr<ImageData> owner;
        SDL_GPUTexture * texture;
        std::uint64_t revision;
        Vector2u size;
        bool target;
    };
    std::unordered_map<ImageData *, Allocation> images;
    struct TargetAllocation {
        std::weak_ptr<const RecordedPass> pass;
        std::weak_ptr<ImageData> owner;
        SDL_GPUTexture * texture;
        Vector2u size;
    };
    std::unordered_map<const RecordedPass *, TargetAllocation> targets;
    std::shared_ptr<ImageData> white = std::make_shared<ImageData>(ImageData{{1,1}, {255,255,255,255}, false, 1});

    ~Impl() {
        if (!device) return;
        SDL_WaitForGPUIdle(device);
        for (auto & item : images) SDL_ReleaseGPUTexture(device, item.second.texture);
        for (auto & item : targets) SDL_ReleaseGPUTexture(device,item.second.texture);
        for (auto & item : pipelines) SDL_ReleaseGPUGraphicsPipeline(device, item.second);
        for (auto * shader : shaders) SDL_ReleaseGPUShader(device, shader);
        if (nearest) SDL_ReleaseGPUSampler(device, nearest);
        if (linear) SDL_ReleaseGPUSampler(device, linear);
        if (vertexBuffer) SDL_ReleaseGPUBuffer(device, vertexBuffer);
        if (indexBuffer) SDL_ReleaseGPUBuffer(device, indexBuffer);
        if (vertexUpload) SDL_ReleaseGPUTransferBuffer(device, vertexUpload);
        if (indexUpload) SDL_ReleaseGPUTransferBuffer(device, indexUpload);
        if (claimed) SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
    }
    void initialize(SDL_Window * nativeWindow, bool vsync) {
        window = nativeWindow;
        bool debug = false;
#ifndef NDEBUG
        debug = true;
#endif
        device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, debug, nullptr);
        require(device != nullptr, "Create SDL GPU device (D3D12, Vulkan or Metal required)");
        if (window) {
            require(SDL_ClaimWindowForGPUDevice(device, window), "Claim GPU window");
            claimed = true;
            auto mode = SDL_GPU_PRESENTMODE_VSYNC;
            if (!vsync) {
                if (SDL_WindowSupportsGPUPresentMode(device, window, SDL_GPU_PRESENTMODE_IMMEDIATE)) mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
                else if (SDL_WindowSupportsGPUPresentMode(device, window, SDL_GPU_PRESENTMODE_MAILBOX)) mode = SDL_GPU_PRESENTMODE_MAILBOX;
                else HIKARI_LOG(warning) << "Non-vsync presentation unavailable; using vsync.";
            }
            require(SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode), "Set GPU presentation mode");
        }
        const auto formats = SDL_GetGPUShaderFormats(device);
        format = (formats & SDL_GPU_SHADERFORMAT_SPIRV) ? SDL_GPU_SHADERFORMAT_SPIRV
               : (formats & SDL_GPU_SHADERFORMAT_DXIL) ? SDL_GPU_SHADERFORMAT_DXIL : SDL_GPU_SHADERFORMAT_MSL;
        SDL_GPUSamplerCreateInfo sampler{};
        sampler.min_filter = sampler.mag_filter = SDL_GPU_FILTER_NEAREST;
        sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        sampler.address_mode_u = sampler.address_mode_v = sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        nearest = SDL_CreateGPUSampler(device, &sampler);
        require(nearest != nullptr, "Create nearest sampler");
        sampler.min_filter = sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
        linear = SDL_CreateGPUSampler(device, &sampler);
        require(linear != nullptr, "Create linear sampler");
        HIKARI_LOG(info) << "SDL GPU backend: " << SDL_GetGPUDeviceDriver(device);
    }
    SDL_GPUShader * shader(const char * name, SDL_GPUShaderStage stage, Uint32 samplers, Uint32 uniforms) {
        Json::Value metadata;
        Json::Reader reader;
        auto manifest = FileSystem::openFileRead(std::string("assets/shaders/gpu/") + name + ".json");
        if (!reader.parse(*manifest,metadata,false) || !bindingCount(metadata["samplers"],samplers) ||
            !bindingCount(metadata["uniform_buffers"],uniforms) ||
            !bindingCount(metadata["storage_textures"],0) || !bindingCount(metadata["storage_buffers"],0))
            throw std::runtime_error(std::string("Incompatible GPU shader binding metadata: ") + name);
        const char * suffix = format == SDL_GPU_SHADERFORMAT_SPIRV ? ".spv" : format == SDL_GPU_SHADERFORMAT_DXIL ? ".dxil" : ".msl";
        const auto bytes = shaderBytes(std::string("assets/shaders/gpu/") + name + suffix);
        if (bytes.empty()) throw std::runtime_error(std::string("Empty GPU shader: ") + name);
        SDL_GPUShaderCreateInfo info{};
        info.code = bytes.data(); info.code_size = bytes.size();
        info.entrypoint = format == SDL_GPU_SHADERFORMAT_MSL ? "main0" : "main";
        info.format = format; info.stage = stage; info.num_samplers = samplers; info.num_uniform_buffers = uniforms;
        auto * result = SDL_CreateGPUShader(device, &info);
        require(result != nullptr, name);
        shaders.push_back(result);
        return result;
    }
    SDL_GPUGraphicsPipeline * pipeline(Material material, SDL_GPUTextureFormat targetFormat) {
        const auto key = std::make_pair(material, targetFormat);
        if (auto it = pipelines.find(key); it != pipelines.end()) return it->second;
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = shader("quad", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        info.fragment_shader = shader(material == Material::Palette ? "palette" : material == Material::Fade ? "fade" : "textured",
            SDL_GPU_SHADERSTAGE_FRAGMENT, material == Material::Palette ? 2 : 1, material == Material::Textured ? 0 : 1);
        SDL_GPUVertexBufferDescription buffer{};
        buffer.slot = 0; buffer.pitch = sizeof(GPUVertex); buffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        SDL_GPUVertexAttribute attributes[3]{};
        attributes[0] = {0,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,0};
        attributes[1] = {1,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,8};
        attributes[2] = {2,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,16};
        info.vertex_input_state.vertex_buffer_descriptions = &buffer;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attributes;
        info.vertex_input_state.num_vertex_attributes = 3;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
        SDL_GPUColorTargetDescription target{};
        target.format = targetFormat;
        target.blend_state.enable_blend = true;
        target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        info.target_info.color_target_descriptions = &target; info.target_info.num_color_targets = 1;
        auto * result = SDL_CreateGPUGraphicsPipeline(device, &info);
        require(result != nullptr, "Create quad pipeline");
        ++pipelinesCreated;
        pipelines.emplace(key, result);
        return result;
    }
    SDL_GPUTexture * texture(const TextureSnapshot & snapshot, SDL_GPUCopyPass * copy) {
        const auto owner = snapshot.image ? snapshot.image : white;
        if (snapshot.pass) {
            auto current = targets.find(snapshot.pass.get());
            if (current != targets.end() && current->second.pass.lock() == snapshot.pass) return current->second.texture;
            SDL_GPUTexture * target = nullptr;
            if (current != targets.end()) {
                const auto & allocation = current->second;
                if (allocation.owner.lock() == owner && allocation.size.x == owner->size.x && allocation.size.y == owner->size.y)
                    target = allocation.texture;
                else SDL_ReleaseGPUTexture(device,allocation.texture);
                targets.erase(current);
            }
            for (auto it = targets.begin(); !target && it != targets.end(); ++it) {
                const auto & allocation = it->second;
                if (allocation.pass.expired() && allocation.owner.lock() == owner &&
                    allocation.size.x == owner->size.x && allocation.size.y == owner->size.y) {
                    target = allocation.texture; targets.erase(it); break;
                }
            }
            if (!target) {
                SDL_GPUTextureCreateInfo info{};
                info.type = SDL_GPU_TEXTURETYPE_2D; info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
                info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
                info.width = owner->size.x; info.height = owner->size.y; info.layer_count_or_depth = 1; info.num_levels = 1;
                info.sample_count = SDL_GPU_SAMPLECOUNT_1;
                target = SDL_CreateGPUTexture(device,&info);
                require(target != nullptr,"Create recorded GPU target");
                ++texturesCreated;
            }
            targets.emplace(snapshot.pass.get(),TargetAllocation{snapshot.pass,owner,target,owner->size});
            return target;
        }
        auto it = images.find(owner.get());
        if (it != images.end() && (it->second.size.x != owner->size.x || it->second.size.y != owner->size.y)) {
            SDL_ReleaseGPUTexture(device, it->second.texture);
            images.erase(it); it = images.end();
        }
        if (it == images.end()) {
            SDL_GPUTextureCreateInfo info{};
            info.type = SDL_GPU_TEXTURETYPE_2D; info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
            info.width = owner->size.x; info.height = owner->size.y; info.layer_count_or_depth = 1; info.num_levels = 1;
            info.sample_count = SDL_GPU_SAMPLECOUNT_1;
            auto * created = SDL_CreateGPUTexture(device, &info);
            require(created != nullptr, "Create GPU texture");
            ++texturesCreated;
            it = images.emplace(owner.get(), Allocation{owner,created,0,owner->size,snapshot.pass != nullptr}).first;
        }
        auto & allocation = it->second;
        if (!snapshot.pass && allocation.revision != owner->revision) {
            if (!copy) throw std::logic_error("Texture upload outside copy pass");
            if (owner->pixels.empty() || owner->pixels.size() > std::numeric_limits<Uint32>::max())
                throw std::runtime_error("Invalid texture upload size");
            SDL_GPUTransferBufferCreateInfo uploadInfo{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,static_cast<Uint32>(owner->pixels.size()),0};
            auto * upload = SDL_CreateGPUTransferBuffer(device, &uploadInfo);
            require(upload != nullptr, "Create texture transfer buffer");
            auto * mapped = SDL_MapGPUTransferBuffer(device, upload, false);
            if (!mapped) { SDL_ReleaseGPUTransferBuffer(device, upload); require(false, "Map texture upload"); }
            std::memcpy(mapped, owner->pixels.data(), owner->pixels.size());
            SDL_UnmapGPUTransferBuffer(device, upload);
            SDL_GPUTextureTransferInfo source{};
            source.transfer_buffer = upload; source.pixels_per_row = owner->size.x; source.rows_per_layer = owner->size.y;
            SDL_GPUTextureRegion destination{};
            destination.texture = allocation.texture; destination.w = owner->size.x; destination.h = owner->size.y; destination.d = 1;
            SDL_UploadToGPUTexture(copy, &source, &destination, true);
            SDL_ReleaseGPUTransferBuffer(device, upload);
            allocation.revision = owner->revision;
        }
        return allocation.texture;
    }
    void buffers(Uint32 required) {
        if (required <= capacity) return;
        constexpr auto maximum = std::numeric_limits<Uint32>::max() / (4 * sizeof(GPUVertex));
        if (required > maximum) throw std::runtime_error("Geometry buffer limit exceeded");
        if (vertexBuffer) SDL_ReleaseGPUBuffer(device, vertexBuffer);
        if (indexBuffer) SDL_ReleaseGPUBuffer(device, indexBuffer);
        if (vertexUpload) SDL_ReleaseGPUTransferBuffer(device, vertexUpload);
        if (indexUpload) SDL_ReleaseGPUTransferBuffer(device, indexUpload);
        vertexBuffer = indexBuffer = nullptr; vertexUpload = indexUpload = nullptr;
        capacity = std::max(required,static_cast<Uint32>(capacity ? std::min<std::size_t>(maximum,static_cast<std::size_t>(capacity)*2) : 1024u));
        SDL_GPUBufferCreateInfo vertexInfo{SDL_GPU_BUFFERUSAGE_VERTEX,capacity * 4 * static_cast<Uint32>(sizeof(GPUVertex)),0};
        SDL_GPUBufferCreateInfo indexInfo{SDL_GPU_BUFFERUSAGE_INDEX,capacity * 6 * static_cast<Uint32>(sizeof(Uint32)),0};
        vertexBuffer = SDL_CreateGPUBuffer(device, &vertexInfo); indexBuffer = SDL_CreateGPUBuffer(device, &indexInfo);
        require(vertexBuffer && indexBuffer, "Create geometry buffers");
        SDL_GPUTransferBufferCreateInfo vInfo{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,vertexInfo.size,0};
        SDL_GPUTransferBufferCreateInfo iInfo{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,indexInfo.size,0};
        vertexUpload = SDL_CreateGPUTransferBuffer(device, &vInfo); indexUpload = SDL_CreateGPUTransferBuffer(device, &iInfo);
        require(vertexUpload && indexUpload, "Create geometry uploads");
    }
    void collect(const TextureSnapshot & image, std::set<const RecordedPass *> & visited, std::set<const RecordedPass *> & active,
                 std::vector<std::shared_ptr<const RecordedPass>> & passes) {
        if (!image.pass || visited.count(image.pass.get())) return;
        if (!active.insert(image.pass.get()).second) throw std::logic_error("Cyclic render target dependency");
        for (const auto & draw : image.pass->draws) {
            if (draw.texture.image == image.pass->output || draw.palette.image == image.pass->output)
                throw std::logic_error("Sampling an active render target");
            if (draw.material == Material::Palette && (!draw.palette.image || draw.parameters[1] <= 0 || draw.parameters[2] <= 0))
                throw std::logic_error("Palette draw requires a color table and valid dimensions");
            collect(draw.texture, visited, active, passes); collect(draw.palette, visited, active, passes);
        }
        active.erase(image.pass.get()); visited.insert(image.pass.get()); passes.push_back(image.pass);
    }
    void encode(SDL_GPUCommandBuffer * command, const TextureSnapshot & image,
                SDL_GPUTexture * swapchain, Uint32 width, Uint32 height) {
        struct UploadRollback {
            Impl & renderer;
            int exceptions = std::uncaught_exceptions();
            ~UploadRollback() {
                if (std::uncaught_exceptions() > exceptions)
                    for (auto & image : renderer.images) image.second.revision = 0;
            }
        } rollback{*this};
        for (auto it = images.begin(); it != images.end();) {
            if (it->second.owner.expired()) { SDL_ReleaseGPUTexture(device, it->second.texture); it = images.erase(it); }
            else ++it;
        }
        for (auto it = targets.begin(); it != targets.end();) {
            if (it->second.owner.expired()) { SDL_ReleaseGPUTexture(device,it->second.texture); it = targets.erase(it); }
            else ++it;
        }
        std::vector<std::shared_ptr<const RecordedPass>> passes;
        std::set<const RecordedPass *> visited, active;
        collect(image, visited, active, passes);
        RecordedPass presentation;
        if (swapchain) {
            RenderTarget screen({width,height});
            Texture source; source.snapshot = image;
            Sprite sprite(source);
            sprite.setScale({static_cast<float>(width)/source.getSize().x,static_cast<float>(height)/source.getSize().y});
            screen.draw(sprite);
            presentation.clear = Color::Blue; presentation.draws = screen.getDraws();
        }
        std::size_t count = presentation.draws.size();
        for (const auto & pass : passes) count += pass->draws.size();
        if (count > std::numeric_limits<Uint32>::max() / (4 * sizeof(GPUVertex)))
            throw std::runtime_error("Frame geometry exceeds GPU buffer limits");
        buffers(static_cast<Uint32>(std::max<std::size_t>(count,1)));
        auto * vertices = static_cast<GPUVertex *>(SDL_MapGPUTransferBuffer(device, vertexUpload, true));
        require(vertices != nullptr, "Map frame vertices");
        auto * indices = static_cast<Uint32 *>(SDL_MapGPUTransferBuffer(device, indexUpload, true));
        if (!indices) { SDL_UnmapGPUTransferBuffer(device, vertexUpload); require(false, "Map frame indices"); }
        std::size_t offset = 0;
        auto pack = [&](const std::vector<Draw> & draws) {
            for (const auto & draw : draws) {
                std::memcpy(vertices + offset*4, draw.vertices.data(), sizeof(GPUVertex)*4);
                const Uint32 base = static_cast<Uint32>(offset*4);
                const Uint32 quadIndices[6]{base,base+1,base+2,base,base+2,base+3};
                std::memcpy(indices + offset*6,quadIndices,sizeof(quadIndices)); ++offset;
            }
        };
        for (const auto & pass : passes) pack(pass->draws);
        pack(presentation.draws);
        SDL_UnmapGPUTransferBuffer(device, vertexUpload); SDL_UnmapGPUTransferBuffer(device, indexUpload);
        std::unique_ptr<SDL_GPUCopyPass,CopyPassEnd> copyOwner(SDL_BeginGPUCopyPass(command));
        auto * copy = copyOwner.get();
        require(copy != nullptr, "Begin frame upload");
        if (count) {
            SDL_GPUTransferBufferLocation source{vertexUpload,0};
            SDL_GPUBufferRegion destination{vertexBuffer,0,static_cast<Uint32>(count*4*sizeof(GPUVertex))};
            SDL_UploadToGPUBuffer(copy,&source,&destination,true);
            source.transfer_buffer = indexUpload;
            destination = {indexBuffer,0,static_cast<Uint32>(count*6*sizeof(Uint32))};
            SDL_UploadToGPUBuffer(copy,&source,&destination,true);
        }
        texture(image,copy);
        for (const auto & pass : passes) {
            texture({pass->output,pass},copy);
            for (const auto & draw : pass->draws) {
                texture(draw.texture,copy);
                if (draw.material == Material::Palette) texture(draw.palette,copy);
            }
        }
        copyOwner.reset();
        offset = 0;
        auto render = [&](const RecordedPass & recorded, SDL_GPUTexture * output, Vector2u size, SDL_GPUTextureFormat targetFormat) {
            SDL_GPUColorTargetInfo target{};
            target.texture = output; target.load_op = SDL_GPU_LOADOP_CLEAR; target.store_op = SDL_GPU_STOREOP_STORE;
            const auto c = recorded.clear;
            target.clear_color = {c.r/255.0f,c.g/255.0f,c.b/255.0f,c.a/255.0f};
            std::unique_ptr<SDL_GPURenderPass,RenderPassEnd> passOwner(SDL_BeginGPURenderPass(command,&target,1,nullptr));
            auto * pass = passOwner.get();
            require(pass != nullptr, "Begin target rendering");
            SDL_GPUBufferBinding vb{vertexBuffer,0}, ib{indexBuffer,0};
            SDL_BindGPUVertexBuffers(pass,0,&vb,1); SDL_BindGPUIndexBuffer(pass,&ib,SDL_GPU_INDEXELEMENTSIZE_32BIT);
            const float projection[4]{static_cast<float>(size.x),static_cast<float>(size.y),0,0};
            SDL_PushGPUVertexUniformData(command,0,projection,sizeof(projection));
            for (std::size_t i = 0; i < recorded.draws.size();) {
                const auto & draw = recorded.draws[i];
                std::size_t end = i+1;
                while (end < recorded.draws.size() && same(draw,recorded.draws[end])) ++end;
                SDL_BindGPUGraphicsPipeline(pass,pipeline(draw.material,targetFormat));
                const auto sampled = draw.texture.image ? draw.texture : TextureSnapshot{white,{}};
                SDL_GPUTextureSamplerBinding bindings[2]{
                    {texture(sampled,nullptr),sampled.image->smooth ? linear : nearest},
                    {nullptr,nearest}
                };
                if (draw.material == Material::Palette) {
                    if (!draw.palette.image) throw std::runtime_error("Palette draw without color table");
                    bindings[1].texture = texture(draw.palette,nullptr);
                }
                SDL_BindGPUFragmentSamplers(pass,0,bindings,draw.material == Material::Palette ? 2 : 1);
                if (draw.material != Material::Textured)
                    SDL_PushGPUFragmentUniformData(command,0,draw.parameters.data(),sizeof(draw.parameters));
                SDL_Rect scissor{std::max(0,draw.clip.position.x),std::max(0,draw.clip.position.y),0,0};
                scissor.w = static_cast<int>(std::min<std::int64_t>(size.x,static_cast<std::int64_t>(draw.clip.position.x)+draw.clip.size.x)-scissor.x);
                scissor.h = static_cast<int>(std::min<std::int64_t>(size.y,static_cast<std::int64_t>(draw.clip.position.y)+draw.clip.size.y)-scissor.y);
                if (scissor.w > 0 && scissor.h > 0) {
                    SDL_SetGPUScissor(pass,&scissor);
                    SDL_DrawGPUIndexedPrimitives(pass,static_cast<Uint32>((end-i)*6),1,static_cast<Uint32>((offset+i)*6),0,0);
                }
                i = end;
            }
            passOwner.reset(); offset += recorded.draws.size();
        };
        for (const auto & pass : passes) render(*pass,texture({pass->output,pass},nullptr),pass->output->size,SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
        if (swapchain) render(presentation,swapchain,{width,height},SDL_GetGPUSwapchainTextureFormat(device,window));
    }
};

Renderer::Renderer(void * window, bool vsync) : impl(std::make_unique<Impl>()) {
    impl->initialize(static_cast<SDL_Window *>(window),vsync);
}
Renderer::~Renderer() = default;
Renderer::Statistics Renderer::getStatistics() const { return {impl->texturesCreated,impl->pipelinesCreated,impl->skippedPresentations}; }
void Renderer::present(const Texture & image) {
    if (!impl->window) throw std::logic_error("Offscreen renderer cannot present");
    if (SDL_GetWindowFlags(impl->window) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) {
        ++impl->skippedPresentations;
        return;
    }
    for (const auto material : {Material::Textured,Material::Palette,Material::Fade})
        impl->pipeline(material,SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
    impl->pipeline(Material::Textured,SDL_GetGPUSwapchainTextureFormat(impl->device,impl->window));
    CommandOwner owner{SDL_AcquireGPUCommandBuffer(impl->device)};
    auto * command = owner.command;
    require(command != nullptr, "Acquire GPU command buffer");
    SDL_GPUTexture * swapchain = nullptr;
    Uint32 width = 0,height = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command,impl->window,&swapchain,&width,&height)) {
        require(false,"Acquire GPU swapchain");
    }
    owner.swapchainAcquired = swapchain != nullptr;
    if (swapchain) impl->encode(command,image.capture(),swapchain,width,height);
    else ++impl->skippedPresentations;
    require(SDL_SubmitGPUCommandBuffer(owner.release()),"Submit GPU frame");
}
std::vector<std::uint8_t> Renderer::readback(const Texture & image) {
    const auto size = image.getSize();
    const auto length = static_cast<std::size_t>(size.x)*size.y*4;
    if (length > std::numeric_limits<Uint32>::max()) throw std::runtime_error("GPU readback too large");
    for (const auto material : {Material::Textured,Material::Palette,Material::Fade})
        impl->pipeline(material,SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
    SDL_GPUTransferBufferCreateInfo info{SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,static_cast<Uint32>(length),0};
    std::unique_ptr<SDL_GPUTransferBuffer,TransferRelease> transferOwner(SDL_CreateGPUTransferBuffer(impl->device,&info),TransferRelease{impl->device});
    auto * transfer = transferOwner.get();
    require(transfer != nullptr,"Create readback transfer");
    CommandOwner owner{SDL_AcquireGPUCommandBuffer(impl->device)};
    auto * command = owner.command;
    require(command != nullptr,"Acquire readback commands");
    impl->encode(command,image.capture(),nullptr,0,0);
    std::unique_ptr<SDL_GPUCopyPass,CopyPassEnd> copyOwner(SDL_BeginGPUCopyPass(command));
    auto * copy = copyOwner.get();
    require(copy != nullptr,"Begin readback copy");
    SDL_GPUTextureRegion source{};
    source.texture = impl->texture(image.capture(),nullptr); source.w = size.x; source.h = size.y; source.d = 1;
    SDL_GPUTextureTransferInfo destination{};
    destination.transfer_buffer = transfer; destination.pixels_per_row = size.x; destination.rows_per_layer = size.y;
    SDL_DownloadFromGPUTexture(copy,&source,&destination); copyOwner.reset();
    auto * fence = SDL_SubmitGPUCommandBufferAndAcquireFence(owner.release());
    require(fence != nullptr,"Submit GPU readback");
    const bool waited = SDL_WaitForGPUFences(impl->device,true,&fence,1);
    SDL_ReleaseGPUFence(impl->device,fence);
    require(waited,"Wait for GPU readback");
    const auto * pixels = static_cast<const std::uint8_t *>(SDL_MapGPUTransferBuffer(impl->device,transfer,false));
    require(pixels != nullptr,"Map GPU readback");
    std::vector<std::uint8_t> result(pixels,pixels+length);
    SDL_UnmapGPUTransferBuffer(impl->device,transfer);
    return result;
}
}
