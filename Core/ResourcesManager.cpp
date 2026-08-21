#include "ResourcesManager.h"
#include "Image.h"

ResourcesManager::ResourcesManager(std::shared_ptr<D3D12Driver> renderer) : m_renderer(renderer)
{
}

ResourcesManager::~ResourcesManager()
{
}

std::shared_ptr<Texture> ResourcesManager::LoadTexture(const std::string& texPath, Image& img)
{
    if(texPath.empty())
        return nullptr;
    
    if(auto renderer = m_renderer.lock())
    {
        auto weakTex = m_textures.find(texPath);
        if(weakTex != m_textures.end())
        {
            if(auto tex = weakTex->second.lock())
                return tex;
        }

        img.LoadImageFromFile(texPath);
        auto texture = renderer->CreateTexture(img.Width, img.Height, TextureFormat::RGBA8, TextureType::ShaderResource);
        renderer->CreateShaderResourceView(texture);
        renderer->UploadTextureData(img, texture);

        m_textures.emplace(texPath, texture);

        return texture;
    }

    return nullptr;
}

std::shared_ptr<RenderItem> ResourcesManager::LoadMesh(const std::string& meshPath)
{
    if(meshPath.empty())
        return nullptr;

    if(auto renderer = m_renderer.lock())
    {
        auto weakMesh = m_renderItems.find(meshPath);
        if(weakMesh != m_renderItems.end())
        {
            if(auto mesh = weakMesh->second.lock())
                return mesh;
        }

        auto model = std::make_shared<RenderItem>();
        model->ImportMesh(renderer, meshPath);

        m_renderItems.emplace(meshPath, model);
        
        return model;
    }

    return nullptr;
}
