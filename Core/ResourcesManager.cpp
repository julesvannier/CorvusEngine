#include "ResourcesManager.h"
#include "Image.h"

ResourcesManager::ResourcesManager(std::shared_ptr<D3D12Driver> device) : m_device(device)
{
}

ResourcesManager::~ResourcesManager()
{
}

std::shared_ptr<Texture> ResourcesManager::LoadTexture(const std::string& texPath, Image& img, TextureFormat format)
{
    if(texPath.empty())
        return nullptr;
    
    if(auto device = m_device.lock())
    {
        auto weakTex = m_textures.find(texPath);
        if(weakTex != m_textures.end())
        {
            if(auto tex = weakTex->second.lock())
                return tex;
        }

        img.LoadImageFromFile(texPath);
        auto texture = device->CreateTexture(img.Width, img.Height, format, TextureType::ShaderResource, texPath.c_str());
        device->CreateShaderResourceView(texture);
        device->UploadTextureData(img, texture);

        m_textures.emplace(texPath, texture);

        return texture;
    }

    return nullptr;
}

std::shared_ptr<RenderItem> ResourcesManager::LoadMesh(const std::string& meshPath)
{
    if(meshPath.empty())
        return nullptr;

    if(auto device = m_device.lock())
    {
        auto weakMesh = m_renderItems.find(meshPath);
        if(weakMesh != m_renderItems.end())
        {
            if(auto mesh = weakMesh->second.lock())
                return mesh;
        }

        auto model = std::make_shared<RenderItem>();
        model->ImportMesh(device, meshPath);

        m_renderItems.emplace(meshPath, model);
        
        return model;
    }

    return nullptr;
}
