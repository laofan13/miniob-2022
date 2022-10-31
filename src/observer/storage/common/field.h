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
// Created by Wangyunlai on 2022/07/05.
//

#pragma once

#include "storage/common/table.h"
#include "storage/common/field_meta.h"

class Field
{
public:
  Field() = default;
  Field(const Table *table, const FieldMeta *field) : table_(table), field_(field)
  {}

  const Table *table() const { return table_; }
  const FieldMeta *meta() const { return field_; }

  AttrType attr_type() const
  {
    return field_->type();
  }

  const char *table_name() const { return table_->name(); }
  const char *field_name() const { return field_->name(); }

  void set_table(const Table *table)
  {
    this->table_ = table;
  }
  void set_field(const FieldMeta *field)
  {
    this->field_ = field;
  }
private:
  const Table *table_ = nullptr;
  const FieldMeta *field_ = nullptr;
};

class AggrField
{
public:
  AggrField() = default;
  AggrField(const Table *table, AggrType aggr_type) 
  : table_(table), aggr_type_(aggr_type)
  {}
  AggrField(const Table *table, const FieldMeta *field, AggrType aggr_type) 
  : table_(table), field_(field), aggr_type_(aggr_type)
  {}

  const Table *table() const { return table_; }
  const FieldMeta *meta() const { return field_; }

  AttrType attr_type() const
  {
    return field_->type();
  }

  const char *table_name() const { return table_->name(); }
  const char *field_name() const { return field_name_; }
  AggrType aggr_type(){ return aggr_type_; }

  void set_table(const Table *table)
  {
    this->table_ = table;
  }
  void set_field(const FieldMeta *field)
  {
    this->field_ = field;
  }
  void set_field_name(const char *field_name)
  {
    this->field_name_ = field_name;
  }
  const char* aggrr_type_to_string() {
    static const char *AGGR_TYPE_NAME[] = {"max","min","count","avg","sum"};
    return AGGR_TYPE_NAME[aggr_type_];
  }
private:
  const Table *table_ = nullptr;
  const FieldMeta *field_ = nullptr;
  const char *field_name_ = nullptr;
  AggrType aggr_type_;
};

class UpdateField: public Field
{
public:
  UpdateField() = default;
  UpdateField(const Table *table, const FieldMeta *field, const Value* value) :
  Field(table,field), value_(value)
  {}

  const Value *value() const { return value_; }

  void set_value(const Value* value)
  {
    this->value_ = value;
  }
private:
  const Value *value_ = nullptr;
};