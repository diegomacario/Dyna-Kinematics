#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <memory>
#include <unordered_map>
#include <iostream>

template<typename TResource>
class ResourceManager
{
public:

   ResourceManager() = default;
   ~ResourceManager() = default;

   ResourceManager(const ResourceManager&) = delete;
   ResourceManager& operator=(const ResourceManager&) = delete;

   ResourceManager(ResourceManager&&) = default;
   ResourceManager& operator=(ResourceManager&&) = default;

   template<typename TResourceLoader, typename... Args>
   std::shared_ptr<TResource> loadResource(const std::string& resourceID, Args&&... args);

   template<typename TResourceLoader, typename... Args>
   std::shared_ptr<TResource> loadUnmanagedResource(Args&&... args) const;

   std::shared_ptr<TResource> getResource(const std::string& resourceID) const;

   bool                       containsResource(const std::string& resourceID) const noexcept;

   void                       stopManagingResource(const std::string& resourceID) noexcept;
   void                       stopManagingAllResources() noexcept;

private:

   std::unordered_map<std::string, std::shared_ptr<TResource>> mResources;
};

template<typename TResource>
template<typename TResourceLoader, typename... Args>
std::shared_ptr<TResource> ResourceManager<TResource>::loadResource(const std::string& resourceID, Args&&... args)
{
   std::shared_ptr<TResource> resource{};

   auto it = mResources.find(resourceID);
   if (it == mResources.cend())
   {
      resource = TResourceLoader{}.loadResource(std::forward<Args>(args)...);

      // We only store the resource if it is not a nullptr
      // We expect the loaders to print an error message when they are unable to load a resource successfully, which is why we don't print anything here
      if (resource)
      {
         mResources[resourceID] = resource;
      }
   }
   else
   {
      std::cout << "Warning - ResourceManager::loadResource - A resource with the following ID already exists: " << resourceID << "\n";
      resource = it->second;
   }

   return resource;
}

template<typename TResource>
template<typename TResourceLoader, typename... Args>
std::shared_ptr<TResource> ResourceManager<TResource>::loadUnmanagedResource(Args&&... args) const
{
   // We expect the loaders to print an error message when they are unable to load a resource successfully, which is why we don't print anything here
   return TResourceLoader{}.loadResource(std::forward<Args>(args)...);
}

template<typename TResource>
std::shared_ptr<TResource> ResourceManager<TResource>::getResource(const std::string& resourceID) const
{
   auto it = mResources.find(resourceID);
   if (it != mResources.end())
   {
      return it->second;
   }
   else
   {
      std::cout << "Error - ResourceManager::getResource - A resource with the following ID does not exist: " << resourceID << "\n";
      return nullptr;
   }
}

template<typename TResource>
bool ResourceManager<TResource>::containsResource(const std::string& resourceID) const noexcept
{
   return (mResources.find(resourceID) != mResources.cend());
}

template<typename TResource>
void ResourceManager<TResource>::stopManagingResource(const std::string& resourceID) noexcept
{
   auto it = mResources.find(resourceID);
   if (it != mResources.end())
   {
      mResources.erase(it);
   }
   else
   {
      std::cout << "Error - ResourceManager::stopManagingResource - A resource with the following ID does not exist: " << resourceID << "\n";
   }
}

template<typename TResource>
void ResourceManager<TResource>::stopManagingAllResources() noexcept
{
   mResources.clear();
}

#endif
