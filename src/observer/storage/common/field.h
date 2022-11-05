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
#include "sql/stmt/select_stmt.h"

#include <sstream>

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

class QueryField: public Field
{
public:
  QueryField() = default;
  QueryField(const Value *value): value_(value)
  {
  }
  QueryField(const Value *value,FuncType func_type): 
  value_(value),func_type_(func_type)
  {
    is_has_func_ = true;
  }
  QueryField(const Table *table, const FieldMeta *field ) 
  : Field(table,field)
  {
    is_attr_ = true;
  }
  QueryField(const Table *table, const FieldMeta *field ,bool is_table_name) 
  : Field(table,field), is_table_name_(is_table_name)
  {
    is_attr_ = true;
  }
  QueryField(const Table *table, const FieldMeta *field, FuncType func_type) 
  : Field(table,field),func_type_(func_type)
  {
    is_attr_ = true;
    is_has_func_ = true;
  }
  QueryField(const Table *table, const FieldMeta *field,FuncType func_type, bool is_table_name) 
  : Field(table,field),func_type_(func_type), is_table_name_(is_table_name)
  {
    is_attr_ = true;
    is_has_func_ = true;
  }

  void set_is_attr(bool is_attr) {
    is_attr_ = is_attr;
  }
  void set_is_has_func(bool is_has_func) {
    is_has_func_ = is_has_func;
  }
  void set_value(Value* value)
  {
    this->value_ = value;
  }
  void set_func_type(FuncType func_type)
  {
    this->func_type_ = func_type;
  }
  void set_is_table_name(bool is_table_name)
  {
    this->is_table_name_ = is_table_name;
  }

  bool is_attr() {
    return is_attr_;
  };
  bool is_has_func() {
    return is_has_func_;
  };
  const Value *value() const {
    return value_;
  };
  FuncType func_type(){ 
    return func_type_; 
  };
  
  std::string string() const
  {
    static const char *FUNC_TYPE_NAME[] = {"max","min","count","avg","sum","length","round","data_format"};
    std::stringstream ss;
    if(!is_has_func_) {
      if(is_attr_) {
        if(is_table_name_) {
          ss << table_name();
          ss << ".";
          ss << field_name();
        }else{
          ss << field_name();
        }
      }else { // value
        ss <<(char *) value_->data;
      }
    }else {
      ss << FUNC_TYPE_NAME[func_type_];
      ss << "(";
      if(is_attr_) {
        if(is_table_name_) {
          ss << table_name();
          ss << ".";
          ss << field_name();
        }else{
          ss << field_name();
        }
      }else { // value
        ss <<(char *) value_->data;
      }
      ss << ")";
    }
    return ss.str();
  }

private:
  bool is_attr_;
  const Value *value_ = nullptr;
  bool is_has_func_;
  FuncType func_type_;
  bool is_table_name_;
};

class UpdateField: public Field
{
public:
  UpdateField() = default;
  UpdateField(const Table *table, const FieldMeta *field, const Value* value) :
  Field(table,field), value_(value)
  {}
  UpdateField(const Table *table, const FieldMeta *field, const Value* value,bool is_subselect, SelectStmt *select_stmt) :
  Field(table,field), value_(value) ,is_subselect_(is_subselect), select_stmt_(select_stmt)
  {}

  ~UpdateField () {
    
  }

  const Value value() const { 
    return *value_; 
  };
  void set_value(const Value* value)
  {
    this->value_ = value;
  }

  bool is_has_subselect() const {
    return is_subselect_ == 1; 
  };
  void set_subselect(bool is_subselect)
  {
    this->is_subselect_ = is_subselect;
  }

  SelectStmt * select_stmt() const {
    return select_stmt_; 
  };

  void set_select_value(Value &select_value)
  {
    this->select_value_ = select_value;
  }

  Value select_value() const
  {
    return select_value_;
  }

private:
  const Value *value_ = nullptr;
  bool is_subselect_ = false;
  SelectStmt *select_stmt_ = nullptr;
  Value select_value_;
};

class SortField: public Field
{
public:
  SortField() = default;
  SortField(const Table *table, const FieldMeta *field, OrderType order_type) :
  Field(table,field), order_type_(order_type)
  {}

  ~SortField () = default;


  void set_order_type(OrderType order_type)
  {
    this->order_type_ = order_type;
  }

  OrderType order_type() const
  {
    return order_type_;
  }

private:
  OrderType order_type_;
};