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

#include "./cyber_topology_message.h"
#include "./channel_msg_factory.h"
#include "./general_channel_message.h"
#include "./screen.h"

#include "cyber/message/message_traits.h"
#include "cyber/proto/topology_change.pb.h"
#include "cyber/proto/role_attributes.pb.h"

#include <ncurses.h>
#include <iomanip>
#include <iostream>

constexpr int SecondColumnOffset = 4;

CyberTopologyMessage::CyberTopologyMessage(const std::string& channel)
    : RenderableMessage(nullptr, 1),
      second_column_(SecondColumnType::MessageFrameRatio),
      col1_width_(8),
      specified_channel_(channel),
      all_channels_map_() {}

CyberTopologyMessage::~CyberTopologyMessage(void) {
  apollo::cyber::Shutdown();
  for (auto item : all_channels_map_) {
    if (!ChannelMessage::isErrorCode(item.second)) {
      delete item.second;
    }
  }
}

RenderableMessage* CyberTopologyMessage::Child(int lineNo) const {
  RenderableMessage* ret = nullptr;
  --lineNo;

  if (lineNo > -1 && lineNo < page_item_count_) {
    int i = 0;

    auto iter = all_channels_map_.cbegin();
    while (i < page_index_ * page_item_count_) {
      ++iter;
      ++i;
    }

    for (i = 0; iter != all_channels_map_.cend(); ++iter) {
      if (i == lineNo) {
        if (!ChannelMessage::isErrorCode(iter->second)) {
          ret = iter->second;
        }
        break;
      }
      ++i;
    }
  }
  return ret;
}

void CyberTopologyMessage::TopologyChanged(
    const apollo::cyber::proto::ChangeMsg& changeMsg) {
  if (::apollo::cyber::proto::OperateType::OPT_JOIN ==
      changeMsg.operate_type()) {
    bool isWriter = true;
    if (::apollo::cyber::proto::RoleType::ROLE_READER ==
        changeMsg.role_type())
      isWriter = false;
    AddReaderWriter(changeMsg.role_attr(), isWriter);
  } else {
    auto iter = all_channels_map_.find(changeMsg.role_attr().channel_name());

    if (iter != all_channels_map_.cend() &&
        !ChannelMessage::isErrorCode(iter->second)) {
      const std::string& nodeName = changeMsg.role_attr().node_name();
      if (::apollo::cyber::proto::RoleType::ROLE_WRITER ==
          changeMsg.role_type()) {
        changeMsg.role_attr();
        iter->second->del_writer(nodeName);
      } else {
        iter->second->del_reader(nodeName);
      }
    }
  }
}

void CyberTopologyMessage::AddReaderWriter(
    const apollo::cyber::proto::RoleAttributes& role, bool isWriter) {
  const std::string& channelName = role.channel_name();

  if (!specified_channel_.empty() && specified_channel_ != channelName) {
    return;
  }

  if (static_cast<int>(channelName.length()) > col1_width_) {
    col1_width_ = channelName.length();
  }

  const std::string& nodeName = role.node_name();
  if (ChannelMsgFactory::Instance()->isFromHere(nodeName)) {
    return;
  }

  ChannelMessage* channelMsg = nullptr;
  const std::string& msgTypeName = role.message_type();
  auto iter = all_channels_map_.find(channelName);
  if (iter == all_channels_map_.cend()) {
    channelMsg = ChannelMsgFactory::Instance()->CreateChannelMessage(
        msgTypeName, channelName);

    if (!ChannelMessage::isErrorCode(channelMsg)) {
      channelMsg->set_parent(this);
      channelMsg->set_message_type(msgTypeName);
      channelMsg->add_reader(channelMsg->NodeName());
    }

    all_channels_map_[channelName] = channelMsg;
  } else {
    channelMsg = iter->second;
  }

  if (!ChannelMessage::isErrorCode(channelMsg)) {
    if (isWriter) {
      if (msgTypeName != apollo::cyber::message::MessageType<
                             apollo::cyber::message::RawMessage>()) {
        channelMsg->set_message_type(msgTypeName);
      }

      channelMsg->add_writer(nodeName);
    } else {
      channelMsg->add_reader(nodeName);
    }
  }
}

void CyberTopologyMessage::ChangeState(const Screen* s, int key) {
  switch (key) {
    case 'f':
    case 'F':
      second_column_ = SecondColumnType::MessageFrameRatio;
      break;

    case 't':
    case 'T':
      second_column_ = SecondColumnType::MessageType;
      break;

    case ' ': {
      ChannelMessage* child = static_cast<ChannelMessage*>(Child(*line_no()));
      if (child) {
        child->set_enabled(!child->is_enabled());
      }
    }

    default:;
  }
}

void CyberTopologyMessage::Render(const Screen* s, int key) {
  page_item_count_ = s->Height() - 1;
  pages_ = all_channels_map_.size() / page_item_count_ + 1;
  ChangeState(s, key);
  SplitPages(key);

  s->AddStr(0, 0, Screen::WHITE_BLACK, "Channels");
  switch (second_column_) {
    case SecondColumnType::MessageType:
      s->AddStr(col1_width_ + SecondColumnOffset, 0, Screen::WHITE_BLACK,
                "TypeName");
      break;
    case SecondColumnType::MessageFrameRatio:
      s->AddStr(col1_width_ + SecondColumnOffset, 0, Screen::WHITE_BLACK,
                "FrameRatio");
      break;
  }

  auto iter = all_channels_map_.cbegin();
  register int tmp = page_index_ * page_item_count_;
  register int line = 0;
  while (line < tmp) {
    ++iter;
    ++line;
  }

  Screen::ColorPair color;
  std::ostringstream outStr;

  tmp = page_item_count_ + 1;
  for (line = 1; iter != all_channels_map_.cend() && line < tmp;
       ++iter, ++line) {
    color = Screen::RED_BLACK;

    if (!ChannelMessage::isErrorCode(iter->second)) {
      if (iter->second->has_message_come()) {
        if (iter->second->is_enabled()) {
          color = Screen::GREEN_BLACK;
        } else {
          color = Screen::YELLOW_BLACK;
        }
      }
    }

    s->SetCurrentColor(color);
    s->AddStr(0, line, iter->first.c_str());

    if (!ChannelMessage::isErrorCode(iter->second)) {
      switch (second_column_) {
        case SecondColumnType::MessageType:
          s->AddStr(col1_width_ + SecondColumnOffset, line,
                    iter->second->message_type().c_str());
          break;
        case SecondColumnType::MessageFrameRatio: {
          outStr.str("");
          outStr << std::fixed << std::setprecision(2)
                 << iter->second->frame_ratio();
          s->AddStr(col1_width_ + SecondColumnOffset, line,
                    outStr.str().c_str());
        } break;
      }
    } else {
      ChannelMessage::ErrorCode errcode =
          ChannelMessage::castPtr2ErrorCode(iter->second);
      s->AddStr(col1_width_ + SecondColumnOffset, line,
                ChannelMessage::errCode2Str(errcode));
    }
    s->ClearCurrentColor();
  }
}
