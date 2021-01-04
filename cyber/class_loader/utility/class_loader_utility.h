/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#ifndef CYBER_CLASS_LOADER_UTILITY_CLASS_LOADER_UTILITY_H_
#define CYBER_CLASS_LOADER_UTILITY_CLASS_LOADER_UTILITY_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "cyber/class_loader/shared_library/shared_library.h"
#include "cyber/class_loader/utility/class_factory.h"
#include "cyber/common/log.h"

/**
 *  class register implementation
 */
namespace apollo {
namespace cyber {
namespace class_loader {

class ClassLoader;

namespace utility {

using ClassClassFactoryMap =
    std::map<std::string, utility::AbstractClassFactoryBase*>;
using BaseToClassFactoryMapMap = std::map<std::string, ClassClassFactoryMap>;

std::recursive_mutex& GetClassFactoryMapMapMutex();

ClassClassFactoryMap& GetClassFactoryMapByBaseClass(
    const std::string& base_typeid);

std::string GetCurLoadingLibraryName();
ClassLoader* GetCurActiveClassLoader();

bool IsLibraryLoaded(const std::string& library_path);
bool LoadLibrary(const std::string& library_path, ClassLoader* loader);
void UnloadLibrary(const std::string& library_path, ClassLoader* loader);

template <typename Derived, typename Base>
void RegisterClass(const std::string& class_name,
                   const std::string& base_class_name);

template <typename Base>
Base* CreateClassObj(const std::string& class_name, ClassLoader* loader);

template <typename Base>
std::vector<std::string> GetValidClassNames(ClassLoader* loader);

template <typename Derived, typename Base>
void RegisterClass(const std::string& class_name,
                   const std::string& base_class_name) {
  AINFO << "Register class:" << class_name << "," << base_class_name;
  utility::AbstractClassFactory<Base>* new_class_factory_obj =
      new utility::ClassFactory<Derived, Base>(class_name, base_class_name);
  auto curr_active_loader = GetCurActiveClassLoader();
  new_class_factory_obj->AddOwnedClassLoader(curr_active_loader);

  GetClassFactoryMapMapMutex().lock();
  ClassClassFactoryMap& factory_map =
      GetClassFactoryMapByBaseClass(typeid(Base).name());
  factory_map[class_name] = new_class_factory_obj;
  GetClassFactoryMapMapMutex().unlock();
}

template <typename Base>
Base* CreateClassObj(const std::string& class_name, ClassLoader* loader) {
  GetClassFactoryMapMapMutex().lock();
  ClassClassFactoryMap& factoryMap =
      GetClassFactoryMapByBaseClass(typeid(Base).name());
  AbstractClassFactory<Base>* factory = nullptr;
  if (factoryMap.find(class_name) != factoryMap.end()) {
    factory = dynamic_cast<utility::AbstractClassFactory<Base>*>(
        factoryMap[class_name]);
  }
  GetClassFactoryMapMapMutex().unlock();

  Base* classobj = nullptr;
  if (factory && factory->IsOwnedBy(loader)) {
    classobj = factory->CreateObj();
  }

  return classobj;
}

template <typename Base>
std::vector<std::string> GetValidClassNames(ClassLoader* loader) {
  std::lock_guard<std::recursive_mutex> lock(GetClassFactoryMapMapMutex());

  ClassClassFactoryMap& factoryMap =
      GetClassFactoryMapByBaseClass(typeid(Base).name());
  std::vector<std::string> classes;
  for (auto& class_factory : factoryMap) {
    AbstractClassFactoryBase* factory = class_factory.second;
    if (factory && factory->IsOwnedBy(loader)) {
      classes.emplace_back(class_factory.first);
    }
  }

  return classes;
}

}  // namespace utility
}  // namespace class_loader
}  // namespace cyber
}  // namespace apollo

#endif  // CYBER_CLASS_LOADER_UTILITY_CLASS_LOADER_UTILITY_H_
