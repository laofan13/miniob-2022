/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2022/07/05.
//

#include "sql/expr/tuple_cell.h"
#include "storage/common/field.h"
#include "common/log/log.h"
#include "util/comparator.h"
#include "util/util.h"

#include <string>
#include <regex>

void TupleCell::to_string(std::ostream &os) const
{
  switch (attr_type_) {
  case INTS: {
    os << *(int *)data_;
  } break;
  case FLOATS: {
    float v = *(float *)data_;
    os << double2string(v);
  } break;
  case DATES: {
    int value = *(int *)data_;
    char buf[16] = {0};
    snprintf(buf,sizeof(buf),"%04d-%02d-%02d",value/10000,(value%10000)/100,value%100);
    os << buf;
  } break;
  case CHARS: {
    for (int i = 0; i < length_; i++) {
      if (data_[i] == '\0') {
        break;
      }
      os << data_[i];
    }
  } break;
  case TEXTS: {
    to_text(os);
  } break;
  default: {
    LOG_WARN("unsupported attr type: %d", attr_type_);
  } break;
  }
}

void TupleCell::to_text(std::ostream &os) const {
  for (int i = PAGENUMSIZE; i < length_; i++) {
    if (data_[i] == '\0') {
      break;
    }
  }
  data_[length_] = '\0';
  os << data_ + PAGENUMSIZE;

  // for (int i = 0; i < TEXTPAGESIZE; i++) {
  //   if (text_data_[i] == '\0') {
  //     break;
  //   }
  // }
  text_data_[TEXTPAGESIZE-TEXTPATCHSIZE] = '\0';
  os << text_data_;
}


int TupleCell::compare(const TupleCell &other) const
{
  if (this->attr_type_ == other.attr_type_) {
    switch (this->attr_type_) {
    case INTS: return compare_int(this->data_, other.data_);
    case FLOATS: return compare_float(this->data_, other.data_);
    case DATES: return compare_int(this->data_, other.data_);
    case CHARS: return compare_string(this->data_, this->length_, other.data_, other.length_);
    default: {
      LOG_WARN("unsupported type: %d", this->attr_type_);
    }
    }
  } else if (this->attr_type_ == INTS && other.attr_type_ == FLOATS) {
    float this_data = *(int *)data_;
    return compare_float(&this_data, other.data_);
  } else if (this->attr_type_ == FLOATS && other.attr_type_ == INTS) {
    float other_data = *(int *)other.data_;
    return compare_float(data_, &other_data);
  }else if (this->attr_type_ == INTS && other.attr_type_ == CHARS) {
    float this_data = *(int *)data_;
    float other_data = std::atof((char *)(other.data_));
    return compare_float(&this_data, &other_data);
  }else if (this->attr_type_ == CHARS && other.attr_type_ == INTS) {
    float this_data = std::atof((char *)(data_));
    float other_data = *(int *)other.data_;
    return compare_float(&this_data, &other_data);
  }else if (this->attr_type_ == FLOATS && other.attr_type_ == CHARS) {
    float other_data = std::atof((char *)(other.data_));
    return compare_float(data_, &other_data);
  }else if (this->attr_type_ == CHARS && other.attr_type_ == FLOATS) {
    float this_data = std::atof((char *)(data_));
    return compare_float(&this_data, other.data_);
  }
  LOG_WARN("not supported");
  return -1; // TODO return rc?
}

int TupleCell::like_match(const TupleCell &other) const
{
  if (this->attr_type_ == other.attr_type_) {
   if(this->attr_type_ != CHARS) {
    LOG_WARN("like match unsupported type: %d", this->attr_type_);
    return -1;
   }
   char *s = (char *)this->data_;
   char *t = (char *)other.data();
   std::string str;
   for(int i =0;i < other.length();i++) {
    if(t[i] == '%') {
      str.push_back('.');
      str.push_back('*');
    }else if(t[i] == '_') {
      str.push_back('.');
    }else{
      str.push_back(t[i]);
    }
   }
   std::regex pattern(str);
   int ret = std::regex_match(s,pattern);
   return !ret;
  } 
  LOG_WARN("not supported");
  return -1; // TODO return rc?
}
