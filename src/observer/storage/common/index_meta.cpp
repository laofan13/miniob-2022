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
// Created by Meiyi & Wangyunlai.wyl on 2021/5/18.
//

#include "storage/common/index_meta.h"
#include "storage/common/field_meta.h"
#include "storage/common/table_meta.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "rc.h"
#include "json/json.h"

const static Json::StaticString FIELD_NAME("name");
const static Json::StaticString FIELDS_NAME("fields");

RC IndexMeta::init(const char *name, const std::vector<FieldMeta> &fields)
{
  if (common::is_blank(name)) {
    LOG_ERROR("Failed to init index, name is empty.");
    return RC::INVALID_ARGUMENT;
  }

  name_ = name;
  for(auto &field: fields) {
    fields_.push_back(field.name());
  }
  return RC::SUCCESS;
}

void IndexMeta::to_json(Json::Value &json_value) const
{
  json_value[FIELD_NAME] = name_;

  Json::Value fields_name;
  for (auto &field : fields_) {
    fields_name.append(field);
  }
  json_value[FIELDS_NAME] = std::move(fields_name);
}

RC IndexMeta::from_json(const TableMeta &table, const Json::Value &json_value, IndexMeta &index)
{
  const Json::Value &name_value = json_value[FIELD_NAME];
  const Json::Value &fields_value = json_value[FIELDS_NAME];
  if (!name_value.isString()) {
    LOG_ERROR("Index name is not a string. json value=%s", name_value.toStyledString().c_str());
    return RC::GENERIC_ERROR;
  }

  if (!fields_value.isArray() || fields_value.size() <= 0) {
    LOG_ERROR("Invalid table meta. fields is not array, json value=%s", fields_value.toStyledString().c_str());
    return RC::GENERIC_ERROR;
  }

  std::vector<FieldMeta> fields;
  for (int i = 0; i < fields_value.size(); i++) {
    const Json::Value &field_value = fields_value[i];
    if (!name_value.isString()) {
      LOG_ERROR("field name is not a string. json value=%s", field_value.toStyledString().c_str());
      return RC::GENERIC_ERROR;
    }
    const FieldMeta *field = table.field(field_value.asCString());
    if (nullptr == field) {
      LOG_ERROR("Deserialize index [%s]: no such field: %s", name_value.asCString(), field_value.asCString());
      return RC::SCHEMA_FIELD_MISSING;
    }
    fields.push_back(*field);
  }

  return index.init(name_value.asCString(), fields);
}

const char *IndexMeta::name() const
{
  return name_.c_str();
}

const std::vector<std::string> &IndexMeta::fields() const
{
  return fields_;
}

void IndexMeta::desc(std::ostream &os) const
{
  os << "index name=" << name_ << ", field=(";

  for(int i =0 ;i < fields_.size();i++) {
    if(i != fields_.size() - 1) {
      os << fields_[i] << ",";
    }else{
      os << fields_[i] << ")";
    }
  }
}