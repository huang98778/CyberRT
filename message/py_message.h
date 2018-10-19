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

#ifndef CYBER_MESSAGE_PY_MESSAGE_H_
#define CYBER_MESSAGE_PY_MESSAGE_H_

#include <iostream>
#include <sstream>
#include <string>

#include "cyber/common/macros.h"

namespace apollo {
namespace cyber {
namespace message {

static const char* const PY_MESSAGE_FULLNAME =
    "apollo.cyber.message.PyMessage";
// static const std::string data_split_pattern = "#@";

class PyMessageWrap {
 public:
  PyMessageWrap() : type_name_("") {}
  PyMessageWrap(const std::string& msg, const std::string& type_name)
      : data_(msg), type_name_(type_name) {}
  PyMessageWrap(const PyMessageWrap& msg)
      : data_(msg.data_), type_name_(msg.type_name_) {}
  virtual ~PyMessageWrap() {}

  class Descriptor {
   public:
    std::string full_name() const { return PY_MESSAGE_FULLNAME; }
    std::string name() const { return PY_MESSAGE_FULLNAME; }
  };

  static const Descriptor* descriptor();
  static std::string TypeName();

  bool SerializeToString(std::string* output) const;
  bool ParseFromString(const std::string& msgstr);

  const std::string& data() const;
  void set_data(const std::string& msg);
  const std::string& type_name();
  void set_type_name(const std::string& type_name);

 private:
  std::string data_;
  std::string type_name_;
};

inline void PyMessageWrap::set_data(const std::string& msg) { data_ = msg; }

inline const std::string& PyMessageWrap::data() const { return data_; }

inline bool PyMessageWrap::ParseFromString(const std::string& msgstr) {
  // todo : will use submsg type ywf
  // std::size_t pos = msgstr.rfind(data_split_pattern);
  // if (pos != std::string::npos) {
  //   std::size_t split_count = data_split_pattern.size();
  //   data_ = msgstr.substr(0, pos);
  //   type_name_ = msgstr.substr(pos + split_count);
  //   return true;
  // }
  data_ = msgstr;
  return true;
}

inline bool PyMessageWrap::SerializeToString(std::string* output) const {
  if (!output) {
    return false;
  }
  // todo : will use submsg type ywf
  // *output = data_ + data_split_pattern + type_name_;
  *output = data_;
  return true;
}

inline const std::string& PyMessageWrap::type_name() { return type_name_; }

inline void PyMessageWrap::set_type_name(const std::string& type_name) {
  type_name_ = type_name;
}

inline const PyMessageWrap::Descriptor* PyMessageWrap::descriptor() {
  static Descriptor desc;
  return &desc;
}

inline std::string PyMessageWrap::TypeName() { return PY_MESSAGE_FULLNAME; }

}  // namespace message
}  // namespace cyber
}  // namespace apollo

#endif  // CYBER_MESSAGE_PY_MESSAGE_H_
